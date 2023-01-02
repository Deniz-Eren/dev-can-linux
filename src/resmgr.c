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

#include <resmgr.h>
#include <config.h>
#include <pci.h>
#include <dev-can-linux/commands.h>

static can_resmgr_t* root_resmgr = NULL;

static pthread_mutex_t rx_mutex = PTHREAD_MUTEX_INITIALIZER;

IOFUNC_OCB_T* can_ocb_calloc (resmgr_context_t* ctp, IOFUNC_ATTR_T* attr);
void can_ocb_free (IOFUNC_OCB_T* ocb);

/*
 * Resource manager POSIX callback functions for clients to call.
 */
int io_close_ocb (resmgr_context_t* ctp, void* reserved,   RESMGR_OCB_T* ocb);
int io_read      (resmgr_context_t* ctp, io_read_t*   msg, RESMGR_OCB_T* ocb);
int io_write     (resmgr_context_t* ctp, io_write_t*  msg, RESMGR_OCB_T* ocb);
int io_devctl    (resmgr_context_t* ctp, io_devctl_t* msg, RESMGR_OCB_T* ocb);
int io_open      (resmgr_context_t* ctp, io_open_t*   msg,
                    RESMGR_HANDLE_T* handle, void* extra);

void* rx_loop (void* arg);

#if CONFIG_QNX_RESMGR_SINGLE_THREAD == 1
void* dispatch_receive_loop (void* arg);
#endif

/*
 * Managing blocking clients
 */
typedef struct {
    uint16_t id;
    int rcvid;
} msg_again_t;

int msg_again_callback(
        message_context_t* ctp, int type, unsigned flags, void* handle);

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

    log_trace("register_netdev: %s\n", dev->name);

    struct user_dev_setup user = {
            .set_bittiming = true,
            .bittiming = { .bitrate = 250000 }
    };
    dev->resmgr_ops->changelink(dev, &user);

    if (dev->netdev_ops->ndo_open(dev)) {
        return -1;
    }

    int id = dev->dev_id;

    const queue_attr_t tx_attr = { .size = 15 };

    device_session_t* device_session;
    if ((device_session = create_device_session(dev, &tx_attr)) != NULL) {
    }

    static int io_created = 0;

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
            can_resmgr_t* resmgr = (can_resmgr_t*)malloc(sizeof(can_resmgr_t));

            store_resmgr(&root_resmgr, resmgr);

            resmgr->device_session = device_session;

#if CONFIG_QNX_RESMGR_THREAD_POOL == 1
            /* initialize dispatch interface */
            resmgr->dispatch = dispatch_create_channel(-1, DISPATCH_FLAG_NOLOCK);
#elif CONFIG_QNX_RESMGR_SINGLE_THREAD == 1
            /* Allocate and initialize a dispatch structure for use
             * by our main loop. This is for the resource manager
             * framework to use. It will receive messages for us,
             * analyze the message type integer and call the matching
             * handler callback function (i.e. io_open, io_read, etc.) */
            resmgr->dispatch = dispatch_create();
