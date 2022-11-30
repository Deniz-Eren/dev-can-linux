/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the Interfaces handler.
 *
 * Version:	@(#)dev.h	1.0.10	08/12/93
 *
 * Authors:	Ross Biro
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *		Corey Minyard <wf-rch!minyard@relay.EU.net>
 *		Donald J. Becker, <becker@cesdis.gsfc.nasa.gov>
 *		Alan Cox, <alan@lxorguk.ukuu.org.uk>
 *		Bjorn Ekwall. <bj0rn@blox.se>
 *              Pekka Riikonen <priikone@poseidon.pspt.fi>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *		Moved to /usr/include/linux for NET3
 */
#ifndef _LINUX_NETDEVICE_H
#define _LINUX_NETDEVICE_H

#include "linux/types.h"
#include "linux/compiler.h"
#include "uapi/linux/if.h"
//#include <linux/timer.h>
//#include <linux/bug.h>
//#include <linux/delay.h>
//#include <linux/atomic.h>
//#include <linux/prefetch.h>
//#include <asm/cache.h>
//#include <asm/byteorder.h>
//
//#include <linux/percpu.h>
//#include <linux/rculist.h>
//#include <linux/dmaengine.h>
//#include <linux/workqueue.h>
//#include <linux/dynamic_queue_limits.h>
//
//#include <linux/ethtool.h>
//#include <net/net_namespace.h>
//#include <net/dsa.h>
//#ifdef CONFIG_DCB
//#include <net/dcbnl.h>
//#endif
//#include <net/netprio_cgroup.h>
//
//#include <linux/netdev_features.h>
//#include <linux/neighbour.h>
//#include <uapi/linux/netdevice.h>
//#include <uapi/linux/if_bonding.h>
#include <limits.h>


enum netdev_tx {
	__NETDEV_TX_MIN	 = INT_MIN,	/* make sure enum is signed */
	NETDEV_TX_OK	 = 0x00,	/* driver took care of packet */
	NETDEV_TX_BUSY	 = 0x10,	/* driver tx path was busy*/
	NETDEV_TX_LOCKED = 0x20,	/* driver tx lock was already taken */
};
typedef enum netdev_tx netdev_tx_t;

/*
 *	Old network device statistics. Fields are native words
 *	(unsigned long) so they can be read and written atomically.
 */

struct net_device_stats {
	unsigned long	rx_packets;
	unsigned long	tx_packets;
	unsigned long	rx_bytes;
	unsigned long	tx_bytes;
	unsigned long	rx_errors;
	unsigned long	tx_errors;
	unsigned long	rx_dropped;
	unsigned long	tx_dropped;
	unsigned long	multicast;
	unsigned long	collisions;
	unsigned long	rx_length_errors;
	unsigned long	rx_over_errors;
	unsigned long	rx_crc_errors;
	unsigned long	rx_frame_errors;
	unsigned long	rx_fifo_errors;
	unsigned long	rx_missed_errors;
	unsigned long	tx_aborted_errors;
	unsigned long	tx_carrier_errors;
	unsigned long	tx_fifo_errors;
	unsigned long	tx_heartbeat_errors;
	unsigned long	tx_window_errors;
	unsigned long	rx_compressed;
	unsigned long	tx_compressed;
};

/*
 * This structure defines the management hooks for network devices.
 * The following hooks can be defined; unless noted otherwise, they are
 * optional and can be filled with a null pointer.
 *
 * int (*ndo_init)(struct net_device *dev);
 *     This function is called once when network device is registered.
 *     The network device can use this to any late stage initializaton
 *     or semantic validattion. It can fail with an error code which will
 *     be propogated back to register_netdev
 *
 * void (*ndo_uninit)(struct net_device *dev);
 *     This function is called when device is unregistered or when registration
 *     fails. It is not called if init fails.
 *
 * int (*ndo_open)(struct net_device *dev);
 *     This function is called when network device transistions to the up
 *     state.
 *
 * int (*ndo_stop)(struct net_device *dev);
 *     This function is called when network device transistions to the down
 *     state.
 *
 * netdev_tx_t (*ndo_start_xmit)(struct sk_buff *skb,
 *                               struct net_device *dev);
 *	Called when a packet needs to be transmitted.
 *	Returns NETDEV_TX_OK.  Can return NETDEV_TX_BUSY, but you should stop
 *	the queue before that can happen; it's for obsolete devices and weird
 *	corner cases, but the stack really does a non-trivial amount
 *	of useless work if you return NETDEV_TX_BUSY.
 *        (can also return NETDEV_TX_LOCKED iff NETIF_F_LLTX)
 *	Required can not be NULL.
 *
 * u16 (*ndo_select_queue)(struct net_device *dev, struct sk_buff *skb,
 *                         void *accel_priv, select_queue_fallback_t fallback);
 *	Called to decide which queue to when device supports multiple
 *	transmit queues.
 *
 * void (*ndo_change_rx_flags)(struct net_device *dev, int flags);
 *	This function is called to allow device receiver to make
 *	changes to configuration when multicast or promiscious is enabled.
 *
 * void (*ndo_set_rx_mode)(struct net_device *dev);
 *	This function is called device changes address list filtering.
 *	If driver handles unicast address filtering, it should set
 *	IFF_UNICAST_FLT to its priv_flags.
 *
 * int (*ndo_set_mac_address)(struct net_device *dev, void *addr);
 *	This function  is called when the Media Access Control address
 *	needs to be changed. If this interface is not defined, the
 *	mac address can not be changed.
 *
 * int (*ndo_validate_addr)(struct net_device *dev);
 *	Test if Media Access Control address is valid for the device.
 *
 * int (*ndo_do_ioctl)(struct net_device *dev, struct ifreq *ifr, int cmd);
 *	Called when a user request an ioctl which can't be handled by
 *	the generic interface code. If not defined ioctl's return
 *	not supported error code.
 *
 * int (*ndo_set_config)(struct net_device *dev, struct ifmap *map);
 *	Used to set network devices bus interface parameters. This interface
 *	is retained for legacy reason, new devices should use the bus
 *	interface (PCI) for low level management.
 *
 * int (*ndo_change_mtu)(struct net_device *dev, int new_mtu);
 *	Called when a user wants to change the Maximum Transfer Unit
 *	of a device. If not defined, any request to change MTU will
 *	will return an error.
 *
 * void (*ndo_tx_timeout)(struct net_device *dev);
 *	Callback uses when the transmitter has not made any progress
 *	for dev->watchdog ticks.
 *
 * struct rtnl_link_stats64* (*ndo_get_stats64)(struct net_device *dev,
 *                      struct rtnl_link_stats64 *storage);
 * struct net_device_stats* (*ndo_get_stats)(struct net_device *dev);
 *	Called when a user wants to get the network device usage
 *	statistics. Drivers must do one of the following:
 *	1. Define @ndo_get_stats64 to fill in a zero-initialised
 *	   rtnl_link_stats64 structure passed by the caller.
 *	2. Define @ndo_get_stats to update a net_device_stats structure
 *	   (which should normally be dev->stats) and return a pointer to
 *	   it. The structure may be changed asynchronously only if each
 *	   field is written atomically.
 *	3. Update dev->stats asynchronously and atomically, and define
 *	   neither operation.
 *
 * int (*ndo_vlan_rx_add_vid)(struct net_device *dev, __be16 proto, u16 vid);
 *	If device support VLAN filtering this function is called when a
 *	VLAN id is registered.
 *
 * int (*ndo_vlan_rx_kill_vid)(struct net_device *dev, __be16 proto, u16 vid);
 *	If device support VLAN filtering this function is called when a
 *	VLAN id is unregistered.
 *
 * void (*ndo_poll_controller)(struct net_device *dev);
 *
 *	SR-IOV management functions.
 * int (*ndo_set_vf_mac)(struct net_device *dev, int vf, u8* mac);
 * int (*ndo_set_vf_vlan)(struct net_device *dev, int vf, u16 vlan, u8 qos);
 * int (*ndo_set_vf_rate)(struct net_device *dev, int vf, int min_tx_rate,
 *			  int max_tx_rate);
 * int (*ndo_set_vf_spoofchk)(struct net_device *dev, int vf, bool setting);
 * int (*ndo_set_vf_trust)(struct net_device *dev, int vf, bool setting);
 * int (*ndo_get_vf_config)(struct net_device *dev,
 *			    int vf, struct ifla_vf_info *ivf);
 * int (*ndo_set_vf_link_state)(struct net_device *dev, int vf, int link_state);
 * int (*ndo_set_vf_port)(struct net_device *dev, int vf,
 *			  struct nlattr *port[]);
 *
 *      Enable or disable the VF ability to query its RSS Redirection Table and
 *      Hash Key. This is needed since on some devices VF share this information
 *      with PF and querying it may adduce a theoretical security risk.
 * int (*ndo_set_vf_rss_query_en)(struct net_device *dev, int vf, bool setting);
 * int (*ndo_get_vf_port)(struct net_device *dev, int vf, struct sk_buff *skb);
 * int (*ndo_setup_tc)(struct net_device *dev, u8 tc)
 * 	Called to setup 'tc' number of traffic classes in the net device. This
 * 	is always called from the stack with the rtnl lock held and netif tx
 * 	queues stopped. This allows the netdevice to perform queue management
 * 	safely.
 *
 *	Fiber Channel over Ethernet (FCoE) offload functions.
 * int (*ndo_fcoe_enable)(struct net_device *dev);
 *	Called when the FCoE protocol stack wants to start using LLD for FCoE
 *	so the underlying device can perform whatever needed configuration or
 *	initialization to support acceleration of FCoE traffic.
 *
 * int (*ndo_fcoe_disable)(struct net_device *dev);
 *	Called when the FCoE protocol stack wants to stop using LLD for FCoE
 *	so the underlying device can perform whatever needed clean-ups to
 *	stop supporting acceleration of FCoE traffic.
 *
 * int (*ndo_fcoe_ddp_setup)(struct net_device *dev, u16 xid,
 *			     struct scatterlist *sgl, unsigned int sgc);
 *	Called when the FCoE Initiator wants to initialize an I/O that
 *	is a possible candidate for Direct Data Placement (DDP). The LLD can
 *	perform necessary setup and returns 1 to indicate the device is set up
 *	successfully to perform DDP on this I/O, otherwise this returns 0.
 *
 * int (*ndo_fcoe_ddp_done)(struct net_device *dev,  u16 xid);
 *	Called when the FCoE Initiator/Target is done with the DDPed I/O as
 *	indicated by the FC exchange id 'xid', so the underlying device can
 *	clean up and reuse resources for later DDP requests.
 *
 * int (*ndo_fcoe_ddp_target)(struct net_device *dev, u16 xid,
 *			      struct scatterlist *sgl, unsigned int sgc);
 *	Called when the FCoE Target wants to initialize an I/O that
 *	is a possible candidate for Direct Data Placement (DDP). The LLD can
 *	perform necessary setup and returns 1 to indicate the device is set up
 *	successfully to perform DDP on this I/O, otherwise this returns 0.
 *
 * int (*ndo_fcoe_get_hbainfo)(struct net_device *dev,
 *			       struct netdev_fcoe_hbainfo *hbainfo);
 *	Called when the FCoE Protocol stack wants information on the underlying
 *	device. This information is utilized by the FCoE protocol stack to
 *	register attributes with Fiber Channel management service as per the
 *	FC-GS Fabric Device Management Information(FDMI) specification.
 *
 * int (*ndo_fcoe_get_wwn)(struct net_device *dev, u64 *wwn, int type);
 *	Called when the underlying device wants to override default World Wide
 *	Name (WWN) generation mechanism in FCoE protocol stack to pass its own
 *	World Wide Port Name (WWPN) or World Wide Node Name (WWNN) to the FCoE
 *	protocol stack to use.
 *
 *	RFS acceleration.
 * int (*ndo_rx_flow_steer)(struct net_device *dev, const struct sk_buff *skb,
 *			    u16 rxq_index, u32 flow_id);
 *	Set hardware filter for RFS.  rxq_index is the target queue index;
 *	flow_id is a flow ID to be passed to rps_may_expire_flow() later.
 *	Return the filter ID on success, or a negative error code.
 *
 *	Slave management functions (for bridge, bonding, etc).
 * int (*ndo_add_slave)(struct net_device *dev, struct net_device *slave_dev);
 *	Called to make another netdev an underling.
 *
 * int (*ndo_del_slave)(struct net_device *dev, struct net_device *slave_dev);
 *	Called to release previously enslaved netdev.
 *
 *      Feature/offload setting functions.
 * netdev_features_t (*ndo_fix_features)(struct net_device *dev,
 *		netdev_features_t features);
 *	Adjusts the requested feature flags according to device-specific
 *	constraints, and returns the resulting flags. Must not modify
 *	the device state.
 *
 * int (*ndo_set_features)(struct net_device *dev, netdev_features_t features);
 *	Called to update device configuration to new features. Passed
 *	feature set might be less than what was returned by ndo_fix_features()).
 *	Must return >0 or -errno if it changed dev->features itself.
 *
 * int (*ndo_fdb_add)(struct ndmsg *ndm, struct nlattr *tb[],
 *		      struct net_device *dev,
 *		      const unsigned char *addr, u16 vid, u16 flags)
 *	Adds an FDB entry to dev for addr.
 * int (*ndo_fdb_del)(struct ndmsg *ndm, struct nlattr *tb[],
 *		      struct net_device *dev,
 *		      const unsigned char *addr, u16 vid)
 *	Deletes the FDB entry from dev coresponding to addr.
 * int (*ndo_fdb_dump)(struct sk_buff *skb, struct netlink_callback *cb,
 *		       struct net_device *dev, struct net_device *filter_dev,
 *		       int idx)
 *	Used to add FDB entries to dump requests. Implementers should add
 *	entries to skb and update idx with the number of entries.
 *
 * int (*ndo_bridge_setlink)(struct net_device *dev, struct nlmsghdr *nlh,
 *			     u16 flags)
 * int (*ndo_bridge_getlink)(struct sk_buff *skb, u32 pid, u32 seq,
 *			     struct net_device *dev, u32 filter_mask,
 *			     int nlflags)
 * int (*ndo_bridge_dellink)(struct net_device *dev, struct nlmsghdr *nlh,
 *			     u16 flags);
 *
 * int (*ndo_change_carrier)(struct net_device *dev, bool new_carrier);
 *	Called to change device carrier. Soft-devices (like dummy, team, etc)
 *	which do not represent real hardware may define this to allow their
 *	userspace components to manage their virtual carrier state. Devices
 *	that determine carrier state from physical hardware properties (eg
 *	network cables) or protocol-dependent mechanisms (eg
 *	USB_CDC_NOTIFY_NETWORK_CONNECTION) should NOT implement this function.
 *
 * int (*ndo_get_phys_port_id)(struct net_device *dev,
 *			       struct netdev_phys_item_id *ppid);
 *	Called to get ID of physical port of this device. If driver does
 *	not implement this, it is assumed that the hw is not able to have
 *	multiple net devices on single physical port.
 *
 * void (*ndo_add_vxlan_port)(struct  net_device *dev,
 *			      sa_family_t sa_family, __be16 port);
 *	Called by vxlan to notiy a driver about the UDP port and socket
 *	address family that vxlan is listnening to. It is called only when
 *	a new port starts listening. The operation is protected by the
 *	vxlan_net->sock_lock.
 *
 * void (*ndo_del_vxlan_port)(struct  net_device *dev,
 *			      sa_family_t sa_family, __be16 port);
 *	Called by vxlan to notify the driver about a UDP port and socket
 *	address family that vxlan is not listening to anymore. The operation
 *	is protected by the vxlan_net->sock_lock.
 *
 * void* (*ndo_dfwd_add_station)(struct net_device *pdev,
 *				 struct net_device *dev)
 *	Called by upper layer devices to accelerate switching or other
 *	station functionality into hardware. 'pdev is the lowerdev
 *	to use for the offload and 'dev' is the net device that will
 *	back the offload. Returns a pointer to the private structure
 *	the upper layer will maintain.
 * void (*ndo_dfwd_del_station)(struct net_device *pdev, void *priv)
 *	Called by upper layer device to delete the station created
 *	by 'ndo_dfwd_add_station'. 'pdev' is the net device backing
 *	the station and priv is the structure returned by the add
 *	operation.
 * netdev_tx_t (*ndo_dfwd_start_xmit)(struct sk_buff *skb,
 *				      struct net_device *dev,
 *				      void *priv);
 *	Callback to use for xmit over the accelerated station. This
 *	is used in place of ndo_start_xmit on accelerated net
 *	devices.
 * netdev_features_t (*ndo_features_check) (struct sk_buff *skb,
 *					    struct net_device *dev
 *					    netdev_features_t features);
 *	Called by core transmit path to determine if device is capable of
 *	performing offload operations on a given packet. This is to give
 *	the device an opportunity to implement any restrictions that cannot
 *	be otherwise expressed by feature flags. The check is called with
 *	the set of features that the stack has calculated and it returns
 *	those the driver believes to be appropriate.
 * int (*ndo_set_tx_maxrate)(struct net_device *dev,
 *			     int queue_index, u32 maxrate);
 *	Called when a user wants to set a max-rate limitation of specific
 *	TX queue.
 * int (*ndo_get_iflink)(const struct net_device *dev);
 *	Called to get the iflink value of this device.
 * void (*ndo_change_proto_down)(struct net_device *dev,
 *				  bool proto_down);
 *	This function is used to pass protocol port error state information
 *	to the switch driver. The switch driver can react to the proto_down
 *      by doing a phys down on the associated switch port.
 * int (*ndo_fill_metadata_dst)(struct net_device *dev, struct sk_buff *skb);
 *	This function is used to get egress tunnel information for given skb.
 *	This is useful for retrieving outer tunnel header parameters while
 *	sampling packet.
 *
 */
struct net_device_ops {
	int			(*ndo_open)(struct net_device *dev);
	int			(*ndo_stop)(struct net_device *dev);
	netdev_tx_t		(*ndo_start_xmit) (struct sk_buff *skb,
						   struct net_device *dev);
	int			(*ndo_change_mtu)(struct net_device *dev,
						  int new_mtu);
};

/**
 *	struct net_device - The DEVICE structure.
 *		Actually, this whole structure is a big mistake.  It mixes I/O
 *		data with strictly "high-level" data, and it has to know about
 *		almost every data structure used in the INET module.
 *
 *	@name:	This is the first field of the "visible" part of this structure
 *		(i.e. as seen by users in the "Space.c" file).  It is the name
 *	 	of the interface.
 *
 *	@name_hlist: 	Device name hash chain, please keep it close to name[]
 *	@ifalias:	SNMP alias
 *	@mem_end:	Shared memory end
 *	@mem_start:	Shared memory start
 *	@base_addr:	Device I/O address
 *	@irq:		Device IRQ number
 *
 *	@carrier_changes:	Stats to monitor carrier on<->off transitions
 *
 *	@state:		Generic network queuing layer state, see netdev_state_t
 *	@dev_list:	The global list of network devices
 *	@napi_list:	List entry, that is used for polling napi devices
 *	@unreg_list:	List entry, that is used, when we are unregistering the
 *			device, see the function unregister_netdev
 *	@close_list:	List entry, that is used, when we are closing the device
 *
 *	@adj_list:	Directly linked devices, like slaves for bonding
 *	@all_adj_list:	All linked devices, *including* neighbours
 *	@features:	Currently active device features
 *	@hw_features:	User-changeable features
 *
 *	@wanted_features:	User-requested features
 *	@vlan_features:		Mask of features inheritable by VLAN devices
 *
 *	@hw_enc_features:	Mask of features inherited by encapsulating devices
 *				This field indicates what encapsulation
 *				offloads the hardware is capable of doing,
 *				and drivers will need to set them appropriately.
 *
 *	@mpls_features:	Mask of features inheritable by MPLS
 *
 *	@ifindex:	interface index
 *	@group:		The group, that the device belongs to
 *
 *	@stats:		Statistics struct, which was left as a legacy, use
 *			rtnl_link_stats64 instead
 *
 *	@rx_dropped:	Dropped packets by core network,
 *			do not use this in drivers
 *	@tx_dropped:	Dropped packets by core network,
 *			do not use this in drivers
 *
 *	@wireless_handlers:	List of functions to handle Wireless Extensions,
 *				instead of ioctl,
 *				see <net/iw_handler.h> for details.
 *	@wireless_data:	Instance data managed by the core of wireless extensions
 *
 *	@netdev_ops:	Includes several pointers to callbacks,
 *			if one wants to override the ndo_*() functions
 *	@ethtool_ops:	Management operations
 *	@header_ops:	Includes callbacks for creating,parsing,caching,etc
 *			of Layer 2 headers.
 *
 *	@flags:		Interface flags (a la BSD)
 *	@priv_flags:	Like 'flags' but invisible to userspace,
 *			see if.h for the definitions
 *	@gflags:	Global flags ( kept as legacy )
 *	@padded:	How much padding added by alloc_netdev()
 *	@operstate:	RFC2863 operstate
 *	@link_mode:	Mapping policy to operstate
 *	@if_port:	Selectable AUI, TP, ...
 *	@dma:		DMA channel
 *	@mtu:		Interface MTU value
 *	@type:		Interface hardware type
 *	@hard_header_len: Maximum hardware header length.
 *
 *	@needed_headroom: Extra headroom the hardware may need, but not in all
 *			  cases can this be guaranteed
 *	@needed_tailroom: Extra tailroom the hardware may need, but not in all
 *			  cases can this be guaranteed. Some cases also use
 *			  LL_MAX_HEADER instead to allocate the skb
 *
 *	interface address info:
 *
 * 	@perm_addr:		Permanent hw address
 * 	@addr_assign_type:	Hw address assignment type
 * 	@addr_len:		Hardware address length
 * 	@neigh_priv_len;	Used in neigh_alloc(),
 * 				initialized only in atm/clip.c
 * 	@dev_id:		Used to differentiate devices that share
 * 				the same link layer address
 * 	@dev_port:		Used to differentiate devices that share
 * 				the same function
 *	@addr_list_lock:	XXX: need comments on this one
 *	@uc_promisc:		Counter, that indicates, that promiscuous mode
 *				has been enabled due to the need to listen to
 *				additional unicast addresses in a device that
 *				does not implement ndo_set_rx_mode()
 *	@uc:			unicast mac addresses
 *	@mc:			multicast mac addresses
 *	@dev_addrs:		list of device hw addresses
 *	@queues_kset:		Group of all Kobjects in the Tx and RX queues
 *	@promiscuity:		Number of times, the NIC is told to work in
 *				Promiscuous mode, if it becomes 0 the NIC will
 *				exit from working in Promiscuous mode
 *	@allmulti:		Counter, enables or disables allmulticast mode
 *
 *	@vlan_info:	VLAN info
 *	@dsa_ptr:	dsa specific data
 *	@tipc_ptr:	TIPC specific data
 *	@atalk_ptr:	AppleTalk link
 *	@ip_ptr:	IPv4 specific data
 *	@dn_ptr:	DECnet specific data
 *	@ip6_ptr:	IPv6 specific data
 *	@ax25_ptr:	AX.25 specific data
 *	@ieee80211_ptr:	IEEE 802.11 specific data, assign before registering
 *
 *	@last_rx:	Time of last Rx
 *	@dev_addr:	Hw address (before bcast,
 *			because most packets are unicast)
 *
 *	@_rx:			Array of RX queues
 *	@num_rx_queues:		Number of RX queues
 *				allocated at register_netdev() time
 *	@real_num_rx_queues: 	Number of RX queues currently active in device
 *
 *	@rx_handler:		handler for received packets
 *	@rx_handler_data: 	XXX: need comments on this one
 *	@ingress_queue:		XXX: need comments on this one
 *	@broadcast:		hw bcast address
 *
 *	@rx_cpu_rmap:	CPU reverse-mapping for RX completion interrupts,
 *			indexed by RX queue number. Assigned by driver.
 *			This must only be set if the ndo_rx_flow_steer
 *			operation is defined
 *	@index_hlist:		Device index hash chain
 *
 *	@_tx:			Array of TX queues
 *	@num_tx_queues:		Number of TX queues allocated at alloc_netdev_mq() time
 *	@real_num_tx_queues: 	Number of TX queues currently active in device
 *	@qdisc:			Root qdisc from userspace point of view
 *	@tx_queue_len:		Max frames per queue allowed
 *	@tx_global_lock: 	XXX: need comments on this one
 *
 *	@xps_maps:	XXX: need comments on this one
 *
 *	@offload_fwd_mark:	Offload device fwding mark
 *
 *	@trans_start:		Time (in jiffies) of last Tx
 *	@watchdog_timeo:	Represents the timeout that is used by
 *				the watchdog ( see dev_watchdog() )
 *	@watchdog_timer:	List of timers
 *
 *	@pcpu_refcnt:		Number of references to this device
 *	@todo_list:		Delayed register/unregister
 *	@link_watch_list:	XXX: need comments on this one
 *
 *	@reg_state:		Register/unregister state machine
 *	@dismantle:		Device is going to be freed
 *	@rtnl_link_state:	This enum represents the phases of creating
 *				a new link
 *
 *	@destructor:		Called from unregister,
 *				can be used to call free_netdev
 *	@npinfo:		XXX: need comments on this one
 * 	@nd_net:		Network namespace this network device is inside
 *
 * 	@ml_priv:	Mid-layer private
 * 	@lstats:	Loopback statistics
 * 	@tstats:	Tunnel statistics
 * 	@dstats:	Dummy statistics
 * 	@vstats:	Virtual ethernet statistics
 *
 *	@garp_port:	GARP
 *	@mrp_port:	MRP
 *
 *	@dev:		Class/net/name entry
 *	@sysfs_groups:	Space for optional device, statistics and wireless
 *			sysfs groups
 *
 *	@sysfs_rx_queue_group:	Space for optional per-rx queue attributes
 *	@rtnl_link_ops:	Rtnl_link_ops
 *
 *	@gso_max_size:	Maximum size of generic segmentation offload
 *	@gso_max_segs:	Maximum number of segments that can be passed to the
 *			NIC for GSO
 *	@gso_min_segs:	Minimum number of segments that can be passed to the
 *			NIC for GSO
 *
 *	@dcbnl_ops:	Data Center Bridging netlink ops
 *	@num_tc:	Number of traffic classes in the net device
 *	@tc_to_txq:	XXX: need comments on this one
 *	@prio_tc_map	XXX: need comments on this one
 *
 *	@fcoe_ddp_xid:	Max exchange id for FCoE LRO by ddp
 *
 *	@priomap:	XXX: need comments on this one
 *	@phydev:	Physical device may attach itself
 *			for hardware timestamping
 *
 *	@qdisc_tx_busylock:	XXX: need comments on this one
 *
 *	@proto_down:	protocol port state information can be sent to the
 *			switch driver and used to set the phys state of the
 *			switch port.
 *
 *	FIXME: cleanup struct net_device such that network protocol info
 *	moves out.
 */

struct net_device {
    char      	name[IFNAMSIZ];
    int         irq;
	struct net_device_stats stats;
    unsigned int        flags;
    unsigned int        mtu;
    const struct net_device_ops *netdev_ops;
    unsigned short          dev_id;
	void* priv;
};

/**
 *	netdev_priv - access network device private data
 *	@dev: network device
 *
 * Get network device private data
 */
static inline void *netdev_priv(const struct net_device *dev)
{
	return dev->priv;
}

/**
 *	netif_start_queue - allow transmit
 *	@dev: network device
 *
 *	Allow upper layers to call the device hard_start_xmit routine.
 */
void netif_start_queue(struct net_device *dev);

/**
 *	netif_wake_queue - restart transmit
 *	@dev: network device
 *
 *	Allow upper layers to call the device hard_start_xmit routine.
 *	Used for flow control when transmit resources are available.
 */
void netif_wake_queue(struct net_device *dev);

/**
 *	netif_stop_queue - stop transmitted packets
 *	@dev: network device
 *
 *	Stop upper layers calling the device hard_start_xmit routine.
 *	Used for flow control when transmit resources are unavailable.
 */
void netif_stop_queue(struct net_device *dev);

/**
 *	netif_queue_stopped - test if transmit queue is flowblocked
 *	@dev: network device
 *
 *	Test if transmit queue on device is currently unable to send.
 */
bool netif_queue_stopped(const struct net_device *dev);

int netif_rx(struct sk_buff *skb);

void netif_carrier_on(struct net_device *dev);

void netif_carrier_off(struct net_device *dev);

#endif	/* _LINUX_NETDEVICE_H */
