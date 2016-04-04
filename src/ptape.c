//  Copyright (c)			break; 2016 Jakub Filipowicz <jakubf@gmail.com>
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
void emi_close(struct emi *e);
struct emi * emi_create(char *img_name, uint16_t type, uint16_t block_size, uint16_t cylinders, uint8_t heads, uint8_t spt, uint32_t len, uint32_t flags);

// -----------------------------------------------------------------------
struct emi * emi_ptape_open(char *img_name)
{
	struct emi *e = emi_open(img_name);

	// punched tape?
	if (e->type != EMI_T_PTAPE) {
		emi_close(e);
		emi_err = -EMI_E_IMG_TYPE;
		return NULL;
	}

	return e;
}

// -----------------------------------------------------------------------
void emi_ptape_close(struct emi *e)
{
	if (e->len != 0)  {
		e->flags |= EMI_USED;
	}
	emi_close(e);
}

// -----------------------------------------------------------------------
struct emi * emi_ptape_create(char *img_name)
{
	struct emi *e;

	// create image
	e = emi_create(img_name, EMI_T_PTAPE, 0, 0, 0, 0, 0, 0);
	if (!e) {
		return NULL;
	}

	return e;
}

// -----------------------------------------------------------------------
int emi_ptape_read(struct emi *e)
{
	int res;
	uint8_t data;

	if (e->type != EMI_T_PTAPE) {
		return -EMI_E_ACCESS;
	}

	res = fread(&data, 1, 1, e->image);
	if (res != 1) {
		if (feof(e->image)) {
			return -EMI_E_EOF;
		}
		return -EMI_E_READ;
	}

	return data;
}

// -----------------------------------------------------------------------
int emi_ptape_write(struct emi *e, uint8_t data)
{
	int res;

	if (e->type != EMI_T_PTAPE) {
		return -EMI_E_ACCESS;
	}

	if ((e->flags & EMI_WRPROTECT) || ((e->flags & (EMI_WORM | EMI_USED)) == (EMI_WORM | EMI_USED))) {
		return -EMI_E_WRPROTECT;
	}

	// write data
	res = fwrite(&data, 1, 1, e->image);
	if (res != 1) {
		return -EMI_E_WRITE;
	}

	e->len += 1;

	return EMI_E_OK;
}

// vim: tabstop=4 shiftwidth=4 autoindent
