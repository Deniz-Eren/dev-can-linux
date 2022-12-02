/*
 * Written by Mark Hemment, 1996 (markhe@nextd.demon.co.uk).
 *
 * (C) SGI 2006, Christoph Lameter
 * 	Cleaned up and restructured to ease the addition of alternative
 * 	implementations of SLAB allocators.
 * (C) Linux Foundation 2008-2013
 *      Unified interface for all slab allocators
 */

#ifndef _LINUX_SLAB_H
#define	_LINUX_SLAB_H

#include <linux/types.h>


/* Define mapping of kzalloc to simply malloc */
#define kzalloc(size, gfp) malloc(size)
#define kfree(ptr) free(ptr)

#endif	/* _LINUX_SLAB_H */
