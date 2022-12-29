/*
 * \file    resmgr.c
 *
 * Copyright (C) 2022 Deniz Eren <deniz.eren@outlook.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <string.h>

struct can_ocb;

#define IOFUNC_OCB_T struct can_ocb
#include <sys/iofunc.h>

#include <sys/resmgr.h>
#include <sys/dispatch.h>
#include <sys/can_dcmd.h>
#include <sys/neutrino.h>

#include <resmgr.h>
#include <config.h>
#include <pci.h>
#include <session.h>

#include "netif.h"


struct net_device* device[MAX_DEVICES];

typedef struct can_ocb {
    iofunc_ocb_t core;

    struct can_resmgr_t* resmgr;

    int session_index;
    session_t *session;
} can_ocb_t;

IOFUNC_OCB_T* can_ocb_calloc (resmgr_context_t* ctp, IOFUNC_ATTR_T* attr);
void can_ocb_free (IOFUNC_OCB_T* ocb);


/*
 * Our dispatch, resource manager, and iofunc variables
 * are declared here. These are some small administrative things
 * for our resource manager.
 */

/*
 * We need n x MAX_MAILBOXES of the structures below since each mailbox has an
 * rx and a tx; i.e. /dev/can0/rx#, /dev/can0/tx#, where # is the mailbox id and
 * the 0 is the device id
 */

static int io_created = 0;

typedef enum channel_type {
    RX_CHANNEL = 0,
    TX_CHANNEL
} channel_type_t;

static struct can_resmgr_t {
    struct net_device* device;

    char name[MAX_NAME_SIZE];
    channel_type_t channel_type;

    dispatch_t *dispatch;
    resmgr_attr_t resmgr_attr;
    dispatch_context_t *dispatch_context;
    iofunc_attr_t iofunc_attr;

    int pathID;

    iofunc_mount_t mount;
    iofunc_funcs_t mount_funcs;

    pthread_attr_t dispatch_thread_attr;
    pthread_t dispatch_thread;

    pthread_attr_t tx_thread_attr;
    pthread_t tx_thread;
} can_resmgr[MAX_DEVICES][MAX_MAILBOXES*2];

static struct can_resmgr_t* _can_resmgr[MAX_DEVICES*MAX_MAILBOXES*2];

/* A resource manager mainly consists of callbacks for POSIX
 * functions a client could call. In the example, we have
 * callbacks for the open(), read() and write() calls. More are
 * possible. If we don't supply own functions (e.g. for stat(),
 * seek(), etc.), the resource manager framework will use default
 * system functions, which in most cases return with an error
 * code to indicate that this resource manager doesn't support
 * this function.*/

/* These prototypes are needed since we are using their names
 * in main(). */

int io_open  (resmgr_context_t *ctp, io_open_t  *msg, RESMGR_HANDLE_T *handle, void *extra);
int io_close_ocb (resmgr_context_t *ctp, void *reserved, RESMGR_OCB_T *ocb);
int io_read  (resmgr_context_t *ctp, io_read_t  *msg, RESMGR_OCB_T *ocb);
int io_write (resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb);
int io_devctl (resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb);

void* tx_loop (void* arg);
void* dispatch_receive_loop (void* arg);

/*
 * Our connect and I/O functions - we supply two tables
 * which will be filled with pointers to callback functions
 * for each POSIX function. The connect functions are all
 * functions that take a path, e.g. open(), while the I/O
 * functions are those functions that are used with a file
 * descriptor (fd), e.g. read().
 */

resmgr_connect_funcs_t connect_funcs;
resmgr_io_funcs_t io_funcs;


