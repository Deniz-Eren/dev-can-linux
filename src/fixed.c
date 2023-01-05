/*
 * \file    fixed.c
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

#include <stdlib.h>

#include <fixed.h>
#include <logs.h>


void* volatile FixedArray[FIXED_MAX_NUM_BLOCKS];
volatile int FixedArrayIndex = 0;

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1 && \
    CONFIG_QNX_INTERRUPT_ATTACH != 1
intrspin_t FixedArraySpin;
#else /* CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 ||
         CONFIG_QNX_INTERRUPT_ATTACH == 1 */
pthread_mutex_t FixedArrayMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

int fixed_memory_init (void) {
    int result = 0;

    int i;
    for (i = 0; i < FIXED_MAX_NUM_BLOCKS; ++i) {
        if ((FixedArray[i] = malloc(FIXED_MAX_BLOCK_SIZE)) == NULL) {
            int j;
            for (j = 0; j < i; ++j) {
                free(FixedArray[j]);
            }

            result = -1;
            break;
        }
    }

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1 && \
    CONFIG_QNX_INTERRUPT_ATTACH != 1
    memset(&FixedArraySpin, 0, sizeof(intrspin_t));
#endif

    return result;
}

void* fixed_malloc (size_t size) {
    void* result = NULL;

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1 && \
    CONFIG_QNX_INTERRUPT_ATTACH != 1
    InterruptLock(&FixedArraySpin);
#else /* CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 ||
         CONFIG_QNX_INTERRUPT_ATTACH == 1 */
    int ret_code;

    do {
        ret_code = pthread_mutex_lock(&FixedArrayMutex);

        if (ret_code != EOK) {
            log_dbg("fixed_malloc: mutex lock error (%s)\n", strerror(ret_code));

            usleep(1);
        }

    } while (ret_code != EOK);
#endif

    if (FixedArrayIndex < FIXED_MAX_NUM_BLOCKS) {
        result = FixedArray[FixedArrayIndex++];
    }

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1 && \
    CONFIG_QNX_INTERRUPT_ATTACH != 1
    InterruptUnlock(&FixedArraySpin);
#else /* CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 ||
         CONFIG_QNX_INTERRUPT_ATTACH == 1 */
    ret_code = pthread_mutex_unlock(&FixedArrayMutex);

    if (ret_code != EOK) {
        log_dbg("fixed_malloc: mutex unlock error (%s)\n", strerror(ret_code));
    }
#endif

    if (result == NULL) {
        log_err("fixed_malloc: BUG! exceeded FIXED_MAX_NUM_BLOCKS !\n");

        return NULL;
    }

    memset(result, 0, size);
    return result;
}

void fixed_free (void* ptr) {
#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1 && \
    CONFIG_QNX_INTERRUPT_ATTACH != 1
    InterruptLock(&FixedArraySpin);
#else /* CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 ||
         CONFIG_QNX_INTERRUPT_ATTACH == 1 */
    int ret_code;

    do {
        ret_code = pthread_mutex_lock(&FixedArrayMutex);

        if (ret_code != EOK) {
            log_dbg("fixed_free: mutex lock error (%s)\n", strerror(ret_code));

            usleep(1);
        }

    } while (ret_code != EOK);
#endif

    FixedArray[--FixedArrayIndex] = ptr;

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1 && \
    CONFIG_QNX_INTERRUPT_ATTACH != 1
    InterruptUnlock(&FixedArraySpin);
#else /* CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 ||
         CONFIG_QNX_INTERRUPT_ATTACH == 1 */
    ret_code = pthread_mutex_unlock(&FixedArrayMutex);

    if (ret_code != EOK) {
        log_dbg("fixed_free: mutex unlock error (%s)\n", strerror(ret_code));
    }
#endif
}
