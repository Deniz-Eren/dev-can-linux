/* SPDX-License-Identifier: GPL-2.0 */
/*
 * \file    linux/stddef.h
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          been modified very slightly to integrate to QNX RTOS.
 *
 * Please also check the "SPDX-License-Identifier" documentation from the Linux
 * Kernel source code repository: github.com/torvalds/linux.git for further
 * details.
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

#ifndef _LINUX_STDDEF_H
#define _LINUX_STDDEF_H

#ifndef __QNX__
#include <uapi/linux/stddef.h>
#endif

#undef NULL
#define NULL ((void *)0)

#ifdef __QNX__
#include <stdbool.h>
#else
enum {
	false	= 0,
	true	= 1
};
#endif

#undef offsetof
#define offsetof(TYPE, MEMBER)	__builtin_offsetof(TYPE, MEMBER)

/**
 * sizeof_field() - Report the size of a struct field in bytes
 *
 * @TYPE: The structure containing the field of interest
 * @MEMBER: The field to return the size of
 */
#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))

/**
 * offsetofend() - Report the offset of a struct field within the struct
 *
 * @TYPE: The type of the structure
 * @MEMBER: The member within the structure to get the end offset of
 */
#define offsetofend(TYPE, MEMBER) \
	(offsetof(TYPE, MEMBER)	+ sizeof_field(TYPE, MEMBER))

/**
 * struct_group() - Wrap a set of declarations in a mirrored struct
 *
 * @NAME: The identifier name of the mirrored sub-struct
 * @MEMBERS: The member declarations for the mirrored structs
 *
 * Used to create an anonymous union of two structs with identical
 * layout and size: one anonymous and one named. The former can be
 * used normally without sub-struct naming, and the latter can be
 * used to reason about the start, end, and size of the group of
 * struct members.
 */
#define struct_group(NAME, MEMBERS...)	\
	__struct_group(/* no tag */, NAME, /* no attrs */, MEMBERS)

/**
 * struct_group_attr() - Create a struct_group() with trailing attributes
 *
 * @NAME: The identifier name of the mirrored sub-struct
 * @ATTRS: Any struct attributes to apply
 * @MEMBERS: The member declarations for the mirrored structs
 *
 * Used to create an anonymous union of two structs with identical
 * layout and size: one anonymous and one named. The former can be
 * used normally without sub-struct naming, and the latter can be
 * used to reason about the start, end, and size of the group of
 * struct members. Includes structure attributes argument.
 */
#define struct_group_attr(NAME, ATTRS, MEMBERS...) \
	__struct_group(/* no tag */, NAME, ATTRS, MEMBERS)

/**
 * struct_group_tagged() - Create a struct_group with a reusable tag
 *
 * @TAG: The tag name for the named sub-struct
 * @NAME: The identifier name of the mirrored sub-struct
 * @MEMBERS: The member declarations for the mirrored structs
 *
 * Used to create an anonymous union of two structs with identical
 * layout and size: one anonymous and one named. The former can be
 * used normally without sub-struct naming, and the latter can be
 * used to reason about the start, end, and size of the group of
 * struct members. Includes struct tag argument for the named copy,
 * so the specified layout can be reused later.
 */
#define struct_group_tagged(TAG, NAME, MEMBERS...) \
	__struct_group(TAG, NAME, /* no attrs */, MEMBERS)

/**
 * DECLARE_FLEX_ARRAY() - Declare a flexible array usable in a union
 *
 * @TYPE: The type of each flexible array element
 * @NAME: The name of the flexible array member
 *
 * In order to have a flexible array member in a union or alone in a
 * struct, it needs to be wrapped in an anonymous struct with at least 1
 * named member, but that member can be empty.
 */
#define DECLARE_FLEX_ARRAY(TYPE, NAME) \
	__DECLARE_FLEX_ARRAY(TYPE, NAME)

#endif