int register_netdev(struct net_device *dev) {
    snprintf(dev->name, IFNAMSIZ, "can%d", dev->dev_id);

    if (dev->dev_id >= MAX_DEVICES) {
        log_err("device id (%d) exceeds max (%d)\n", dev->dev_id, MAX_DEVICES);
    }

    device[dev->dev_id] = dev;

    log_trace("register_netdev: %s\n", dev->name);

    struct user_dev_setup user = {
            .set_bittiming = true,
            .bittiming = { .bitrate = 250000 }
    };
    dev->resmgr_ops->changelink(dev, &user);

    if (dev->netdev_ops->ndo_open(dev)) {
        return -1;
    }

    if (!io_created) {
        io_created = 1;

        /* Now, let's initialize the tables of connect functions and
         * I/O functions to their defaults (system fallback
         * routines) and then override the defaults with the
         * functions that we are providing. */
        iofunc_func_init( _RESMGR_CONNECT_NFUNCS, &connect_funcs,
                _RESMGR_IO_NFUNCS, &io_funcs );

        /* Now we override the default function pointers with
         * some of our own coded functions: */
        connect_funcs.open = io_open;
        io_funcs.close_ocb = io_close_ocb;
        io_funcs.read = io_read;
        io_funcs.write = io_write;
        io_funcs.devctl = io_devctl;
    }

    int i, j;

    for (i = 0; i < 1; ++i) { // default number of channels
        for (j = 0; j < 2; ++j) { // 2 for rx & tx
            struct can_resmgr_t* resmgr = &can_resmgr[dev->dev_id][i*2+j];

            resmgr->device = dev;

            /* Allocate and initialize a dispatch structure for use
             * by our main loop. This is for the resource manager
             * framework to use. It will receive messages for us,
             * analyze the message type integer and call the matching
             * handler callback function (i.e. io_open, io_read, etc.) */
            resmgr->dispatch = dispatch_create();

            if (resmgr->dispatch == NULL) {
                log_err("couldn't dispatch_create: %s\n", strerror(errno));

                return -1;
            }

            /* Set up the resource manager attributes structure. We'll
             * use this as a way of passing information to
             * resmgr_attach(). The attributes are used to specify
             * the maximum message length to be received at once,
             * and the number of message fragments (iov's) that
             * are possible for the reply.
             * For now, we'll just use defaults by setting the
             * attribute structure to zeroes. */
            memset(&resmgr->resmgr_attr, 0, sizeof(resmgr_attr_t));
            resmgr->resmgr_attr.nparts_max = 256;
            resmgr->resmgr_attr.msg_max_size = 8192;

            if (j == 0) {
                snprintf( resmgr->name, MAX_NAME_SIZE,
                        "/dev/can%d/rx%d", dev->dev_id, i );

                resmgr->channel_type = RX_CHANNEL;
            }
            else {
                snprintf( resmgr->name, MAX_NAME_SIZE,
                        "/dev/can%d/tx%d", dev->dev_id, i );

                resmgr->channel_type = TX_CHANNEL;
            }

            iofunc_attr_init(&resmgr->iofunc_attr, S_IFCHR | 0666, NULL, NULL);

            // set up the mount functions structure
            // with our allocate/deallocate functions

            // _IOFUNC_NFUNCS is from the .h file
            resmgr->mount_funcs.nfuncs = _IOFUNC_NFUNCS;

            // your new OCB allocator
            resmgr->mount_funcs.ocb_calloc = can_ocb_calloc;

            // your new OCB deallocator
            resmgr->mount_funcs.ocb_free = can_ocb_free;

            // set up the mount structure
            memset(&resmgr->mount, 0, sizeof (mount));

            resmgr->mount.funcs = &resmgr->mount_funcs;
            resmgr->iofunc_attr.mount = &resmgr->mount;

            resmgr->pathID = resmgr_attach(
                    resmgr->dispatch, &resmgr->resmgr_attr, resmgr->name,
                    _FTYPE_ANY, 0, &connect_funcs, &io_funcs,
                    &resmgr->iofunc_attr );

            if (resmgr->pathID == -1) {
                log_err("resmgr_attach fail: %s\n", strerror(errno));

                return -1;
            }

            _can_resmgr[resmgr->pathID] = resmgr;

            /* Now we allocate some memory for the dispatch context
             * structure, which will later be used when we receive
             * messages. */
            resmgr->dispatch_context = dispatch_context_alloc(resmgr->dispatch);

            if (resmgr->dispatch_context == NULL) {
                log_err("dispatch_context_alloc fail: %s\n", strerror(errno));

                return -1;
            }

            pthread_attr_init(&resmgr->dispatch_thread_attr);
            pthread_attr_setdetachstate(
                    &resmgr->dispatch_thread_attr, PTHREAD_CREATE_DETACHED );

            pthread_create( &resmgr->dispatch_thread,
                    &resmgr->dispatch_thread_attr,
                    &dispatch_receive_loop, resmgr );

            device_sessions[dev->dev_id].num_sessions = 0;

            log_trace("resmgr_attach -> %d\n", resmgr->pathID);

		    dev->flags |= IFF_UP;
        }
    }

    return 0;
}

