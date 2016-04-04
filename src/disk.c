//  Copyright (c) 2013-2016 Jakub Filipowicz <jakubf@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "emimg.h"

struct emi * emi_open(char *img_name);
struct emi * emi_create(char *img_name, uint16_t type, uint16_t block_size, uint16_t cylinders, uint8_t heads, uint8_t spt, uint32_t len, uint32_t flags);

// -----------------------------------------------------------------------
struct emi * emi_disk_open(char *img_name)
{
	struct emi *e = emi_open(img_name);

	if ((e->cylinders <= 0) || (e->heads <= 0) || (e->spt <= 0) || (e->block_size <= 0)) {
		emi_close(e);
		e = NULL;
		emi_err = -EMI_E_GEOM;
	}

	if (e->type != EMI_T_DISK) {
		emi_close(e);
		e = NULL;
		emi_err = -EMI_E_IMG_TYPE;
	}

	return e;
}

// -----------------------------------------------------------------------
struct emi * emi_disk_create(char *img_name, uint16_t block_size, uint16_t cylinders, uint8_t heads, uint8_t spt)
{
	if ((cylinders <= 0) || (heads <= 0) || (spt <= 0) || (block_size <= 0)) {
		emi_err = -EMI_E_GEOM;
		return NULL;
	}

	return emi_create(img_name, EMI_T_DISK, block_size, cylinders, heads, spt, 0, 0);
}

// -----------------------------------------------------------------------
static int chs2offset(struct emi *e, int cyl, int head, int sect)
{
	return e->block_size * (sect + (head * e->spt) + (cyl * e->heads * e->spt));
}

// -----------------------------------------------------------------------
int emi_disk_read(struct emi *e, uint8_t *buf, unsigned cyl, unsigned head, unsigned sect)
{
	int res;

	if (e->type != EMI_T_DISK) {
		return -EMI_E_ACCESS;
	}

	if ((cyl >= e->cylinders) || (head >= e->heads) || (sect >= e->spt)) {
		return -EMI_E_SEEK;
	}

	res = fseek(e->image, EMI_HEADER_SIZE + chs2offset(e, cyl, head, sect), SEEK_SET);
	if (res < 0) {
		return -EMI_E_SEEK;
	}

	res = fread(buf, 1, e->block_size, e->image);
	if (res != e->block_size) {
		return -EMI_E_READ;
	}

	return EMI_E_OK;
}

// -----------------------------------------------------------------------
int emi_disk_write(struct emi *e, uint8_t *buf, unsigned cyl, unsigned head, unsigned sect)
{
	int res;

	if (e->type != EMI_T_DISK) {
		return -EMI_E_ACCESS;
	}

	if ((cyl >= e->cylinders) || (head >= e->heads) || (sect >= e->spt)) {
		return -EMI_E_SEEK;
	}

	if (e->flags & EMI_WRPROTECT) {
		return -EMI_E_WRPROTECT;
	}

	res = fseek(e->image, EMI_HEADER_SIZE + chs2offset(e, cyl, head, sect), SEEK_SET);
	if (res < 0) {
		return -EMI_E_SEEK;
	}

	res = fwrite(buf, 1, e->block_size, e->image);
	if (res != e->block_size) {
		return -EMI_E_WRITE;
	}

	return EMI_E_OK;
}

// vim: tabstop=4 shiftwidth=4 autoindent
