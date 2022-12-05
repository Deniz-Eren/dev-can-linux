#ifndef _LINUX_BITOPS_H
#define _LINUX_BITOPS_H

#include <asm-generic/bitops/fls64.h>


static inline unsigned fls_long(unsigned long l)
{
	if (sizeof(l) == 4)
		return fls(l);
	return fls64(l);
}

#endif