void unregister_netdev(struct net_device *dev) {
    int i, j;

    log_trace("unregister_netdev: %s\n", dev->name);

    dev->flags &= ~IFF_UP;

    if (dev->netdev_ops->ndo_stop(dev)) {
        log_err("internal error; ndo_stop failure\n");
    }

    for (i = 0; i < 1; ++i) { // default number of channels
        for (j = 0; j < 2; ++j) { // 2 for rx & tx
            struct can_resmgr_t* resmgr = &can_resmgr[dev->dev_id][i*2+j];

            if (resmgr_detach(resmgr->dispatch, resmgr->pathID, 0) == -1) {
                log_err( "internal error; resmgr_detach failure (%d, %d)\n",
                        i, j );
            }

            pthread_join(resmgr->dispatch_thread, NULL);

            // TODO: investigate if dispatch_context_free() is needed and why
            //       we get SIGSEGV when following code is uncommented.
            //dispatch_context_free(resmgr->dispatch_context);

            if (dispatch_destroy(resmgr->dispatch) == -1) {
                log_err( "internal error; dispatch_destroy failure (%d, %d)\n",
                        i, j );
            }
        }
    }
}


IOFUNC_OCB_T* can_ocb_calloc (resmgr_context_t* ctp, IOFUNC_ATTR_T* attr) {
    log_trace("can_ocb_calloc -> %s\n", _can_resmgr[ctp->id]->name);

    struct can_ocb* ocb;

    if (!(ocb = calloc(1, sizeof(*ocb)))) {
        return NULL;
    }

    ocb->resmgr = _can_resmgr[ctp->id];

    struct net_device* device = ocb->resmgr->device;
    int dev_id = device->dev_id;
    device_sessions_t* ds = &device_sessions[dev_id];
    int num_sessions = ds->num_sessions;

    ocb->session_index = num_sessions;
    ocb->session = &ds->sessions[ocb->session_index];

    if (num_sessions + 1 == MAX_SESSIONS) {
    }

    queue_attr_t rx_attr = { .size = 0 };
    queue_attr_t tx_attr = { .size = 0 };

    if (ocb->resmgr->channel_type == RX_CHANNEL) {
        rx_attr.size = 1024;
    }
    else if (ocb->resmgr->channel_type == TX_CHANNEL) {
        tx_attr.size = device->tx_queue_len = 16;
    }

    if (create_session(ocb->session, device, &rx_attr, &tx_attr) != EOK) {
        log_err("create_session failed: %s\n", ocb->resmgr->name);
    }

    ds->num_sessions++;

    // If this is the first tx session for the device create the tx thread
    if (ocb->resmgr->channel_type == TX_CHANNEL &&
        number_of_tx_sessions(device) == 1)
    {
        pthread_attr_init(&ocb->resmgr->tx_thread_attr);
        pthread_attr_setdetachstate(
                &ocb->resmgr->tx_thread_attr, PTHREAD_CREATE_DETACHED );

        pthread_create( &ocb->resmgr->tx_thread,
                &ocb->resmgr->tx_thread_attr,
                &tx_loop, ocb->resmgr );
    }

    return ocb;
}

