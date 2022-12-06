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
#include <sys/iofunc.h>
#include <sys/resmgr.h>
#include <sys/dispatch.h>
#include <sys/can_dcmd.h>
#include <sys/neutrino.h>

#include <resmgr.h>
#include <config.h>
#include <pci.h>

struct net_device* device[MAX_DEVICES];


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

static dispatch_t *dpp[MAX_DEVICES][MAX_MAILBOXES*2];
static resmgr_attr_t rattr[MAX_DEVICES][MAX_MAILBOXES*2];
static dispatch_context_t *ctp[MAX_DEVICES][MAX_MAILBOXES*2];
static iofunc_attr_t ioattr[MAX_DEVICES][MAX_MAILBOXES*2];

static int pathID[MAX_DEVICES][MAX_MAILBOXES*2];
static pthread_attr_t attr[MAX_DEVICES][MAX_MAILBOXES*2];

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

void* receive_loop (void*  arg);

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
        iofunc_func_init (_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                _RESMGR_IO_NFUNCS, &io_funcs);

        /* Now we override the default function pointers with
         * some of our own coded functions: */
        connect_funcs.open = io_open;
        io_funcs.close_ocb = io_close_ocb;
        io_funcs.read = io_read;
        io_funcs.write = io_write;
        io_funcs.devctl = io_devctl;
    }

    char name[MAX_NAME_SIZE];
    int i, j;

    for (i = 0; i < 1; ++i) { // default number of channels
        for (j = 0; j < 2; ++j) { // 2 for rx & tx
            /* Allocate and initialize a dispatch structure for use
             * by our main loop. This is for the resource manager
             * framework to use. It will receive messages for us,
             * analyze the message type integer and call the matching
             * handler callback function (i.e. io_open, io_read, etc.) */
            dpp[dev->dev_id][i*2+j] = dispatch_create ();
            if (dpp == NULL) {
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
            memset (&rattr[dev->dev_id][i*2+j], 0, sizeof(resmgr_attr_t));

            if (j == 0) {
                snprintf(name, MAX_NAME_SIZE, "/dev/can%d/rx%d", dev->dev_id, i);
            }
            else {
                snprintf(name, MAX_NAME_SIZE, "/dev/can%d/tx%d", dev->dev_id, i);
            }

            iofunc_attr_init(&ioattr[dev->dev_id][i*2+j], S_IFCHR | 0666, NULL, NULL);

            pathID[dev->dev_id][i*2+j] = resmgr_attach (dpp[dev->dev_id][i*2+j], &rattr[dev->dev_id][i*2+j], name,
                                     _FTYPE_ANY, 0,
                                     &connect_funcs, &io_funcs, &ioattr[dev->dev_id][i*2+j]);

            if (pathID[dev->dev_id][i*2+j] == -1) {
                log_err("couldn't attach pathname: %s\n", strerror(errno));

                return -1;
            }

            /* Now we allocate some memory for the dispatch context
             * structure, which will later be used when we receive
             * messages. */
            ctp[dev->dev_id][i*2+j] = dispatch_context_alloc(dpp[dev->dev_id][i*2+j]);

            pthread_attr_init(&attr[dev->dev_id][i*2+j]);
            pthread_attr_setdetachstate(&attr[dev->dev_id][i*2+j], PTHREAD_CREATE_DETACHED);
            pthread_create(NULL, &attr[dev->dev_id][i*2+j], &receive_loop, ctp[dev->dev_id][i*2+j]);

            log_trace("resmgr_attach -> %d\n", pathID[dev->dev_id][i*2+j]);
		    dev->flags |= IFF_UP;
        }
    }

    return 0;
}

void unregister_netdev(struct net_device *dev) {
    log_trace("unregister_netdev: %s\n", dev->name);

    if (dev->netdev_ops->ndo_stop(dev)) {
        log_err("internal error; ndo_stop failure\n");
    }

    dev->flags &= ~IFF_UP;
}


/*
 * Resource Manager
 *
 * Message thread and callback functions for connection and IO
 */

void* receive_loop (void*  arg) {
    dispatch_context_t* ctp = (dispatch_context_t*)arg;

    /* Done! We can now go into our "receive loop" and wait
     * for messages. The dispatch_block() function is calling
     * MsgReceive() under the covers, and receives for us.
     * The dispatch_handler() function analyzes the message
     * for us and calls the appropriate callback function. */
    while (1) {
        if ((ctp = dispatch_block (ctp)) == NULL) {
            log_err("dispatch_block failed: %s\n", strerror(errno));

            return (0);
        }
        /* Call the correct callback function for the message
         * received. This is a single-threaded resource manager,
         * so the next request will be handled only when this
         * call returns. Consult our documentation if you want
         * to create a multi-threaded resource manager. */
        dispatch_handler(ctp);
    }

    return (0);
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

int
io_open (resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle, void *extra)
{
    log_trace("in io_open -> %d\n", ctp->id);

    return (iofunc_open_default (ctp, msg, handle, extra));
}

/* Why we don't have any close callback? Because the default
 * function, iofunc_close_ocb_default(), does all we need in this
 * case: Free the ocb, update the time stamps etc. See the docs
 * for more info.
 */

int io_close_ocb (resmgr_context_t *ctp, void *reserved, RESMGR_OCB_T *ocb) {
    log_trace("in io_close_ocb -> %d\n", ctp->id);

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
int
io_read (resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb)
{
    int status;

    log_trace("in io_read -> %d\n", ctp->id);

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

int
io_write (resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb)
{
    int status;
    char *buf;

    log_trace("in io_write -> %d\n", ctp->id);

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

int
io_devctl (resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
    int nbytes, status;

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

    log_trace("in io_devctl -> %d\n", ctp->id);

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

        data->canmsg.mid = 0x0; // set MID
        data->canmsg.ext.timestamp = 0x0; // set TIMESTAMP
        data->canmsg.len = 0x0; // set LEN

        for (i = 0; i < CAN_MSG_DATA_MAX; ++i) {
            data->canmsg.dat[i] = i; // Set DAT
        }

        nbytes = sizeof(data->canmsg);

        log_trace("CAN_DEVCTL_READ_CANMSG_EXT; MID: %x, TS: %x, LEN: %x, DAT: %2x %2x %2x %2x %2x %2x %2x %2x\n",
                data->canmsg.mid,
                data->canmsg.ext.timestamp,
                data->canmsg.len,
                data->canmsg.dat[0],
                data->canmsg.dat[1],
                data->canmsg.dat[2],
                data->canmsg.dat[3],
                data->canmsg.dat[4],
                data->canmsg.dat[5],
                data->canmsg.dat[6],
                data->canmsg.dat[7]);
        break;
    }
    case CAN_DEVCTL_WRITE_CANMSG_EXT: // e.g. canctl -u0,rx0 -w0x22,1,0x55 # canctl not working with this option!
    {
        struct can_msg canmsg = data->canmsg;
        nbytes = 0;

        log_trace("CAN_DEVCTL_WRITE_CANMSG_EXT; MID: %x, TS: %x, LEN: %x, DAT: %2x %2x %2x %2x %2x %2x %2x %2x\n",
                canmsg.mid,
                canmsg.ext.timestamp,
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
    default:
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
