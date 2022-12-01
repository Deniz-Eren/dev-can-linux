#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <stdarg.h>
#include <linux/types.h>
#include <linux/compiler.h>

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max/clamp at all, of course.
 */
#define min_t(type, x, y) ({			\
	type __min1 = (x);			\
	type __min2 = (y);			\
	__min1 < __min2 ? __min1: __min2; })

#define max_t(type, x, y) ({			\
	type __max1 = (x);			\
	type __max2 = (y);			\
	__max1 > __max2 ? __max1: __max2; })

#endif