void can_ocb_free (IOFUNC_OCB_T* ocb) {
    log_trace("can_ocb_free -> %s\n", ocb->resmgr->name);

    struct net_device* device = ocb->resmgr->device;
    int dev_id = device->dev_id;
    device_sessions_t* ds = &device_sessions[dev_id];
    int num_sessions = ds->num_sessions;

    destroy_session(ocb->session);

    // If this is the last tx session for the device wait for tx thread to exit
    if (ocb->resmgr->channel_type == TX_CHANNEL &&
        number_of_tx_sessions(device) == 1)
    {
        pthread_join(ocb->resmgr->tx_thread, NULL);
    }

    ds->num_sessions--;

    int i;
    for (i = ocb->session_index; i < num_sessions; ++i) {
        ds->sessions[i] = ds->sessions[i+1];
    }

    free(ocb);
}


void* tx_loop (void* arg) {
    struct can_resmgr_t* resmgr = (struct can_resmgr_t*)arg;
    device_sessions_t* ds = &device_sessions[resmgr->device->dev_id];

    int tx_session_count = 1; // Created when first tx session is opened

    while (1) {
        if (!(resmgr->device->flags & IFF_UP) || tx_session_count == 0) {
            log_trace("tx_loop exit: %s\n", resmgr->name);

            pthread_exit(NULL);
        }

        if (netif_tx(resmgr->device)) {
            tx_session_count = number_of_tx_sessions(resmgr->device);
        }
    }

    return NULL;
}

/*
 * Resource Manager
 *
 * Message thread and callback functions for connection and IO
 */

void* dispatch_receive_loop (void* arg) {
    struct can_resmgr_t* resmgr = (struct can_resmgr_t*)arg;

    dispatch_context_t* ctp = resmgr->dispatch_context;

    /* Done! We can now go into our "receive loop" and wait
     * for messages. The dispatch_block() function is calling
     * MsgReceive() under the covers, and receives for us.
     * The dispatch_handler() function analyzes the message
     * for us and calls the appropriate callback function. */
    while (1) {
        if ((ctp = dispatch_block(ctp)) == NULL) {
            if (resmgr->device->flags & IFF_UP) {
                log_err("dispatch_block failed: %s\n", strerror(errno));
            }

            pthread_exit(NULL);
        }
        /* Call the correct callback function for the message
         * received. This is a single-threaded resource manager,
         * so the next request will be handled only when this
         * call returns. Consult our documentation if you want
         * to create a multi-threaded resource manager. */
        dispatch_handler(ctp);
    }

    return NULL;
}

/*
 *  io_open
 *
 * We are called here when the client does an open().
 * In this simple example, we just call the default routine
 * (which would be called anyway if we did not supply our own
 * callback), which creates an OCB (Open Context Block) for us.
 * In more complex resource managers, you will want to check if
 * the hardware is available, for example.
 */

int io_open (resmgr_context_t* ctp, io_open_t* msg,
        RESMGR_HANDLE_T* handle, void* extra)
{
    log_trace("io_open -> %s\n", _can_resmgr[ctp->id]->name);

    return (iofunc_open_default (ctp, msg, handle, extra));
}

/* Why we don't have any close callback? Because the default
 * function, iofunc_close_ocb_default(), does all we need in this
 * case: Free the ocb, update the time stamps etc. See the docs
 * for more info.
 */

int io_close_ocb (resmgr_context_t* ctp, void* reserved, RESMGR_OCB_T* _ocb) {
    log_trace("io_close_ocb -> %s\n", _can_resmgr[ctp->id]->name);

    iofunc_ocb_t* ocb = (iofunc_ocb_t*)_ocb;

    return iofunc_close_ocb_default(ctp, reserved, ocb);
}

