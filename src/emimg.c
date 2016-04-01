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

#define _XOPEN_SOURCE 500

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "emimg.h"

#define EMI_MAGIC "E4IM"

int emi_err;

static const char *emi_error_desc[] = {
/* EMI_E_OK */				"OK",
/* EMI_E_EXISTS */			"image already exists",
/* EMI_E_OPEN */			"cannot open image file",
/* EMI_E_ALLOC */			"cannot allocate memory",
/* EMI_E_HEADER_WRITE */	"error writing header",
/* EMI_E_HEADER_READ */		"error readig header",
/* EMI_E_NO_SECTOR */		"sector not found",
/* EMI_E_READ */			"read error",
/* EMI_E_WRITE */			"write error",
/* EMI_E_MAGIC */			"emi header not found",
/* EMI_E_FORMAT_V_MAJOR */	"image format major version mismatch",
/* EMI_E_FORMAT_V_MINOR */	"image format minor version mismatch",
/* EMI_E_FLAGS */			"unknown flag, flag cannot be changed or mismatched flags",
/* EMI_E_IMG_TYPE */		"unknown image type",
/* EMI_E_WRPROTECT */		"media write protected",
/* EMI_E_ACCESS */			"incompatibile media",
/* EMI_E_GEOM */			"wrong geometry",

/* EMI_E_UNKNOWN */			"unknown error",
};

static int __emi_header_write(struct emi *e);

// -----------------------------------------------------------------------
void emi_close(struct emi *e)
{
	if (!e) return;

	if (e->image) {
		__emi_header_write(e);
		fclose(e->image);
	}
	if (e->img_name) free(e->img_name);
	free(e);
}

// -----------------------------------------------------------------------
const char * emi_get_err(int i)
{
	if (i < 0) i *= -1;
	if (i >= EMI_E_MAX) {
		return emi_error_desc[EMI_E_MAX];
	} else {
		return emi_error_desc[i];
	}
}


// -----------------------------------------------------------------------
const char * emi_get_media_type_name(unsigned i)
{
	static const char *emi_media_type_names[] = {
		"none",
		"hard disk drive",
		"punchar tape",
		"magnetic tape",
		"unknown"
	};

	if (i >= EMI_T_MAX) {
		return emi_media_type_names[EMI_T_MAX];
	} else {
		return emi_media_type_names[i];
	}
}


// -----------------------------------------------------------------------
void emi_header_print(struct emi *e)
{
	printf("Image name   : %s\n", e->img_name);
	printf("Header len   : %u\n", EMI_HEADER_SIZE);
	printf("Magic        : %c%c%c%c\n", e->magic[0], e->magic[1], e->magic[2], e->magic[3]);
	printf("Image ver.   : %u.%u\n", e->v_major, e->v_minor);
	printf("Media type   : %u (%s)\n", e->type, emi_get_media_type_name(e->type));
	printf("Flags        : %s%s%s\n",
		e->flags & EMI_WRPROTECT ? "wrprotect " : "",
		e->flags & EMI_WORM ? "worm " : "",
		e->flags & EMI_USED ? "used " : ""
	);
	if (e->type == EMI_T_MTAPE) {
		printf("Total length : %u\n", e->len);
	}
	if (e->type == EMI_T_DISK) {
		printf("CHS geometry : %u / %u / %u\n", e->cylinders, e->heads, e->spt);
		printf("Block size   : %u bytes\n", e->block_size);
	}
}

// -----------------------------------------------------------------------
static int __emi_header_read(struct emi *e)
{
	uint8_t *buf = calloc(1, EMI_HEADER_SIZE);
	if (!buf) {
		return EMI_E_ALLOC;
	}

	// read data
	if (fseek(e->image, 0, SEEK_SET)) {
		free(buf);
		return EMI_E_HEADER_READ;
	}
	if (fread(buf, 1, EMI_HEADER_SIZE, e->image) !=  EMI_HEADER_SIZE) {
		free(buf);
		return EMI_E_HEADER_READ;
	}

	// unpack data
	uint8_t *pos = buf;
	memcpy(e->magic, pos, 4); pos += 4;
	e->v_major = *pos; pos += 1;
	e->v_minor = *pos; pos += 1;
	e->lib_v_major = *pos; pos += 1;
	e->lib_v_minor = *pos; pos += 1;
	e->lib_v_patch = *pos; pos += 1;
	e->type = ntohs(*(uint16_t*)pos); pos += 2;
	e->flags = ntohl(*(uint32_t*)pos); pos += 4;
	e->cylinders = ntohs(*(uint16_t*)pos); pos += 2;
	e->heads = *pos; pos += 1;
	e->spt = *pos; pos += 1;
	e->block_size = ntohs(*(uint16_t*)pos); pos += 2;
	e->len = ntohl(*(uint32_t*)pos); pos += 4;

	free(buf);
	return EMI_E_OK;
}