#endif

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
                        "/dev/can%d/rx%d", id, i );

                resmgr->channel_type = RX_CHANNEL;
            }
            else {
                snprintf( resmgr->name, MAX_NAME_SIZE,
                        "/dev/can%d/tx%d", id, i );

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

            resmgr->id = resmgr_attach(
                    resmgr->dispatch, &resmgr->resmgr_attr, resmgr->name,
                    _FTYPE_ANY, 0, &connect_funcs, &io_funcs,
                    &resmgr->iofunc_attr );

            if (resmgr->id == -1) {
                log_err("resmgr_attach fail: %s\n", strerror(errno));

                return -1;
            }

            resmgr->latency_limit_ms = 0;

            /* Attach a callback (handler) for two message types */
            if (message_attach( resmgr->dispatch, NULL, _IO_MAX + 1,
                        _IO_MAX + 1, msg_again_callback, NULL ) == -1)
            {
                log_err("message_attach() failed: %s\n", strerror(errno));

                return -1;
            }

#if CONFIG_QNX_RESMGR_THREAD_POOL == 1
            /* initialize thread pool attributes */
            memset(&resmgr->thread_pool_attr, 0, sizeof(thread_pool_attr_t));
            resmgr->thread_pool_attr.handle = resmgr->dispatch;
            resmgr->thread_pool_attr.context_alloc = dispatch_context_alloc;
            resmgr->thread_pool_attr.block_func = dispatch_block; 
            resmgr->thread_pool_attr.unblock_func = dispatch_unblock;
            resmgr->thread_pool_attr.handler_func = dispatch_handler;
            resmgr->thread_pool_attr.context_free = dispatch_context_free;
            resmgr->thread_pool_attr.lo_water = 4;
            resmgr->thread_pool_attr.hi_water = 8;
            resmgr->thread_pool_attr.increment = 1;
            resmgr->thread_pool_attr.maximum = 50;

            /* allocate a thread pool handle */
            if (( resmgr->thread_pool =
                    thread_pool_create(&resmgr->thread_pool_attr, 0) ) == NULL)
            {
                log_err( "thread_pool_create fail: " \
                         "Unable to initialize thread pool.\n");

                return -1;
            }

            /* Start the threads. This function doesn't return. */
            thread_pool_start(resmgr->thread_pool);

#elif CONFIG_QNX_RESMGR_SINGLE_THREAD == 1
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
#endif

            log_trace("resmgr_attach -> %d\n", resmgr->id);

		    dev->flags |= IFF_UP;
        }
    }

    return 0;
}

void unregister_netdev(struct net_device *dev) {
    int i, j;

    log_trace("unregister_netdev: %s\n", dev->name);

    int id = dev->dev_id;

    dev->flags &= ~IFF_UP;

    if (dev->netdev_ops->ndo_stop(dev)) {
        log_err("internal error; ndo_stop failure\n");
    }

    can_resmgr_t** location = &root_resmgr;

    while (*location != NULL) {
        can_resmgr_t* resmgr = *location;

        if (resmgr->device_session->device != dev) {
            location = &(*location)->next;

            continue;
        }

        destroy_device_session(resmgr->device_session);

        if (resmgr->prev && resmgr->next) {
            resmgr->prev->next = resmgr->next;
            resmgr->next->prev = resmgr->prev;
        }
        else if (resmgr->prev) {
            resmgr->prev->next = NULL;
        }
        else if (resmgr->next) {
            resmgr->next->prev = NULL;

            /* Since this node does not have a previous node, it must be the root
             * node. Therefore when it is destroyed the new root node must then be
             * the next node. */
            root_resmgr = resmgr->next;
        }
        else {
            /* Since this node does not have a previous or a next node, it must be
             * the root and last remaining node. Therefore when it is destroyed the
             * root node must be set to NULL. */

            root_resmgr = NULL;
        }

#if CONFIG_QNX_RESMGR_THREAD_POOL == 1
        if (thread_pool_destroy(resmgr->thread_pool) == -1) {
            log_err( "internal error; thread_pool_destroy failure " \
                     "(%d, %d)\n", i, j );
        }
#endif

        if (resmgr_detach(resmgr->dispatch, resmgr->id, 0) == -1) {
            log_err( "internal error; resmgr_detach failure (%d, %d)\n",
                    i, j );
        }

#if CONFIG_QNX_RESMGR_SINGLE_THREAD == 1
        pthread_join(resmgr->dispatch_thread, NULL);
#endif

#if CONFIG_QNX_RESMGR_SINGLE_THREAD == 1
        // TODO: investigate if dispatch_context_free() is needed and why
        //       we get SIGSEGV when following code is uncommented.
        //dispatch_context_free(resmgr->dispatch_context);
#endif

        if (dispatch_destroy(resmgr->dispatch) == -1) {
            log_err( "internal error; dispatch_destroy failure (%d, %d)\n",
                    i, j );
        }

        free(resmgr);
        break;
    }
}