/*
 *  io_read
 *
 *  At this point, the client has called the library read()
 *  function, and expects zero or more bytes.  Since this is
 *  the /dev/Null resource manager, we return zero bytes to
 *  indicate EOF -- no more bytes expected.
 */

/* The message that we received can be accessed via the
 * pointer *msg. A pointer to the OCB that belongs to this
 * read is the *ocb. The *ctp pointer points to a context
 * structure that is used by the resource manager framework
 * to determine whom to reply to, and more. */
int io_read (resmgr_context_t* ctp, io_read_t* msg, RESMGR_OCB_T* _ocb) {
    int status;

    iofunc_ocb_t* ocb = (iofunc_ocb_t*)_ocb;

    log_trace("io_read -> %s\n", _can_resmgr[ctp->id]->name);

    /* Here we verify if the client has the access
     * rights needed to read from our device */
    if ((status = iofunc_read_verify(ctp, msg, ocb, NULL)) != EOK) {
        return (status);
    }

    /* We check if our read callback was called because of
     * a pread() or a normal read() call. If pread(), we return
     * with an error code indicating that we don't support it.*/
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
        return (ENOSYS);
    }
    /* Here we set the number of bytes we will return. As we
     * are the null device, we will return 0 bytes. Normally,
     * here you would write the number of bytes you
     * are actually returning. */
    _IO_SET_READ_NBYTES(ctp, 0);

    /* The next line (commented) is used to tell the system how
     * large your buffer is in which you want to return your
     * data for the read() call. We don't set it here because
     * as a null device, we return 0 data. See the "time
     * resource manager" example for an actual use.
     */
    //  SETIOV( ctp->iov, buf, sizeof(buf));

    if (msg->i.nbytes > 0) {
        ocb->attr->flags |= IOFUNC_ATTR_ATIME;
    }

    /* We return 0 parts, because we are the null device.
     * Normally, if you return actual data, you would return at
     * least 1 part. A pointer to and a buffer length for 1 part
     * are located in the ctp structure.  */
    return (_RESMGR_NPARTS (0));
    /* What you return could also be several chunks of
     * data. In this case, you would return more
     * than 1 part. See the "time resource manager" example
     * on how to use this. */

}

/*
 *  io_write
 *
 *  At this point, the client has called the library write()
 *  function, and expects that our resource manager will write
 *  the number of bytes that have been specified to the device.
 *
 *  Since this is /dev/Null, all of the clients' writes always
 *  work -- they just go into Deep Outer Space.
 */

int io_write (resmgr_context_t* ctp, io_write_t* msg, RESMGR_OCB_T* _ocb) {
    int status;
    char *buf;

    iofunc_ocb_t* ocb = (iofunc_ocb_t*)_ocb;

    log_trace("io_write -> %s\n", _can_resmgr[ctp->id]->name);

    /* Check the access permissions of the client */
    if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK) {
        return (status);
    }

    /* Check if pwrite() or normal write() */
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
        return (ENOSYS);
    }

    /* Set the number of bytes successfully written for
     * the client. This information will be passed to the
     * client by the resource manager framework upon reply.
     * In this example, we just take the number of  bytes that
     * were sent to us and claim we successfully wrote them. */
    _IO_SET_WRITE_NBYTES (ctp, msg -> i.nbytes);
    printf("got write of %d bytes, data:\n", msg->i.nbytes);

    /* Here we print the data. This is a good example for the case
     * where you actually would like to do something with the data.
     */
    /* First check if our message buffer was large enough
     * to receive the whole write at once. If yes, print data.*/
    if( (msg->i.nbytes <= ctp->info.msglen - ctp->offset - sizeof(msg->i)) &&
            (ctp->info.msglen < ctp->msg_max_size))  { // space for NUL byte
        buf = (char *)(msg+1);
        buf[msg->i.nbytes] = '\0';
        printf("%s\n", buf );
    } else {
        /* If we did not receive the whole message because the
         * client wanted to send more than we could receive, we
         * allocate memory for all the data and use resmgr_msgread()
         * to read all the data at once. Although we did not receive
         * the data completely first, because our buffer was not big
         * enough, the data is still fully available on the client
         * side, because its write() call blocks until we return
         * from this callback! */
        buf = malloc( msg->i.nbytes + 1);
        resmgr_msgread( ctp, buf, msg->i.nbytes, sizeof(msg->i));
        buf[msg->i.nbytes] = '\0';
        printf("%s\n", buf );
        free(buf);
    }

    /* Finally, if we received more than 0 bytes, we mark the
     * file information for the device to be updated:
     * modification time and change of file status time. To
     * avoid constant update of the real file status information
     * (which would involve overhead getting the current time), we
     * just set these flags. The actual update is done upon
     * closing, which is valid according to POSIX. */
    if (msg->i.nbytes > 0) {
        ocb->attr->flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;
    }

    return (_RESMGR_NPARTS (0));
}

