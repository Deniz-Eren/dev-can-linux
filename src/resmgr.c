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

IOFUNC_OCB_T* can_ocb_calloc (resmgr_context_t* ctp, IOFUNC_ATTR_T* attr);
void can_ocb_free (IOFUNC_OCB_T* ocb);

/*
 * Resource manager POSIX callback functions for clients to call.
 */
int io_close_ocb (resmgr_context_t* ctp, void* reserved,   RESMGR_OCB_T* ocb);
int io_read      (resmgr_context_t* ctp, io_read_t*   msg, RESMGR_OCB_T* ocb);
int io_write     (resmgr_context_t* ctp, io_write_t*  msg, RESMGR_OCB_T* ocb);
int io_unblock   (resmgr_context_t* ctp, io_pulse_t* msg,  RESMGR_OCB_T* ocb);
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

void create_user_dev_setup ( struct can_devctl_timing* timing, u32 freq,
        struct user_dev_setup* user )
{
    int TQ = timing->bit_rate_prescaler * BILLION / freq;
    int one_bit = BILLION / timing->ref_clock_freq / TQ;
    int prop_seg = one_bit
                - timing->sync_jump_width
                - timing->time_segment_1
                - timing->time_segment_2;

    struct user_dev_setup user1 = {
            .set_bittiming = true,
            .bittiming = {
                .bitrate = 0, // set to zero to invoke it's calculation
                .sample_point = (timing->sync_jump_width
                                + prop_seg
                                + timing->time_segment_1)*10/one_bit,
                .tq = TQ,
                .prop_seg = prop_seg,
                .sjw = timing->sync_jump_width,
                .phase_seg1 = timing->time_segment_1,
                .phase_seg2 = timing->time_segment_2,
                .brp = timing->bit_rate_prescaler
            }
    };

    struct user_dev_setup user2 = {
            .set_bittiming = true,
            .bittiming = {
                .bitrate = timing->ref_clock_freq,
                .sample_point = 0,
                .tq = 0,
                .prop_seg = 0,
                .sjw = 0,
                .phase_seg1 = 0,
                .phase_seg2 = 0,
                .brp = 0
            }
    };

    if (timing->bit_rate_prescaler == 0) {
        memcpy(user, &user2, sizeof(struct user_dev_setup));
    }
    else {
        memcpy(user, &user1, sizeof(struct user_dev_setup));
    }
}


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

void log_trace_bittiming_info (struct net_device* device);