IOFUNC_OCB_T* can_ocb_calloc (resmgr_context_t* ctp, IOFUNC_ATTR_T* attr) {
    can_resmgr_t* resmgr = get_resmgr(&root_resmgr, ctp->id);

    log_trace("can_ocb_calloc -> %s (id: %d)\n", resmgr->name, ctp->id);

    struct can_ocb* ocb;

    if (!(ocb = calloc(1, sizeof(*ocb)))) {
        return NULL;
    }

    ocb->resmgr = resmgr;

    device_session_t* ds = resmgr->device_session;
    struct net_device* device = ds->device;

    queue_attr_t rx_attr = { .size = 1024 };

    if (ocb->resmgr->channel_type != RX_CHANNEL) {
        rx_attr.size = 0;
    }

    if ((ocb->session = create_client_session(device, &rx_attr)) == NULL) {
        log_err("create_client_session failed: %s\n", ocb->resmgr->name);
    }

    // Every rx session has it's own rx thread to call resmgr_msg_again()
    if (ocb->resmgr->channel_type == RX_CHANNEL) {
        ocb->rx.queue = &ocb->session->rx_queue;
        ocb->rx.rcvid = -1;

        if (pthread_cond_init(&ocb->rx.cond, NULL) != EOK) {
        }

        pthread_attr_init(&ocb->rx.thread_attr);
        pthread_attr_setdetachstate(
                &ocb->rx.thread_attr, PTHREAD_CREATE_DETACHED );

        pthread_create( &ocb->rx.thread, &ocb->rx.thread_attr,
                &rx_loop, ocb );
    }

    return ocb;
}

void can_ocb_free (IOFUNC_OCB_T* ocb) {
    log_trace("can_ocb_free -> %s\n", ocb->resmgr->name);

    pthread_mutex_lock(&rx_mutex);
    ocb->rx.queue = NULL;
    pthread_cond_signal(&ocb->rx.cond);
    pthread_mutex_unlock(&rx_mutex);

    destroy_client_session(ocb->session);

    // Wait for rx thread to exit
    if (ocb->resmgr->channel_type == RX_CHANNEL) {
        pthread_join(ocb->rx.thread, NULL);

        pthread_mutex_lock(&rx_mutex);
        if (ocb->rx.rcvid != -1) {
            // Don't leave any blocking clients hanging
            MsgError(ocb->rx.rcvid, EBADF);
            ocb->rx.rcvid = -1;
        }
        pthread_mutex_unlock(&rx_mutex);
    }

    free(ocb);
}

int msg_again_callback (message_context_t* ctp, int type, unsigned flags,
        void* handle)
{
    msg_again_t* msg_again = (msg_again_t*)ctp->msg;

    // function resmgr_msg_again() seems to modify the ctp->rcvid value
    // so let's save it and restore it afer the call
    int rcvid_save = ctp->rcvid;

    if (msg_again->rcvid != -1) {
        if (resmgr_msg_again(ctp, msg_again->rcvid) == -1) {
        }
    }

    ctp->rcvid = rcvid_save;

    MsgReply(ctp->rcvid, EOK, NULL, 0);
    return 0;
}

void* rx_loop (void* arg) {
    struct can_ocb* ocb = (struct can_ocb*)arg;

    device_session_t* ds = ocb->resmgr->device_session;
    struct net_device* device = ds->device;

    int coid;
    msg_again_t msg_again = { .id = _IO_MAX + 1 };

    /* Connect to our channel */
    if ((coid = message_connect(
                    ocb->resmgr->dispatch, MSG_FLAG_SIDE_CHANNEL )) == -1)
    {
        log_err("rx_loop exit: Unable to attach to channel.\n");

        return NULL;
    }

    while (1) {
        pthread_mutex_lock(&rx_mutex);

        while ((device->flags & IFF_UP)
                && (ocb->rx.queue != NULL)
                && (ocb->rx.rcvid == -1))
        {
            pthread_cond_wait(&ocb->rx.cond, &rx_mutex);
        }

        pthread_mutex_unlock(&rx_mutex);

        if (!(device->flags & IFF_UP) || (ocb->rx.queue == NULL)) {
            log_trace("rx_loop exit\n");

            pthread_exit(NULL);
        }

        if (ocb->rx.rcvid != -1) {
            struct can_msg* canmsg = dequeue_peek(ocb->rx.queue);

            pthread_mutex_lock(&rx_mutex);
            msg_again.rcvid = ocb->rx.rcvid;
            pthread_mutex_unlock(&rx_mutex);

            if (ocb->rx.queue != NULL && msg_again.rcvid != -1) {
                if (MsgSend(coid, &msg_again, sizeof(msg_again_t), NULL, 0)
                        == -1)
                {
                    log_err("rx_loop MsgSend error: %s\n", strerror(errno));
                }
            }
        }
    }

    return NULL;
}

