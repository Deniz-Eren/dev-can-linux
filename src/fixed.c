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


void* volatile FixedArray[FIXED_MAX_NUM_BLOCKS];
volatile int FixedArrayIndex = 0;

intrspin_t FixedArraySpin;


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

    memset(&FixedArraySpin, 0, sizeof(intrspin_t));

    return result;
}

void* fixed_malloc (size_t size) {
    void* result = NULL;

    InterruptLock(&FixedArraySpin);
    if (FixedArrayIndex < FIXED_MAX_NUM_BLOCKS) {
        result = FixedArray[FixedArrayIndex++];
    }
    InterruptUnlock(&FixedArraySpin);

    memset(result, 0, size);
    return result;
}

void fixed_free (void* ptr) {
    InterruptLock(&FixedArraySpin);
    FixedArray[--FixedArrayIndex] = ptr;
    InterruptUnlock(&FixedArraySpin);
}