int register_netdev (struct net_device* dev) {
    if (dev == NULL) {
        log_err("register_netdev failed: invalid net_device\n");
        return -1;
    }

    if (probe_driver_selection == NULL) {
        log_err("register_netdev failed: invalid probe_driver_selection\n");
        return -1;
    }

    int id = next_device_id++;

    snprintf( dev->name, IFNAMSIZ, "%s-can%d",
            probe_driver_selection->driver->name, dev->dev_id );

    log_trace("register_netdev: %s\n", dev->name);

    struct user_dev_setup user = {
            .set_restart_ms = true,
            .restart_ms = optr_restart_ms,
            .set_bittiming = true,
            .bittiming = { .bitrate = 250000 }
    };

    if (id < num_optb_configs) {
        if (id == optb_config[id].id && optb_config[id].bitrate != 0) {
            if (optb_config[id].btr0 != 0 || optb_config[id].btr1 != 0) {
                user.set_bittiming = false;
                user.set_btr = true;
                user.bittiming.bitrate = optb_config[id].bitrate;
                user.can_btr.btr0 = optb_config[id].btr0;
                user.can_btr.btr1 = optb_config[id].btr1;
            }
            else if (optb_config[id].bitrate != 0) {
                struct can_priv* priv = netdev_priv(dev);
                struct can_devctl_timing timing;

                user.bittiming.bitrate = optb_config[id].bitrate;
                timing.ref_clock_freq = optb_config[id].bitrate;
                timing.sync_jump_width = optb_config[id].sjw;
                timing.time_segment_1 = optb_config[id].phase_seg1;
                timing.time_segment_2 = optb_config[id].phase_seg2;
                timing.bit_rate_prescaler = optb_config[id].bprm;

                create_user_dev_setup(&timing, priv->clock.freq, &user);
            }
        }
    }

    if (opts) { /* silent mode; disable all CAN-bus TX capability */
        user.set_ctrlmode = true;
        user.ctrlmode.flags = user.ctrlmode.mask = CAN_CTRLMODE_LISTENONLY;
    }

    int err;
    if ((err = dev->resmgr_ops->changelink(dev, &user, NULL)) != 0) {
        log_err("register_netdev: changelink failed: %d\n", err);

        return -1;
    }

    dev->device_session = NULL;

    log_trace_bittiming_info(dev);

    if (dev->netdev_ops->ndo_open(dev)) {
        log_err("register_netdev failed: ndo_open error\n");

        return -1;
    }

    const queue_attr_t tx_attr = { .size = 15 };

    device_session_t* device_session;
    if ((device_session = create_device_session(dev, &tx_attr)) != NULL) {
    }

    dev->device_session = device_session;

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
        io_funcs.unblock = io_unblock;
        io_funcs.write = io_write;
        io_funcs.devctl = io_devctl;
    }

    int num_channels[2] = {
        DEFAULT_NUM_RX_CHANNELS,
        DEFAULT_NUM_TX_CHANNELS
    };

    int is_extended_mid = 0; // Default is standard MIDs

    if (optx) {
        is_extended_mid = 1; // Change to extended if driver option is given
    }

    if (id < num_optu_configs) {
        if (id == optu_config[id].id) {
            num_channels[0] = optu_config[id].num_rx_channels;
            num_channels[1] = optu_config[id].num_tx_channels;

            if (optu_config[id].is_extended_mid != -1) {
                // Override driver option if individual device option is given
                is_extended_mid = optu_config[id].is_extended_mid;
            }
        }
    }

    int i, j;
    for (j = 0; j < 2; ++j) { // 2 for rx & tx
        for (i = 0; i < num_channels[j]; ++i) { // number of channels
            can_resmgr_t* resmgr = (can_resmgr_t*)malloc(sizeof(can_resmgr_t));

            store_resmgr(&root_resmgr, resmgr);

            resmgr->device_session = device_session;
            resmgr->driver_selection = probe_driver_selection;

            // Property is_extended_mid is only applicable for read and write
            // functions not used for direct devctl send/receive functionality.
            resmgr->is_extended_mid = is_extended_mid;

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
            resmgr->resmgr_attr.nparts_max = 1;
            resmgr->resmgr_attr.msg_max_size = 2048;

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

            log_info("%s -> %s\n", dev->name, resmgr->name);

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

            resmgr->latency_limit_ms = 0;   /* Maximum allowed latency in
                                               milliseconds */
            resmgr->mid = 0x00000000;       /* CAN message identifier */
            resmgr->mfilter = 0xFFFFFFFF;   /* CAN message filter */
            resmgr->prio = 0;               /* CAN priority - not used */
            resmgr->shutdown = 0;

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
    if (dev == NULL) {
        log_err("unregister_netdev failed: invalid net_device\n");
        return;
    }

    log_trace("unregister_netdev: %s\n", dev->name);

    dev->flags &= ~IFF_UP;

    if (dev->netdev_ops) {
        if (dev->netdev_ops->ndo_stop(dev)) {
            log_err("internal error; ndo_stop failure\n");
        }
    }

    can_resmgr_t* location = root_resmgr;

    while (location != NULL) {
        can_resmgr_t* resmgr = location;

        char name[MAX_NAME_SIZE];
        strncpy(name, resmgr->name, MAX_NAME_SIZE);

        if (resmgr->device_session->device != dev) {
            location = location->next;

            continue;
        }

        resmgr->shutdown = 1;

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

            /* Since this node does not have a previous node, it must be the
             * root node. Therefore when it is destroyed the new root node must
             * then be the next node. */
            root_resmgr = resmgr->next;
        }
        else {
            /* Since this node does not have a previous or a next node, it must
             * be the root and last remaining node. Therefore when it is
             * destroyed the root node must be set to NULL. */

            root_resmgr = NULL;
        }

#if CONFIG_QNX_RESMGR_THREAD_POOL == 1
        if (thread_pool_destroy(resmgr->thread_pool) == -1) {
            log_err( "internal error; thread_pool_destroy failure (%s)\n",
                    name );
        }
#endif

        if (resmgr_detach(resmgr->dispatch, resmgr->id, 0) == -1) {
            log_err("internal error; resmgr_detach failure (%s)\n", name);
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
            log_err( "internal error; dispatch_destroy failure (%s)\n",
                    name );
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

    if ((ocb->session =
            create_client_session( device, &rx_attr,
                &resmgr->mid,
                &resmgr->mfilter,
                &resmgr->prio )) == NULL)
    {
        log_err("create_client_session failed: %s\n", ocb->resmgr->name);
    }

    ocb->rx.read_size = 0;
    ocb->rx.read_buffer = NULL;
    ocb->rx.nbytes = 0;
    ocb->rx.offset = 0;

    // Every rx session has it's own rx thread to call resmgr_msg_again()
    if (ocb->resmgr->channel_type == RX_CHANNEL) {
        ocb->rx.queue = &ocb->session->rx_queue;
        ocb->rx.blocked_clients = NULL;

        ocb->rx.read_size = 4096;
        ocb->rx.read_buffer = malloc(ocb->rx.read_size);

        int result;
        if ((result = pthread_mutex_init(&ocb->rx.mutex, NULL)) != EOK) {
            log_err("can_ocb_calloc pthread_mutex_init failed: %d\n",
                    result);

            destroy_client_session(ocb->session);
            free(ocb);

            return NULL;
        }

        if ((result = pthread_cond_init(&ocb->rx.cond, NULL)) != EOK) {
            log_err("create_device_session pthread_cond_init failed: %d\n",
                    result);

            pthread_mutex_destroy(&ocb->rx.mutex);
            destroy_client_session(ocb->session);
            free(ocb);

            return NULL;
        }

        pthread_create(&ocb->rx.thread, NULL, &rx_loop, ocb);
    }

    return ocb;
}

void can_ocb_free (IOFUNC_OCB_T* ocb) {
    log_trace("can_ocb_free -> %s\n", ocb->resmgr->name);

    pthread_mutex_lock(&ocb->rx.mutex);
    ocb->rx.queue = NULL;
    pthread_cond_signal(&ocb->rx.cond);
    pthread_mutex_unlock(&ocb->rx.mutex);

    if (ocb->session) {
        destroy_client_session(ocb->session);
        ocb->session = NULL;
    }

    // Wait for rx thread to exit
    if (ocb->resmgr->channel_type == RX_CHANNEL) {
        pthread_join(ocb->rx.thread, NULL);

        pthread_mutex_lock(&ocb->rx.mutex);

        // Don't leave any blocking clients hanging
        blocked_client_t *client = ocb->rx.blocked_clients;

        while (client != NULL) {
            MsgError(client->rcvid, EBADF);

            client = client->next;
        }

        free_all_blocked_clients(&ocb->rx.blocked_clients);
        pthread_mutex_unlock(&ocb->rx.mutex);
    }

    pthread_mutex_lock(&ocb->rx.mutex);
    pthread_mutex_destroy(&ocb->rx.mutex);
    pthread_cond_destroy(&ocb->rx.cond);

    ocb->rx.read_size = 0;
    if (ocb->rx.read_buffer) {
       free(ocb->rx.read_buffer);
       ocb->rx.read_buffer = NULL;
    }

    free(ocb);

    // Notice we never unlocked the mutex, since we know the dequeue() is not
    // waiting and we are in the process of destroying the session.
    // No need: pthread_mutex_unlock(&ocb->rx.mutex);
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

    can_resmgr_t* resmgr = ocb->resmgr;

    int coid;
    msg_again_t msg_again = { .id = _IO_MAX + 1 };

    /* Connect to our channel */
    if ((coid = message_connect(
                    ocb->resmgr->dispatch, MSG_FLAG_SIDE_CHANNEL )) == -1)
    {
        log_err("rx_loop exit: Unable to attach to channel.\n");

        pthread_exit(NULL);
    }

    while (1) {
        int status = -1;

        pthread_mutex_lock(&ocb->rx.mutex);

        while ((!resmgr->shutdown)
                && (ocb->rx.queue != NULL)
                && (ocb->rx.blocked_clients == NULL))
        {
            pthread_cond_wait(&ocb->rx.cond, &ocb->rx.mutex);
        }

        pthread_mutex_unlock(&ocb->rx.mutex);

        if (resmgr->shutdown || (ocb->rx.queue == NULL)) {
            log_trace("rx_loop exit\n");

            ConnectDetach(coid);
            pthread_exit(NULL);
        }

        struct can_msg* canmsg = dequeue_peek(ocb->rx.queue);

        if (canmsg == NULL) {
            continue;
        }

        pthread_mutex_lock(&ocb->rx.mutex);
        blocked_client_t* client = ocb->rx.blocked_clients;

        msg_again.rcvid = client->rcvid;
        pthread_mutex_unlock(&ocb->rx.mutex);

        if ((status = MsgSend(
                coid, &msg_again, sizeof(msg_again_t), NULL, 0 )) == -1)
        {
            log_err( "rx_loop MsgSend status: %d, error: %s\n",
                    status, strerror(errno) );
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
            if (!resmgr->shutdown) {
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
    log_trace( "io_open -> (id: %d, rcvid: %ld)\n",
            ctp->id, (long int)ctp->rcvid );

    return (iofunc_open_default (ctp, msg, handle, extra));
}

/* Why we don't have any close callback? Because the default
 * function, iofunc_close_ocb_default(), does all we need in this
 * case: Free the ocb, update the time stamps etc. See the docs
 * for more info.
 */

int io_close_ocb (resmgr_context_t* ctp, void* reserved, RESMGR_OCB_T* _ocb) {
    log_trace("io_close_ocb -> id: %d\n", ctp->id);

    iofunc_ocb_t* ocb = (iofunc_ocb_t*)_ocb;

    _ocb->rx.nbytes = 0;
    _ocb->rx.offset = 0;

    return iofunc_close_ocb_default(ctp, reserved, ocb);
}

/*
 * io_read
 *
 * At this point, the client has called the library read() function, and
 * expects zero or more bytes.
 *
 * The message that we received can be accessed via the pointer *msg. A pointer
 * to the OCB that belongs to this read is the *ocb. The *ctp pointer points to
 * a context structure that is used by the resource manager framework to
 * determine whom to reply to, and more.
 */
int io_read (resmgr_context_t* ctp, io_read_t* msg, RESMGR_OCB_T* _ocb) {
    int     nparts;
    int     status;

    iofunc_ocb_t* ocb = (iofunc_ocb_t*)_ocb;

    log_trace( "io_read -> (id: %d, rcvid: %ld)\n",
            ctp->id, (long int)ctp->rcvid );

    /* Verify the client has the access rights needed to read from our device */
    if ((status = iofunc_read_verify(ctp, msg, ocb, NULL)) != EOK) {
        return (status);
    }

    /* Check if the read callback was called because of a pread() or a normal
     * read() call. If pread(), return with an error code indicating that it
     * isn't supported.
     */
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
        return ENOSYS;
    }

    if (_ocb->resmgr->channel_type == TX_CHANNEL) {
        return EIO; // Input/output error
    }

    if (_ocb->session->rx_queue.attr.size == 0) {
        _IO_SET_READ_NBYTES(ctp, 0);

        return (_RESMGR_NPARTS(0));
    }

    if (_ocb->rx.read_buffer == NULL
        || _ocb->rx.read_size < _IO_READ_GET_NBYTES(msg))
    {
        if (_ocb->rx.read_buffer) {
            free(_ocb->rx.read_buffer);
        }

        _ocb->rx.read_size += _IO_READ_GET_NBYTES(msg);
        _ocb->rx.read_buffer = malloc(_ocb->rx.read_size);

        if (_ocb->rx.read_buffer == NULL) {
            _ocb->rx.read_buffer = NULL;
            _ocb->rx.read_size = 0;

            return ENOMEM; // Not enough memory
        }
    }

    while (_ocb->rx.nbytes < _IO_READ_GET_NBYTES(msg)) {
        struct can_msg* canmsg =
            dequeue_peek_noblock(&_ocb->session->rx_queue);

        if (canmsg == NULL) {
            break;
        }

        if (_ocb->rx.nbytes + canmsg->len - _ocb->rx.offset
                <= _IO_READ_GET_NBYTES(msg))
        {
            size_t nbytes = canmsg->len - _ocb->rx.offset;

            memcpy( _ocb->rx.read_buffer + _ocb->rx.nbytes,
                    canmsg->dat + _ocb->rx.offset, nbytes );

            _ocb->rx.nbytes += nbytes;
            _ocb->rx.offset = 0;

            dequeue_noblock(&_ocb->session->rx_queue, 0);
        }
        else {
            size_t nbytes = _IO_READ_GET_NBYTES(msg) - _ocb->rx.nbytes;

            memcpy( _ocb->rx.read_buffer + _ocb->rx.nbytes,
                    canmsg->dat + _ocb->rx.offset, nbytes );

            _ocb->rx.nbytes = _IO_READ_GET_NBYTES(msg);
            _ocb->rx.offset += nbytes;
        }
    }

    if (_ocb->rx.nbytes < _IO_READ_GET_NBYTES(msg)) {
        pthread_mutex_lock(&_ocb->rx.mutex);

        blocked_client_t *client =
            get_blocked_client(&_ocb->rx.blocked_clients, ctp->rcvid);

        if (client == NULL) {
            blocked_client_t* new_block = malloc(sizeof(blocked_client_t));
            new_block->prev = new_block->next = NULL;
            new_block->rcvid = ctp->rcvid;

            store_blocked_client(&_ocb->rx.blocked_clients, new_block);
        }

        pthread_cond_signal(&_ocb->rx.cond);
        pthread_mutex_unlock(&_ocb->rx.mutex);

        return _RESMGR_NOREPLY;
    }
    else {
        /* Set up the return data IOV.
         * This is used to tell the system how large the buffer is in which we
         * want to return the data for the read() call.
         */
        SETIOV(ctp->iov, _ocb->rx.read_buffer, _ocb->rx.nbytes);

        /* Set up the number of bytes (returned by client's read()) */
        _IO_SET_READ_NBYTES(ctp, _ocb->rx.nbytes);

        nparts = 1;

        pthread_mutex_lock(&_ocb->rx.mutex);
        remove_blocked_client(&_ocb->rx.blocked_clients, ctp->rcvid);
        pthread_mutex_unlock(&_ocb->rx.mutex);
    }

    /* Mark the access time as invalid (we just accessed it) */
    if (msg->i.nbytes > 0) {
        ocb->attr->flags |= IOFUNC_ATTR_ATIME;
    }

    _ocb->rx.nbytes = 0;

    return (_RESMGR_NPARTS(nparts));
}

/*
 *  io_write
 *
 *  At this point, the client has called the library write()
 *  function, and expects that our resource manager will write
 *  the number of bytes that have been specified to the device.
 */
int io_write (resmgr_context_t* ctp, io_write_t* msg, RESMGR_OCB_T* _ocb) {
    int status;
    char *buf;

    iofunc_ocb_t* ocb = (iofunc_ocb_t*)_ocb;

    log_trace( "io_write -> (id: %d, rcvid: %ld)\n",
            ctp->id, (long int)ctp->rcvid );

    /* Check the access permissions of the client */
    if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK) {
        return (status);
    }

    /* Check if pwrite() or normal write() */
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
        return (ENOSYS);
    }

    if (_ocb->resmgr->channel_type == RX_CHANNEL) {
        return EIO; // Input/output error
    }

    /* Set the number of bytes successfully written for the client. This
     * information will be passed to the client by the resource manager
     * framework upon reply.
     */
    _IO_SET_WRITE_NBYTES (ctp, msg -> i.nbytes);
    log_trace("io_write: got write of %d bytes, data:\n", msg->i.nbytes);

    struct can_msg canmsg = {
        .mid = _ocb->resmgr->mid,
        .ext = { .is_extended_mid = _ocb->resmgr->is_extended_mid }
    };

    /* First check if our message buffer was large enough to receive the whole
     * write at once. If yes, print data.
     */
    if ((msg->i.nbytes <= ctp->info.msglen - ctp->offset - sizeof(msg->i)) &&
            (ctp->info.msglen < ctp->msg_max_size))
    {
        buf = (char *)(msg+1);

        buf[msg->i.nbytes] = '\0';
        log_trace("io_write: buf: %s\n", buf);

        int n = msg->i.nbytes/8 + (msg->i.nbytes%8 ? 1 : 0);

        int i;
        for (i = 0; i < n; ++i) {
            canmsg.len = msg->i.nbytes - 8*i;

            if (canmsg.len > 8) {
                canmsg.len = 8;
            }

            memcpy(canmsg.dat, buf+8*i, canmsg.len);

            log_trace("io_write; %s TS: %ums [%s] %X [%d] " \
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

            enqueue(&_ocb->resmgr->device_session->tx_queue, &canmsg);
        }
    }
    else {
        /* If we did not receive the whole message because the client wanted to
         * send more than we could receive, we allocate memory for all the data
         * and use resmgr_msgread() to read all the data at once. Although we
         * did not receive the data completely first, because our buffer was not
         * big enough, the data is still fully available on the client side,
         * because its write() call blocks until we return from this callback.
         */
        buf = malloc(msg->i.nbytes + 1);
        resmgr_msgread(ctp, buf, msg->i.nbytes, sizeof(msg->i));

        buf[msg->i.nbytes] = '\0';
        log_trace("io_write: buf: %s\n", buf);

        int n = msg->i.nbytes/8 + (msg->i.nbytes%8 ? 1 : 0);

        int i;
        for (i = 0; i < n; ++i) {
            canmsg.len = msg->i.nbytes - 8*i;

            if (canmsg.len > 8) {
                canmsg.len = 8;
            }

            memcpy(canmsg.dat, buf+8*i, canmsg.len);

            log_trace("io_write; %s TS: %ums [%s] %X [%d] " \
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

            enqueue(&_ocb->resmgr->device_session->tx_queue, &canmsg);
        }

        free(buf);
    }

    /* Finally, if we received more than 0 bytes, we mark the file information
     * for the device to be updated: modification time and change of file status
     * time. To avoid constant update of the real file status information (which
     * would involve overhead getting the current time), we just set these
     * flags. The actual update is done upon closing, which is valid according
     * to POSIX. */
    if (msg->i.nbytes > 0) {
        ocb->attr->flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;
    }

    return (_RESMGR_NPARTS (0));
}

int io_unblock (resmgr_context_t* ctp, io_pulse_t* msg, RESMGR_OCB_T* _ocb) {
    log_trace("io_unblock -> id: %d\n", ctp->id);

    iofunc_ocb_t* ocb = (iofunc_ocb_t*)_ocb;

    int status;
    if((status = iofunc_unblock_default(ctp, msg, ocb)) != _RESMGR_DEFAULT) {
        log_dbg("iofunc_unblock_default: No client connection was found.\n");

        return status;
    }

    return 0;
}

int io_devctl (resmgr_context_t* ctp, io_devctl_t* msg, RESMGR_OCB_T* _ocb) {
    int nbytes, status;

    iofunc_ocb_t* ocb = (iofunc_ocb_t*)_ocb;

    union data_t {
        uint32_t        latency_limit;
        uint32_t        bitrate;
        uint32_t        info2;

#if _NTO_VERSION >= 800
        CAN_DCMD_DATA   dcmd;
#else // i.e. _NTO_VERSION == 710
        DCMD_DATA       dcmd;
#endif
    } *data;

    log_trace("io_devctl -> id: %d\n", ctp->id);

    /*
     Let common code handle DCMD_ALL_* cases.
     You can do this before or after you intercept devctls, depending
     on your intentions.  Here we aren't using any predefined values,
     so let the system ones be handled first. See note 2.
    */
    if ((status = iofunc_devctl_default(ctp, msg, ocb)) !=
         _RESMGR_DEFAULT)
    {
        log_trace("iofunc_devctl_default == %d\n", status);
        return(status);
    }
    status = nbytes = 0;

    /* Get the data from the message. See Note 3. */
    data = _DEVCTL_DATA(msg->i);

    /*
     * Where applicable an associated canctl, candump or cansend tool usage
     * example is given as comments in each case below.
     */
    switch (msg->i.dcmd) {
    /*
     * Extended devctl() commands; these are in addition to the standard
     * QNX dev-can-* driver protocol commands
     */
    case EXT_CAN_DEVCTL_SET_LATENCY_LIMIT_MS:
    {
        uint32_t can_latency_limit = data->latency_limit;
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
        data->dcmd.mid = _ocb->resmgr->mid; // <- set MID
        nbytes = sizeof(data->dcmd.mid);

        log_trace("CAN_DEVCTL_GET_MID: %x\n", data->dcmd.mid);
        break;
    }
    case CAN_DEVCTL_SET_MID: // e.g. canctl -u1,rx1 -m 0x11CC0000
    {
        uint32_t mid = data->dcmd.mid;
        nbytes = 0;

        _ocb->resmgr->mid = mid;

        log_trace("CAN_DEVCTL_SET_MID: %x\n", mid);
        break;
    }
    case CAN_DEVCTL_GET_MFILTER: // e.g. #canctl -u0,tx0 -F
    {
        data->dcmd.mfilter = _ocb->resmgr->mfilter; // set MFILTER
        nbytes = sizeof(data->dcmd.mfilter);

        log_trace("CAN_DEVCTL_GET_MFILTER: %x\n", data->dcmd.mfilter);
        break;
    }
    case CAN_DEVCTL_SET_MFILTER: // e.g. canctl -u0,tx0 -f 0x11CC0000
    {
        uint32_t mfilter = data->dcmd.mfilter;
        nbytes = 0;

        _ocb->resmgr->mfilter = mfilter;

        log_trace("CAN_DEVCTL_SET_MFILTER: %x\n", mfilter);
        break;
    }
    case CAN_DEVCTL_GET_PRIO: // canctl -u1,tx1 -P
    {
        data->dcmd.prio = _ocb->resmgr->prio; // set PRIO
        nbytes = sizeof(data->dcmd.prio);

        log_trace("CAN_DEVCTL_GET_PRIO: %x\n", data->dcmd.prio);

        return ENOTSUP; // Not supported
    }
    case CAN_DEVCTL_SET_PRIO: // e.g. canctl -u1,tx1 -p 5
    {
        uint32_t prio = data->dcmd.prio;
        nbytes = 0;

        _ocb->resmgr->prio = prio;

        log_trace("CAN_DEVCTL_SET_PRIO: %x\n", prio);

        return ENOTSUP; // Not supported
    }
    case CAN_DEVCTL_GET_TIMESTAMP: // e.g. canctl -u1 -T
    {
        nbytes = sizeof(data->dcmd.timestamp);

        // set TIMESTAMP
        if (optt) {
            data->dcmd.timestamp = user_timestamp;
        }
        else if (user_timestamp_time != 0) {
            data->dcmd.timestamp =
                user_timestamp + get_clock_time_us()/1000 - user_timestamp_time;
        }
        else {
            data->dcmd.timestamp = get_clock_time_us()/1000;
        }

        log_trace("CAN_DEVCTL_GET_TIMESTAMP: %x\n", data->dcmd.timestamp);
        break;
    }
    case CAN_DEVCTL_SET_TIMESTAMP: // e.g. canctl -u1 -t 0xAAAAAA
    {
        uint32_t ts = data->dcmd.timestamp;
        nbytes = 0;

        user_timestamp = ts;
        user_timestamp_time = get_clock_time_us()/1000;

        log_trace("CAN_DEVCTL_SET_TIMESTAMP: %x\n", ts);
        break;
    }
    case CAN_DEVCTL_READ_CANMSG_EXT: // e.g. canctl -u0,rx0 -r
    {
        if (_ocb->resmgr->channel_type == TX_CHANNEL) {
            log_trace("CAN_DEVCTL_READ_CANMSG_EXT: Input/output error\n");

            return EIO; // Input/output error
        }

        struct can_msg* canmsg =
            dequeue_noblock( &_ocb->session->rx_queue,
                    _ocb->resmgr->latency_limit_ms );

        if (canmsg != NULL) { // Could be a zero size rx queue, i.e. a tx queue
            data->dcmd.canmsg = *canmsg;

            nbytes = sizeof(data->dcmd.canmsg);

            log_trace("CAN_DEVCTL_READ_CANMSG_EXT; %s TS: %ums [%s] %X [%d] " \
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

            pthread_mutex_lock(&_ocb->rx.mutex);
            remove_blocked_client(&_ocb->rx.blocked_clients, ctp->rcvid);
            pthread_mutex_unlock(&_ocb->rx.mutex);
        }
        else {
            pthread_mutex_lock(&_ocb->rx.mutex);

            blocked_client_t* new_block = malloc(sizeof(blocked_client_t));
            new_block->prev = new_block->next = NULL;
            new_block->rcvid = ctp->rcvid;

            store_blocked_client(&_ocb->rx.blocked_clients, new_block);

            pthread_cond_signal(&_ocb->rx.cond);
            pthread_mutex_unlock(&_ocb->rx.mutex);

            log_trace("CAN_DEVCTL_READ_CANMSG_EXT: _RESMGR_NOREPLY\n");

            return _RESMGR_NOREPLY;
        }

        break;
    }
    case CAN_DEVCTL_WRITE_CANMSG_EXT: // e.g. canctl -u0,rx0 -w0x22,1,0x55
    {
        if (_ocb->resmgr->channel_type == RX_CHANNEL) {
            log_trace("CAN_DEVCTL_WRITE_CANMSG_EXT: Input/output error\n");

            return EIO; // Input/output error
        }

        struct can_msg canmsg = data->dcmd.canmsg;
        nbytes = 0;

        enqueue(&_ocb->resmgr->device_session->tx_queue, &canmsg);

        log_trace("CAN_DEVCTL_WRITE_CANMSG_EXT; %s TS: %ums [%s] %X [%d] " \
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
        data->dcmd.error.drvr1 = 0x0; // set DRIVER ERROR 1
        data->dcmd.error.drvr2 = 0x0; // set DRIVER ERROR 1
        data->dcmd.error.drvr3 = 0x0; // set DRIVER ERROR 1
        data->dcmd.error.drvr4 = 0x0; // set DRIVER ERROR 1
        nbytes = sizeof(data->dcmd.error);

        log_trace("CAN_DEVCTL_ERROR: %x %x %x %x\n",
                data->dcmd.error.drvr1,
                data->dcmd.error.drvr2,
                data->dcmd.error.drvr3,
                data->dcmd.error.drvr4);
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
        u32 debug = data->info2;
        nbytes = 0;

        log_trace("CAN_DEVCTL_DEBUG_INFO2: %x\n", debug);
        break;
    }
    case CAN_DEVCTL_GET_STATS: // e.g. canctl -s
    {
        nbytes = sizeof(data->dcmd.stats);

        struct net_device* device = _ocb->resmgr->device_session->device;
        struct sja1000_priv* priv = netdev_priv(device);

        data->dcmd.stats.transmitted_frames = device->stats.tx_packets;
        data->dcmd.stats.received_frames = device->stats.rx_packets;
        data->dcmd.stats.missing_ack = device->stats.tx_dropped;

        /* Bus errors */
        data->dcmd.stats.total_frame_errors =
            device->stats.rx_errors + device->stats.tx_errors;

        /* Arbitration lost errors */
        data->dcmd.stats.stuff_errors = priv->can.can_stats.arbitration_lost;

        data->dcmd.stats.form_errors = 0;
        data->dcmd.stats.dom_bit_recess_errors = 0;
        data->dcmd.stats.recess_bit_dom_errors = 0;
        data->dcmd.stats.parity_errors = 0;
        data->dcmd.stats.crc_errors = device->stats.rx_crc_errors;
        data->dcmd.stats.hw_receive_overflows = device->stats.rx_over_errors;
        data->dcmd.stats.sw_receive_q_full = device->stats.rx_dropped;

        /* Changes to error warning state */
        data->dcmd.stats.error_warning_state_count =
            priv->can.can_stats.error_warning;

        /* Changes to error passive state */
        data->dcmd.stats.error_passive_state_count =
            priv->can.can_stats.error_passive;

        /* Changes to bus off state */
        data->dcmd.stats.bus_off_state_count = priv->can.can_stats.bus_off;

        data->dcmd.stats.bus_idle_count = priv->can.can_stats.bus_error;

        /* CAN controller re-starts */
        data->dcmd.stats.power_down_count =
            data->dcmd.stats.wake_up_count = priv->can.can_stats.restarts;

        data->dcmd.stats.rx_interrupts = device->stats.rx_errors;
        data->dcmd.stats.tx_interrupts = device->stats.tx_errors;
        data->dcmd.stats.total_interrupts =
            device->stats.rx_errors + device->stats.tx_errors;

        // TODO: These may need to be incremented on resmgr side:
        // device->stats.rx_bytes;
        // device->stats.tx_bytes;
        // device->stats.rx_dropped;
        // device->stats.tx_dropped;
        // device->stats.multicast;
        // device->stats.collisions;
        // device->stats.rx_length_errors;
        // device->stats.rx_frame_errors;
        // device->stats.rx_fifo_errors;
        // device->stats.rx_missed_errors;
        // device->stats.tx_aborted_errors;
        // device->stats.tx_carrier_errors;
        // device->stats.tx_fifo_errors;
        // device->stats.tx_heartbeat_errors;
        // device->stats.tx_window_errors;
        // device->stats.rx_compressed;
        // device->stats.tx_compressed;

        log_trace("CAN_DEVCTL_GET_STATS\n");
        break;
    }
    case CAN_DEVCTL_GET_INFO: // e.g. canctl -u0,rx0 -i
    {
        nbytes = sizeof(data->dcmd.info);

        struct net_device* device = _ocb->resmgr->device_session->device;
        struct can_priv* priv = netdev_priv(device);

        /* CAN device description */
        snprintf( data->dcmd.info.description, 64,
                "dev-can-linux dev: %s, driver: %s",
                _ocb->resmgr->name,
                _ocb->resmgr->driver_selection->driver->name );

        /* Number of message queue objects */
        data->dcmd.info.msgq_size = 0; // TODO: set msgq_size

        /* Number of client wait queue objects */
        data->dcmd.info.waitq_size = 0; // TODO: set waitq_size

        /* CAN driver mode - I/O or raw frames */
        data->dcmd.info.mode = CANDEV_MODE_RAW_FRAME;

        /* Bit rate */
        data->dcmd.info.bit_rate = priv->bittiming.bitrate;

        /* Bit rate prescaler */
        data->dcmd.info.bit_rate_prescaler = priv->bittiming.brp;

        /* Time quantum Sync Jump Width */
        data->dcmd.info.sync_jump_width = priv->bittiming.sjw;

        /* Time quantum Time Segment 1 */
        data->dcmd.info.time_segment_1 = priv->bittiming.phase_seg1;

        /* Time quantum Time Segment 2 */
        data->dcmd.info.time_segment_2 = priv->bittiming.phase_seg2;

        /* Number of TX Mailboxes */
        data->dcmd.info.num_tx_mboxes = 0; // TODO: set num_tx_mboxes

        /* Number of RX Mailboxes */
        data->dcmd.info.num_rx_mboxes = 0; // TODO: set num_rx_mboxes

        /* External loopback is enabled */
        data->dcmd.info.loopback_external = 0; // TODO: set loopback_external

        /* Internal loopback is enabled */
        data->dcmd.info.loopback_internal = optE; // TODO: check meaning of this

        /* Auto timed bus on after bus off */
        data->dcmd.info.autobus_on = 0; // TODO: set autobus_on

        /* Receiver only, no ack generation */
        data->dcmd.info.silent =
            (priv->ctrlmode & CAN_CTRLMODE_LISTENONLY ? 1 : 0);

        log_trace("CAN_DEVCTL_GET_INFO\n");
        break;
    }
    case CAN_DEVCTL_SET_TIMING: // e.g. canctl -u0,rx0 -c 250k,2,7,2,1
                                // e.g. canctl -u0,rx0 -c 1M,1,3,2,1
                                // e.g. canctl -u0,rx0 -c 0,1,3,2,1
                                //          reference clock don't change if '0'
    {
        struct can_devctl_timing timing = data->dcmd.timing;
        nbytes = 0;

        struct net_device* device = _ocb->resmgr->device_session->device;

        struct can_priv* priv = netdev_priv(device);

        if (!timing.ref_clock_freq) {
            timing.ref_clock_freq = priv->bittiming.bitrate;
        }

        struct user_dev_setup user;
        create_user_dev_setup(&timing, priv->clock.freq, &user);

        int err;

        device->flags &= ~IFF_UP;

        if ((err = device->resmgr_ops->changelink(device, &user, NULL)) != 0) {
            log_err("CAN_DEVCTL_SET_TIMING: changelink failed: (%d) %s\n",
                    -err, strerror(-err));

            device->flags |= IFF_UP;
            return -err;
        }

        device->flags |= IFF_UP;

        log_trace("CAN_DEVCTL_SET_TIMING: success\n");
        log_trace_bittiming_info(device);

        break;
    }
    case CAN_DEVCTL_RX_FRAME_RAW_NOBLOCK:
    case CAN_DEVCTL_RX_FRAME_RAW_BLOCK: // e.g. candump -u0,rx0
    {
        if (_ocb->resmgr->channel_type == TX_CHANNEL) {
            log_trace( "CAN_DEVCTL_RX_FRAME_RAW_%s: Input/output error\n",
                    msg->i.dcmd == CAN_DEVCTL_RX_FRAME_RAW_BLOCK
                        ? "BLOCK" : "NOBLOCK" );

            return EIO; // Input/output error
        }

        struct can_msg* canmsg =
            dequeue_noblock( &_ocb->session->rx_queue,
                    _ocb->resmgr->latency_limit_ms );

        if (canmsg != NULL) { // Could be a zero size rx queue, i.e. a tx queue
            data->dcmd.canmsg = *canmsg;

            nbytes = sizeof(data->dcmd.canmsg);

            log_trace("%s; %s TS: %ums [%s] %X [%d] " \
                      "%02X %02X %02X %02X %02X %02X %02X %02X\n",
                    (msg->i.dcmd == CAN_DEVCTL_RX_FRAME_RAW_BLOCK
                        ? "CAN_DEVCTL_RX_FRAME_RAW_BLOCK"
                        : "CAN_DEVCTL_RX_FRAME_RAW_NOBLOCK"),
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

            pthread_mutex_lock(&_ocb->rx.mutex);
            remove_blocked_client(&_ocb->rx.blocked_clients, ctp->rcvid);
            pthread_mutex_unlock(&_ocb->rx.mutex);
        }
        else if (msg->i.dcmd == CAN_DEVCTL_RX_FRAME_RAW_BLOCK) {
            pthread_mutex_lock(&_ocb->rx.mutex);

            blocked_client_t* new_block = malloc(sizeof(blocked_client_t));
            new_block->prev = new_block->next = NULL;
            new_block->rcvid = ctp->rcvid;

            store_blocked_client(&_ocb->rx.blocked_clients, new_block);

            pthread_cond_signal(&_ocb->rx.cond);
            pthread_mutex_unlock(&_ocb->rx.mutex);

            log_trace( "CAN_DEVCTL_RX_FRAME_RAW_%s: _RESMGR_NOREPLY\n",
                    msg->i.dcmd == CAN_DEVCTL_RX_FRAME_RAW_BLOCK
                        ? "BLOCK" : "NOBLOCK" );

            return _RESMGR_NOREPLY; /* put the client in block state */
        }
        else { // CAN_DEVCTL_RX_FRAME_RAW_NOBLOCK
            log_trace( "CAN_DEVCTL_RX_FRAME_RAW_%s: EAGAIN\n",
                    msg->i.dcmd == CAN_DEVCTL_RX_FRAME_RAW_BLOCK
                        ? "BLOCK" : "NOBLOCK" );

            return EAGAIN; /* There are no messages in the queue. */
        }

        break;
    }
    case CAN_DEVCTL_TX_FRAME_RAW: // e.g. cansend -u0,tx0 -w0x1234,1,0xABCD
    {
        if (_ocb->resmgr->channel_type == RX_CHANNEL) {
            log_trace("CAN_DEVCTL_TX_FRAME_RAW: Input/output error\n");

            return EIO; // Input/output error
        }

        struct can_msg canmsg = data->dcmd.canmsg;
        nbytes = 0;

        enqueue(&_ocb->resmgr->device_session->tx_queue, &canmsg);

        log_trace("CAN_DEVCTL_TX_FRAME_RAW; %s TS: %ums [%s] %X [%d] " \
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

void log_trace_bittiming_info (struct net_device* device) {
    struct can_priv* priv = netdev_priv(device);

    /* Clock frequency */
    log_trace("  clock: %uHz\n", priv->clock.freq);

    /* Bit-rate in bits/second */
    log_trace("  bitrate: %ubits/second\n", priv->bittiming.bitrate);

    /* Sample point in one-tenth of a percent */
    log_trace("  sample_point: %u (1/10 of percent)\n",
            priv->bittiming.sample_point);

    /* Time quanta (TQ) in nanoseconds */
    log_trace("  tq: %uns (TQ)\n", priv->bittiming.tq);

    /* Propagation segment in TQs */
    log_trace("  prop_seg: %u\n", priv->bittiming.prop_seg);

    /* Phase buffer segment 1 in TQs */
    log_trace("  phase_seg1: %uTQ\n", priv->bittiming.phase_seg1);

    /* Phase buffer segment 2 in TQs */
    log_trace("  phase_seg2: %uTQ\n", priv->bittiming.phase_seg2);

    /* Synchronisation jump width in TQs */
    log_trace("  sjw: %uTQ\n", priv->bittiming.sjw);

    /* Bit-rate prescaler */
    log_trace("  brp: %u\n", priv->bittiming.brp);
}