int io_devctl (resmgr_context_t* ctp, io_devctl_t* msg, RESMGR_OCB_T* _ocb) {
    int nbytes, status;

    iofunc_ocb_t* ocb = (iofunc_ocb_t*)_ocb;

    union data_t {
        // CAN_DEVCTL_DEBUG_INFO:
        //  no data

        // CAN_DEVCTL_DEBUG_INFO2:
        // CAN_DEVCTL_GET_MID:
        // CAN_DEVCTL_SET_MID:
        // CAN_DEVCTL_GET_MFILTER:
        // CAN_DEVCTL_SET_MFILTER:
        // CAN_DEVCTL_GET_PRIO:
        // CAN_DEVCTL_SET_PRIO:
        // CAN_DEVCTL_GET_TIMESTAMP:
        // CAN_DEVCTL_SET_TIMESTAMP:
        uint32_t                    u32;

        // CAN_DEVCTL_READ_CANMSG_EXT:
        // CAN_DEVCTL_WRITE_CANMSG_EXT:
        // CAN_DEVCTL_RX_FRAME_RAW_NOBLOCK:
        // CAN_DEVCTL_RX_FRAME_RAW_BLOCK:
        // CAN_DEVCTL_TX_FRAME_RAW:
        struct can_msg              canmsg;

        // CAN_DEVCTL_ERROR:
        struct can_devctl_error     err;

        // CAN_DEVCTL_GET_STATS:
        struct can_devctl_stats     stats;

        // CAN_DEVCTL_GET_INFO:
        struct can_devctl_info      info;

        // CAN_DEVCTL_SET_TIMING:
        struct can_devctl_timing    timing;
    } *data;

    log_trace("io_devctl -> %s\n", _can_resmgr[ctp->id]->name);

    /*
     Let common code handle DCMD_ALL_* cases.
     You can do this before or after you intercept devctls, depending
     on your intentions.  Here we aren't using any predefined values,
     so let the system ones be handled first. See note 2.
    */
    if ((status = iofunc_devctl_default(ctp, msg, ocb)) !=
         _RESMGR_DEFAULT) {
        return(status);
    }
    status = nbytes = 0;

    /* Get the data from the message. See Note 3. */
    data = _DEVCTL_DATA(msg->i);

    switch (msg->i.dcmd) {
    /*
     * Where applicable an associated canctl tool usage example is given as
     * comments in each case below.
     */
    case CAN_DEVCTL_GET_MID: // e.g. canctl -u1,rx0 -M
    {
        data->u32 = 0x0; // <- set MID
        nbytes = sizeof(data->u32);

        log_trace("CAN_DEVCTL_GET_MID: %x\n", data->u32);
        break;
    }
    case CAN_DEVCTL_SET_MID: // e.g. canctl -u1,rx1 -m 0x11CC0000
    {
        uint32_t mid = data->u32;
        nbytes = 0;

        log_trace("CAN_DEVCTL_SET_MID: %x\n", mid);
        break;
    }
    case CAN_DEVCTL_GET_MFILTER: // e.g. #canctl -u0,tx0 -F
    {
        data->u32 = 0x0; // set MFILTER
        nbytes = sizeof(data->u32);

        log_trace("CAN_DEVCTL_GET_MFILTER: %x\n", data->u32);
        break;
    }
    case CAN_DEVCTL_SET_MFILTER: // e.g. canctl -u0,tx0 -f 0x11CC0000
    {
        uint32_t mfilter = data->u32;
        nbytes = 0;

        log_trace("CAN_DEVCTL_SET_MFILTER: %x\n", mfilter);
        break;
    }
    case CAN_DEVCTL_GET_PRIO: // canctl -u1,tx1 -P
    {
        data->u32 = 0x0; // set PRIO
        nbytes = sizeof(data->u32);

        log_trace("CAN_DEVCTL_GET_PRIO: %x\n", data->u32);
        break;
    }
    case CAN_DEVCTL_SET_PRIO: // e.g. canctl -u1,tx1 -p 5
    {
        uint32_t prio = data->u32;
        nbytes = 0;

        log_trace("CAN_DEVCTL_SET_PRIO: %x\n", prio);
        break;
    }
    case CAN_DEVCTL_GET_TIMESTAMP: // e.g. canctl -u1 -T
    {
        data->u32 = 0x0; // set TIMESTAMP
        nbytes = sizeof(data->u32);

        log_trace("CAN_DEVCTL_GET_TIMESTAMP: %x\n", data->u32);
        break;
    }
    case CAN_DEVCTL_SET_TIMESTAMP: // e.g. canctl -u1 -t 0xAAAAAA
    {
        uint32_t ts = data->u32;
        nbytes = 0;

        log_trace("CAN_DEVCTL_SET_TIMESTAMP: %x\n", ts);
        break;
    }
    case CAN_DEVCTL_READ_CANMSG_EXT: // e.g. canctl -u1,rx0 -r # canctl not working with this option!
    {
        int i;

        struct can_msg* canmsg = dequeue(&_ocb->session->rx);

        if (canmsg != NULL) { // Could be a zero size rx queue, i.e. a tx queue
            data->canmsg = *canmsg;

            nbytes = sizeof(data->canmsg);

            log_trace("CAN_DEVCTL_READ_CANMSG_EXT; %s TS: %X [%s] %X [%d] " \
                      "%02X %02X %02X %02X %02X %02X %02X %02X\n",
                    _can_resmgr[ctp->id]->name,
                    canmsg->ext.timestamp,
                    canmsg->ext.is_extended_mid ? "EFF" : "SFF",
                    canmsg->mid,
                    canmsg->len,
                    canmsg->dat[0],
                    canmsg->dat[1],
                    canmsg->dat[2],
                    canmsg->dat[3],
                    canmsg->dat[4],
                    canmsg->dat[5],
                    canmsg->dat[6],
                    canmsg->dat[7]);
        }
        else {
            nbytes = 0;
        }

        break;
    }
    case CAN_DEVCTL_WRITE_CANMSG_EXT: // e.g. canctl -u0,rx0 -w0x22,1,0x55
    // case -2141977334: // TODO: raise to QNX
                      // Instead of the above macro, this command ID seems to be
                      // sent by canctl program. Perhaps canctl hasn't been
                      // recompiled after some changes made to 'sys/can_dcmd.h'
                      // by QNX? However the data is all scrambled and yields
                      // incorrect results. Thus custom verion of canctl needs to
                      // be made.
    {
        struct can_msg canmsg = data->canmsg;
        nbytes = 0;

        device_sessions_t* ds = &device_sessions[_ocb->resmgr->device->dev_id];

        int i;
        for (i = 0; i < ds->num_sessions; ++i) {
            if (enqueue(&ds->sessions[i].tx, &canmsg) != EOK) {
            }
        }

        log_trace("CAN_DEVCTL_WRITE_CANMSG_EXT; %s TS: %X [%s] %X [%d] " \
                  "%02X %02X %02X %02X %02X %02X %02X %02X\n",
                _can_resmgr[ctp->id]->name,
                canmsg.ext.timestamp,
                canmsg.ext.is_extended_mid ? "EFF" : "SFF",
                canmsg.mid,
                canmsg.len,
                canmsg.dat[0],
                canmsg.dat[1],
                canmsg.dat[2],
                canmsg.dat[3],
                canmsg.dat[4],
                canmsg.dat[5],
                canmsg.dat[6],
                canmsg.dat[7]);

        break;
    }
    case CAN_DEVCTL_ERROR: // e.g. canctl -u0,rx0 -e
    {
        data->err.drvr1 = 0x0; // set DRIVER ERROR 1
        data->err.drvr2 = 0x0; // set DRIVER ERROR 1
        data->err.drvr3 = 0x0; // set DRIVER ERROR 1
        data->err.drvr4 = 0x0; // set DRIVER ERROR 1
        nbytes = sizeof(data->err);

        log_trace("CAN_DEVCTL_ERROR: %x %x %x %x\n",
                data->err.drvr1,
                data->err.drvr2,
                data->err.drvr3,
                data->err.drvr4);
        break;
    }
    case CAN_DEVCTL_DEBUG_INFO: // e.g. canctl -d # Some strange behaviour INFO vs INFO2
    {
        nbytes = 0;

        log_err("Some debug info to stderr (& syslog)\n");
        break;
    }
    case CAN_DEVCTL_DEBUG_INFO2: // e.g. canctl -d # Some strange behaviour INFO vs INFO2
    {
        data->u32 = 0x0; // <- set INFO2
        nbytes = sizeof(data->u32);

        log_trace("CAN_DEVCTL_DEBUG_INFO2: %x\n", data->u32);
        break;
    }
    case CAN_DEVCTL_GET_STATS: // e.g. canctl -s
    {
        data->stats.transmitted_frames = 0x0; // set transmitted_frames

        nbytes = sizeof(data->stats);

        log_trace("CAN_DEVCTL_GET_STATS; %d\n",
                data->stats.transmitted_frames);
        break;
    }
    case CAN_DEVCTL_GET_INFO: // e.g. canctl -i
    {
        snprintf(data->info.description, 64, "driver: %s", detected_driver->name); // set description

        nbytes = sizeof(data->info);

        log_trace("CAN_DEVCTL_GET_INFO; %s\n",
                data->info.description);
        break;
    }
    case CAN_DEVCTL_SET_TIMING: // e.g. canctl -c 12,1,1,1,1 # TODO: come up with realistic example comment
    {
        struct can_devctl_timing timing = data->timing;
        nbytes = 0;

        log_trace("CAN_DEVCTL_SET_TIMING: %x\n", timing.ref_clock_freq);
        break;
    }
    case CAN_DEVCTL_RX_FRAME_RAW_NOBLOCK:
    case CAN_DEVCTL_RX_FRAME_RAW_BLOCK:
    case CAN_DEVCTL_TX_FRAME_RAW:
        return(ENOSYS);

    default:
        log_trace("io_devctl unknown command: %d\n", msg->i.dcmd);

        return(ENOSYS);
    }

    /* Clear the return message. Note that we saved our data past
       this location in the message. */
    memset(&msg->o, 0, sizeof(msg->o));

    /*
     If you wanted to pass something different to the return
     field of the devctl() you could do it through this member.
     See note 5.
    */
    msg->o.ret_val = status;

    /* Indicate the number of bytes and return the message */
    msg->o.nbytes = nbytes;
    return(_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + nbytes));
}
