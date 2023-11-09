/*
 * \file    resmgr.h
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

#ifndef SRC_RESMGR_H_
#define SRC_RESMGR_H_

#include <pci/pci.h>
#include <session.h>

#include <drivers/net/can/sja1000/sja1000.h>

struct can_ocb;

#if CONFIG_QNX_RESMGR_THREAD_POOL == 1
/*
 * Define THREAD_POOL_PARAM_T so that we can avoid compiler warnings
 */
#define THREAD_POOL_PARAM_T dispatch_context_t
#endif

/*
 * Extending the OCB the attribute structure
 */
#define IOFUNC_OCB_T struct can_ocb

#include <sys/iofunc.h>

#include <sys/resmgr.h>
#include <sys/dispatch.h>
#include <sys/can_dcmd.h>
#include <sys/neutrino.h>

#define MAX_NAME_SIZE (IFNAMSIZ*2) // e.g. /dev/can1/rx0


/*
 * Instead of netlink interface we introduce this more specific and
 * simplified user settings. Note that the user in this context is not
 * necessarily the network layer. In the case of QNX dev-can-* drivers
 * the user is specifically a resource manager driver.
 */
struct user_dev_setup {
    struct can_bittiming bittiming;
    struct can_bittiming data_bittiming;
    struct can_btr_t {  /* Special feature to force btr0 and btr1 to specific */
        u8 btr0;        /* values needed for some applications. */
        u8 btr1;
    } can_btr;
    struct can_ctrlmode ctrlmode;
    int restart_ms;
    struct can_tdc tdc;
    u16 termination;

    bool set_bittiming;
    bool set_data_bittiming;
    bool set_btr;       /* Special feature */
    bool set_ctrlmode;
    bool set_restart_ms;
    bool set_restart;
    bool set_tdc;
    bool set_termination;
};

/*
 *  struct resmgr_ops - resource manager link operations
 *
 *  @changelink:      Function for changing parameters of an existing device
 *  @get_xstats_size: Function to calculate required room for dumping device
 *                    specific statistics
 *  @fill_xstats:     Function to dump device specific statistics
 */
struct resmgr_ops {
    int     (*changelink)(struct net_device* dev, struct user_dev_setup* user,
                          struct netlink_ext_ack *extack);

    size_t  (*get_xstats_size)(const struct net_device* dev);

    int     (*fill_xstats)(struct sk_buff* skb, const struct net_device* dev);
};

typedef struct blocked_client {
    struct blocked_client *prev, *next;

    int rcvid; // resmgr_msg_again() entry; -1 when no blocking client
} blocked_client_t;

/*
 * Extended OCB structure
 */
typedef struct can_ocb {
    iofunc_ocb_t core;

    struct can_resmgr* resmgr;

    int session_index;
    client_session_t *session;

    struct rx_t {
        pthread_attr_t thread_attr;
        pthread_t thread;
        pthread_mutex_t mutex;
        pthread_cond_t cond;

        blocked_client_t* blocked_clients;

        queue_t* queue;

        char* read_buffer;
        size_t read_size;
        size_t nbytes, offset;
    } rx;
} can_ocb_t;

typedef enum channel_type {
    RX_CHANNEL = 0,
    TX_CHANNEL
} channel_type_t;

typedef struct can_resmgr {
    struct can_resmgr *prev, *next;

    device_session_t* device_session;
    struct driver_selection* driver_selection;
    int is_extended_mid; // Only applicable for read and write functions not
                         // used for direct devctl send/receive functionality.

    char name[MAX_NAME_SIZE];
    channel_type_t channel_type;

    dispatch_t* dispatch;
    resmgr_attr_t resmgr_attr;
    iofunc_attr_t iofunc_attr;

#if CONFIG_QNX_RESMGR_THREAD_POOL == 1
    thread_pool_attr_t thread_pool_attr;
    thread_pool_t* thread_pool;
#elif CONFIG_QNX_RESMGR_SINGLE_THREAD == 1
    dispatch_context_t* dispatch_context;
    pthread_attr_t dispatch_thread_attr;
    pthread_t dispatch_thread;
#endif

    int id;

    iofunc_mount_t mount;
    iofunc_funcs_t mount_funcs;

    uint32_t latency_limit_ms;  /* Maximum allowed latency in milliseconds */
    uint32_t mid;               /* CAN message identifier */
    uint32_t mfilter;           /* CAN message filter */
    uint32_t prio;              /* CAN priority - not used */

    int shutdown;
} can_resmgr_t;


static inline void store_resmgr (can_resmgr_t** root, can_resmgr_t* r) {
    if (*root == NULL) {
        *root = r;
        r->prev = r->next = NULL;
        return;
    }

    can_resmgr_t* last = *root;

    while (last->next != NULL) {
        last = last->next;
    }

    last->next = r;
    r->prev = last;
    r->next = NULL;
}

static inline can_resmgr_t* get_resmgr (can_resmgr_t** root, int id) {
    can_resmgr_t* location = *root;

    while (location != NULL) {
        if (location->id == id) {
            return location;
        }

        location = location->next;
    }

    return NULL;
}

static inline void free_all_resmgrs (can_resmgr_t** root) {
    while (*root != NULL) {
        can_resmgr_t* next = (*root)->next;

        free(*root);

        *root = next;
    }
}

static inline void store_blocked_client (
        blocked_client_t** root, blocked_client_t* r)
{
    if (*root == NULL) {
        *root = r;
        r->prev = r->next = NULL;
        return;
    }

    blocked_client_t* last = *root;

    while (last->next != NULL) {
        last = last->next;
    }

    last->next = r;
    r->prev = last;
    r->next = NULL;
}

static inline blocked_client_t* get_blocked_client (
        blocked_client_t** root, int rcvid)
{
    blocked_client_t* location = *root;

    while (location != NULL) {
        if (location->rcvid == rcvid) {
            return location;
        }

        location = location->next;
    }

    return NULL;
}

static inline void remove_blocked_client (blocked_client_t** root, int rcvid) {
    blocked_client_t* location = *root;

    while (location != NULL) {
        if (location->rcvid == rcvid) {
            if (location->prev == NULL) {
                *root = location->next;
                if (*root != NULL) {
                    (*root)->prev = NULL;
                }
            }
            else {
                location->prev->next = location->next;

                if (location->next) {
                    location->next->prev = location->prev;
                }
            }

            blocked_client_t* next = NULL;

            if (location != NULL) {
                next = location->next;
            }

            free(location);

            location = next;
        }
        else {
            location = location->next;
        }
    }
}

static inline void free_all_blocked_clients (blocked_client_t** root) {
    while (*root != NULL) {
        blocked_client_t* next = (*root)->next;

        free(*root);

        *root = next;
    }
}

#endif /* SRC_RESMGR_H_ */