#if CONFIG_QNX_RESMGR_SINGLE_THREAD == 1
/*
 * Resource Manager
 *
 * Message thread and callback functions for connection and IO
 */

void* dispatch_receive_loop (void* arg) {
    can_resmgr_t* resmgr = (can_resmgr_t*)arg;

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
#endif

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
    can_resmgr_t* resmgr = get_resmgr(&root_resmgr, ctp->id);

    log_trace( "io_open -> %s (id: %d, rcvid: %d)\n",
            resmgr->name, ctp->id, ctp->rcvid);

    return (iofunc_open_default (ctp, msg, handle, extra));
}

/* Why we don't have any close callback? Because the default
 * function, iofunc_close_ocb_default(), does all we need in this
 * case: Free the ocb, update the time stamps etc. See the docs
 * for more info.
 */

int io_close_ocb (resmgr_context_t* ctp, void* reserved, RESMGR_OCB_T* _ocb) {
    can_resmgr_t* resmgr = get_resmgr(&root_resmgr, ctp->id);

    log_trace("io_close_ocb -> id: %d\n", ctp->id);

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

    log_trace("io_read -> id: %d\n", ctp->id);

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

    log_trace("io_write -> id: %d\n", ctp->id);

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

        // EXT_CAN_DEVCTL_SET_LATENCY_LIMIT_MS
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

    log_trace("io_devctl -> id: %d\n", ctp->id);

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

    /*
     * Where applicable an associated canctl tool usage example is given as
     * comments in each case below.
     */
    switch (msg->i.dcmd) {
    /*
     * Extended devctl() commands; these are in addition to the standard
     * QNX dev-can-* driver protocol commands
     */
    case EXT_CAN_DEVCTL_SET_LATENCY_LIMIT_MS:
    {
        uint32_t can_latency_limit = data->u32;
        nbytes = 0;

        _ocb->resmgr->latency_limit_ms = can_latency_limit;

        log_trace("EXT_CAN_DEVCTL_SET_LATENCY_LIMIT_MS: %dms (%s)\n",
                can_latency_limit,
                _ocb->resmgr->name);

        break;
    }
    /*
     * Standard QNX dev-can-* driver protocol commands
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

        if (_ocb->session->rx_queue.attr.size == 0) {
            nbytes = 0;

            break;
        }

        struct can_msg* canmsg =
            dequeue_nonblock( &_ocb->session->rx_queue,
                    _ocb->resmgr->latency_limit_ms );

        if (canmsg != NULL) { // Could be a zero size rx queue, i.e. a tx queue
            data->canmsg = *canmsg;

            nbytes = sizeof(data->canmsg);

            log_trace("CAN_DEVCTL_READ_CANMSG_EXT; %s TS: %u [%s] %X [%d] " \
                      "%02X %02X %02X %02X %02X %02X %02X %02X\n",
                    _ocb->resmgr->name,
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

            pthread_mutex_lock(&rx_mutex);
            _ocb->rx.rcvid = -1;
            pthread_mutex_unlock(&rx_mutex);
        }
        else {
            pthread_mutex_lock(&rx_mutex);
            _ocb->rx.rcvid = ctp->rcvid;

            pthread_cond_signal(&_ocb->rx.cond);
            pthread_mutex_unlock(&rx_mutex);

            return _RESMGR_NOREPLY;
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

        enqueue(&_ocb->resmgr->device_session->tx_queue, &canmsg);

        log_trace("CAN_DEVCTL_WRITE_CANMSG_EXT; %s TS: %u [%s] %X [%d] " \
                  "%02X %02X %02X %02X %02X %02X %02X %02X\n",
                _ocb->resmgr->name,
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
        u32 debug = data->u32;
        nbytes = 0;

        log_trace("CAN_DEVCTL_DEBUG_INFO2: %x\n", debug);
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