// -----------------------------------------------------------------------
static int __emi_header_write(struct emi *e)
{
	uint8_t *buf = calloc(1, EMI_HEADER_SIZE);
	if (!buf) {
		return EMI_E_ALLOC;
	}

	// pack data
	uint8_t *pos = buf;
	memcpy(pos, e->magic, 4); pos += 4;
	*pos = e->v_major; pos += 1;
	*pos = e->v_minor; pos += 1;
	*pos = e->lib_v_major; pos += 1;
	*pos = e->lib_v_minor; pos += 1;
	*pos = e->lib_v_patch; pos += 1;
	*(uint16_t*)pos = htons(e->type); pos += 2;
	*(uint32_t*)pos = htonl(e->flags); pos += 4;
	*(uint16_t*)pos = htons(e->cylinders); pos += 2;
	*pos = e->heads; pos += 1;
	*pos = e->spt; pos += 1;
	*(uint16_t*)pos = htons(e->block_size); pos += 2;
	*(uint32_t*)pos = htonl(e->len); pos += 4;

	// write data
	if (fseek(e->image, 0, SEEK_SET)) {
		free(buf);
		return EMI_E_HEADER_WRITE;
	}
	if (fwrite(buf, 1, EMI_HEADER_SIZE, e->image) !=  EMI_HEADER_SIZE) {
		free(buf);
		return EMI_E_HEADER_WRITE;
	}

	free(buf);
	return EMI_E_OK;
}

// -----------------------------------------------------------------------
int emi_flag_set(struct emi *e, uint32_t flag)
{
	if (flag & !EMI_FLAGS_SETTABLE) {
		return EMI_E_FLAGS;
	}
	e->flags |= flag;
	return EMI_E_OK;
}

// -----------------------------------------------------------------------
int emi_flag_clear(struct emi *e, uint32_t flag)
{
	if (flag & !EMI_FLAGS_SETTABLE) {
		return EMI_E_FLAGS;
	}
	e->flags &= ~flag;
	return EMI_E_OK;
}

// -----------------------------------------------------------------------
static int __emi_header_check(struct emi *e)
{
	// wrong magic
	if (strncmp(e->magic, EMI_MAGIC, 4) != 0) {
		return EMI_E_MAGIC;
	}
	// incompatibile major version
	if (e->v_major != EMI_FORMAT_V_MAJOR) {
		return EMI_E_FORMAT_V_MAJOR;
	}
	// incompatibile minor version (old software, newer image version)
	if (e->v_minor > EMI_FORMAT_V_MINOR) {
		return EMI_E_FORMAT_V_MINOR;
	}
	// unknown flags set
	if (e->flags & !EMI_FLAGS_ALL) {
		return EMI_E_FLAGS;
	}

	return EMI_E_OK;
}

// -----------------------------------------------------------------------
struct emi * emi_open(char *img_name)
{
	int res;
	emi_err = EMI_E_OK;

	struct emi *e = calloc(1, sizeof(struct emi));
	if (!e) {
		emi_err = EMI_E_ALLOC;
		return NULL;
	}
	// open image
	e->image = fopen(img_name, "r+");
	if (!e->image) {
		emi_err = EMI_E_OPEN;
		emi_close(e);
		return NULL;
	}

	// read header
	res = __emi_header_read(e);
	if (res != EMI_E_OK) {
		emi_err = res;
		emi_close(e);
		return NULL;
	}

	// check header
	res = __emi_header_check(e);
	if (res != EMI_E_OK) {
		emi_err = res;
		emi_close(e);
		return NULL;
	}

	e->img_name = strdup(img_name);

	return e;
}

// -----------------------------------------------------------------------
struct emi * emi_create(char *img_name, uint16_t type, uint16_t block_size, uint16_t cylinders, uint8_t heads, uint8_t spt, uint32_t len, uint32_t flags)
{
	int res;

	emi_err = EMI_E_OK;

	// we don't destroy images
	struct stat st;
	if (stat(img_name, &st) == 0) {
		emi_err = EMI_E_EXISTS;
		return NULL;
	}

	struct emi *e = calloc(1, sizeof(struct emi));
	if (!e) {
		emi_err = EMI_E_ALLOC;
		return NULL;
	}

	// fill header data
	strncpy(e->magic, EMI_MAGIC, 4);
	e->v_major = EMI_FORMAT_V_MAJOR;
	e->v_minor = EMI_FORMAT_V_MINOR;
	e->lib_v_major = EMIMG_VERSION_MAJOR;
	e->lib_v_minor = EMIMG_VERSION_MINOR;
	e->lib_v_patch = EMIMG_VERSION_PATCH;
	e->type = type;
	e->flags = flags;
	e->cylinders = cylinders;
	e->heads = heads;
	e->spt = spt;
	e->block_size = block_size;
	e->len = len;
	e->img_name = strdup(img_name);

	// create image file
	e->image = fopen(img_name, "w+");
	if (!e->image) {
		free(e);
		emi_err = EMI_E_OPEN;
		return NULL;
	}

	// write header
	res = __emi_header_write(e);
	if (res != EMI_E_OK) {
		emi_close(e);
		emi_err = res;
		return NULL;
	}

	return e;
}

// vim: tabstop=4 shiftwidth=4 autoindent
