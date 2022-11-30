/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Global definitions for the INET interface module.
 *
 * Version:	@(#)if.h	1.0.2	04/18/93
 *
 * Authors:	Original taken from Berkeley UNIX 4.3, (c) UCB 1982-1988
 *		Ross Biro
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#ifndef _LINUX_IF_H
#define _LINUX_IF_H

//#include <linux/libc-compat.h>          /* for compatibility with glibc */
#include "linux/types.h"		/* for "__kernel_caddr_t" et al	*/
//#include <linux/socket.h>		/* for "struct sockaddr" et al	*/
//#include <linux/compiler.h>		/* for "__user" et al           */

#ifdef __UAPI_DEF_IF_IFNAMSIZ
#define	IFNAMSIZ	16
#endif /* __UAPI_DEF_IF_IFNAMSIZ */
#define	IFALIASZ	256
//#include <linux/hdlc/ioctl.h>

/**
 * enum net_device_flags - &struct net_device flags
 *
 * These are the &struct net_device flags, they can be set by drivers, the
 * kernel and some can be triggered by userspace. Userspace can query and
 * set these flags using userspace utilities but there is also a sysfs
 * entry available for all dev flags which can be queried and set. These flags
 * are shared for all types of net_devices. The sysfs entries are available
 * via /sys/class/net/<dev>/flags. Flags which can be toggled through sysfs
 * are annotated below, note that only a few flags can be toggled and some
 * other flags are always always preserved from the original net_device flags
 * even if you try to set them via sysfs. Flags which are always preserved
 * are kept under the flag grouping @IFF_VOLATILE. Flags which are volatile
 * are annotated below as such.
 *
 * You should have a pretty good reason to be extending these flags.
 *
 * @IFF_UP: interface is up. Can be toggled through sysfs.
 * @IFF_BROADCAST: broadcast address valid. Volatile.
 * @IFF_DEBUG: turn on debugging. Can be toggled through sysfs.
 * @IFF_LOOPBACK: is a loopback net. Volatile.
 * @IFF_POINTOPOINT: interface is has p-p link. Volatile.
 * @IFF_NOTRAILERS: avoid use of trailers. Can be toggled through sysfs.
 *	Volatile.
 * @IFF_RUNNING: interface RFC2863 OPER_UP. Volatile.
 * @IFF_NOARP: no ARP protocol. Can be toggled through sysfs. Volatile.
 * @IFF_PROMISC: receive all packets. Can be toggled through sysfs.
 * @IFF_ALLMULTI: receive all multicast packets. Can be toggled through
 *	sysfs.
 * @IFF_MASTER: master of a load balancer. Volatile.
 * @IFF_SLAVE: slave of a load balancer. Volatile.
 * @IFF_MULTICAST: Supports multicast. Can be toggled through sysfs.
 * @IFF_PORTSEL: can set media type. Can be toggled through sysfs.
 * @IFF_AUTOMEDIA: auto media select active. Can be toggled through sysfs.
 * @IFF_DYNAMIC: dialup device with changing addresses. Can be toggled
 *	through sysfs.
 * @IFF_LOWER_UP: driver signals L1 up. Volatile.
 * @IFF_DORMANT: driver signals dormant. Volatile.
 * @IFF_ECHO: echo sent packets. Volatile.
 */
enum net_device_flags {
/* for compatibility with glibc net/if.h */
#ifdef __UAPI_DEF_IF_NET_DEVICE_FLAGS
	IFF_UP				= 1<<0,  /* sysfs */
	IFF_BROADCAST			= 1<<1,  /* volatile */
	IFF_DEBUG			= 1<<2,  /* sysfs */
	IFF_LOOPBACK			= 1<<3,  /* volatile */
	IFF_POINTOPOINT			= 1<<4,  /* volatile */
	IFF_NOTRAILERS			= 1<<5,  /* sysfs */
	IFF_RUNNING			= 1<<6,  /* volatile */
	IFF_NOARP			= 1<<7,  /* sysfs */
	IFF_PROMISC			= 1<<8,  /* sysfs */
	IFF_ALLMULTI			= 1<<9,  /* sysfs */
	IFF_MASTER			= 1<<10, /* volatile */
	IFF_SLAVE			= 1<<11, /* volatile */
	IFF_MULTICAST			= 1<<12, /* sysfs */
	IFF_PORTSEL			= 1<<13, /* sysfs */
	IFF_AUTOMEDIA			= 1<<14, /* sysfs */
	IFF_DYNAMIC			= 1<<15, /* sysfs */
#endif /* __UAPI_DEF_IF_NET_DEVICE_FLAGS */
#ifdef __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO
	IFF_LOWER_UP			= 1<<16, /* volatile */
	IFF_DORMANT			= 1<<17, /* volatile */
	IFF_ECHO			= 1<<18, /* volatile */
#endif /* __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO */
};

#endif /* _LINUX_IF_H */
