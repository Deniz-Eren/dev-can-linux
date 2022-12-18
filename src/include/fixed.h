/*
 * \file    fixed.h
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

#ifndef SRC_FIXED_H_
#define SRC_FIXED_H_

#include <string.h>

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1 && \
    CONFIG_QNX_INTERRUPT_ATTACH != 1
#include <sys/neutrino.h>
#else /* CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 ||
         CONFIG_QNX_INTERRUPT_ATTACH == 1 */
#include <pthread.h>
#include <unistd.h>
#endif

/*
 * All users must check the types they are allocating are with the block-size
 * limit at compile time using:
 *
 * #include <assert.h>
 *
 * static_assert( sizeof( <user-type> ) <= FIXED_MAX_BLOCK_SIZE,
 *      "<user-type> exceeds fixed block-size" );
 *
 * This requirement will prevent regression.
 */
#define FIXED_MAX_NUM_BLOCKS  256
#define FIXED_MAX_BLOCK_SIZE  32


extern void* volatile FixedArray[FIXED_MAX_NUM_BLOCKS];
extern volatile int FixedArrayIndex;

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1 && \
    CONFIG_QNX_INTERRUPT_ATTACH != 1
extern intrspin_t FixedArraySpin;
#else /* CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 ||
         CONFIG_QNX_INTERRUPT_ATTACH == 1 */
extern pthread_mutex_t FixedArrayMutex;
#endif


/*
 * The initialization function must be called once and only once at the
 * beginning before the fixed_malloc and fixed_free functions are used.
 */
int fixed_memory_init (void);

/*
 * The fixed_malloc function checks if requested more than the maximum number of
 * available blocks and returns NULL on failure.
 */
void* fixed_malloc (size_t size);

/*
 * The fixed_free function must be called for every fixed_malloc pointer
 * returned and ONLY for fixed_malloc returned pointers. No sanity checks are
 * possible as there are interrupt spin locks being used so the user must take
 * necessary precautions.
 */
void fixed_free (void* ptr);

#endif /* SRC_FIXED_H_ */
