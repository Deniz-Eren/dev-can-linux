#ifndef __LINUX_COMPILER_H
#define __LINUX_COMPILER_H

# define likely(x)  __builtin_expect(!!(x), 1)
# define unlikely(x)    __builtin_expect(!!(x), 0)

#endif /* __LINUX_COMPILER_H */
