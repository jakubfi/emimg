//  Copyright (c) 2013 Jakub Filipowicz <jakubf@gmail.com>
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

#ifndef E4IMG_H
#define E4IMG_H

#include <stdio.h>
#include <inttypes.h>

extern int emi_err;

enum emi_open_modes {
	EMI_RO = 0b01,
	EMI_WO = 0b10,
	EMI_RW = EMI_RO | EMI_WO,
};

enum emi_errors {
	EMI_E_OK = 0,
	EMI_E_EXISTS,
	EMI_E_OPEN,
	EMI_E_ALLOC,
	EMI_E_HEADER_WRITE,
	EMI_E_HEADER_READ,
	EMI_E_NO_SECTOR,
	EMI_E_READ,
	EMI_E_WRITE,
	EMI_E_MAGIC,
	EMI_E_FORMAT_V_MAJOR,
	EMI_E_FORMAT_V_MINOR,
	EMI_E_FLAGS,
	EMI_E_IMG_TYPE,
	EMI_E_WRPROTECT,
	EMI_E_ACCESS,
	EMI_E_GEOM,

	EMI_E_MAX,
};

enum emi_media_flags {
	EMI_WRPROTECT	= 1 << 0,	// write prohibited
	EMI_WORM		= 1 << 1,	// Write Once Read Many
	EMI_USED		= 1 << 2,	// used (not blank) media
};

#define EMI_FLAGS_ALL		(EMI_WRPROTECT | EMI_WORM | EMI_USED)
#define EMI_FLAGS_SETTABLE	(EMI_WRPROTECT)

enum emi_media_type {
	EMI_T_NONE = 0,		// media type not set
	EMI_T_DISK,			// hard disk drive
	EMI_T_PTAPE,		// punched tape
	EMI_T_MTAPE,		// magnetic tape
	EMI_T_MAX
};

struct emi {
	char magic[4];			// 4
	uint8_t v_major;		// 1
	uint8_t v_minor;		// 1
	uint8_t lib_v_major;	// 1
	uint8_t lib_v_minor;	// 1
	uint8_t lib_v_patch;	// 1
	uint16_t type;			// 2
	uint32_t flags;			// 4

	uint16_t cylinders;		// 2
	uint8_t heads;			// 1
	uint8_t spt;			// 1
	uint16_t block_size;	// 2
	uint32_t len;			// 4
// --------------------------------
#define EMI_HEADER_SIZE		  25
	char *img_name;
	FILE *image;
};

// management
struct emi * emi_open(char *img_name);
void emi_close(struct emi *e);
const char * emi_get_err(int i);
void emi_header_print(struct emi *e);
int emi_flag_set(struct emi *e, uint32_t flag);
int emi_flag_clear(struct emi *e, uint32_t flag);

// disk
struct emi * emi_disk_open(char *img_name);
struct emi * emi_disk_create(char *img_name, uint16_t block_size, uint16_t cylinders, uint8_t heads, uint8_t spt);
int emi_disk_read(struct emi *e, uint8_t *buf, unsigned cyl, unsigned head, unsigned sect);
int emi_disk_write(struct emi *e, uint8_t *buf, unsigned cyl, unsigned head, unsigned sect);

#endif

// vim: tabstop=4 shiftwidth=4 autoindent
