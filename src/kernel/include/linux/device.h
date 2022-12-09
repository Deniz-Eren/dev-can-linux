/*
 * \file    linux/device.h
 * \brief   Defines simplified 'struct device' and some of its functions.
 *
 * \details This file has the exact same name and path as the one supplied by
 *          the Linux Kernel source-code, however the contents have been
 *          completely replaced. This has been done to keep the rest of the
 *          Linux Kernel source-code unchanged so that ongoing updates to the
 *          Linux Kernel can be propagated to this project easily.
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

#ifndef _DEVICE_H_
#define _DEVICE_H_


/**
 * struct device - The basic device structure
 * @driver_data: Private pointer for driver specific info.
 */
struct device {
	void		*driver_data;	/* Driver data, set and get with
					   dev_set_drvdata/dev_get_drvdata */
};

static inline void *dev_get_drvdata(const struct device *dev)
{
	return dev->driver_data;
}

static inline void dev_set_drvdata(struct device *dev, void *data)
{
	dev->driver_data = data;
}

#endif /* _DEVICE_H_ */
