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
#include <arpa/inet.h>

#include "emimg.h"

enum emi_mtape_block_types {
	EMI_MT_DATA,
	EMI_MT_BOT,
	EMI_MT_EOT,
	EMI_MT_EOF,
	EMI_MT_ERASED,
	EMI_MT_MAX
};

struct emi_mtape_header {
	uint8_t type;
	uint16_t size;
};

#define EM_MT_HDR_SIZE sizeof(struct emi_mtape_header)

struct emi * emi_open(char *img_name);
void emi_close(struct emi *e);
struct emi * emi_create(char *img_name, uint16_t type, uint16_t block_size, uint16_t cylinders, uint8_t heads, uint8_t spt, uint32_t len, uint32_t flags);

// -----------------------------------------------------------------------
static int emi_mtape_header_read(struct emi *e, struct emi_mtape_header *hdr)
{
	if (fread(e->hbuf, 1, EM_MT_HDR_SIZE, e->image) != EM_MT_HDR_SIZE) {
		return -EMI_E_HEADER_READ;
	}

	uint8_t *pos = e->hbuf;
	hdr->type = *pos; pos += 1;
	hdr->size = ntohs(*(uint16_t*)pos); pos += 2;

	return EMI_E_OK;
}

// -----------------------------------------------------------------------
static int emi_mtape_header_write(struct emi *e, struct emi_mtape_header *hdr)
{
	uint8_t *pos = e->hbuf;
	*pos = hdr->type; pos += 1;
	*(uint16_t*)pos = htons(hdr->size); pos += 2;

	if (fwrite(e->hbuf, 1, EM_MT_HDR_SIZE, e->image) != EM_MT_HDR_SIZE) {
		return -EMI_E_HEADER_WRITE;
	}

	return EMI_E_OK;
}

// -----------------------------------------------------------------------
static int emi_mtape_full(struct emi *e)
{
	long pos = ftell(e->image);
	if (pos > e->len) {
		return 1;
	}
	return 0;
}

// -----------------------------------------------------------------------
struct emi * emi_mtape_open(char *img_name)
{
	int res;

	struct emi *e = emi_open(img_name);

	// magnetic tape tape?
	if (e->type != EMI_T_MTAPE) {
		emi_close(e);
		emi_err = -EMI_E_IMG_TYPE;
		return NULL;
	}

	// seek to tape start
	res = emi_mtape_bot(e);
	if (res != EMI_E_OK) {
		emi_close(e);
		emi_err = res;
		return NULL;
	}

	return e;
}

// -----------------------------------------------------------------------
void emi_mtape_close(struct emi *e)
{
	emi_close(e);
}

// -----------------------------------------------------------------------
struct emi * emi_mtape_create(char *img_name, uint32_t size)
{
	struct emi *e;
	int res;
	struct emi_mtape_header hdr;

	// create image
	e = emi_create(img_name, EMI_T_MTAPE, 0, 0, 0, 0, size, 0);
	if (!e) {
		return NULL;
	}

	// write BOT block
	hdr.type = EMI_MT_BOT;
	hdr.size = 0;
	res = emi_mtape_header_write(e, &hdr);
	if (res != EMI_E_OK) {
		emi_err = res;
		emi_close(e);
		return NULL;
	}

	// write EOT block
	hdr.type = EMI_MT_EOT;
	res = emi_mtape_header_write(e, &hdr);
	if (res != EMI_E_OK) {
		emi_err = res;
		emi_close(e);
		return NULL;
	}

	// seek to tape start
	res = emi_mtape_bot(e);
	if (res != EMI_E_OK) {
		emi_err = -EMI_E_SEEK;
		emi_close(e);
		return NULL;
	}

	return e;
}

// -----------------------------------------------------------------------
int emi_mtape_read(struct emi *e, uint8_t *buf)
{
	int res;
	struct emi_mtape_header hdr;

	if (e->type != EMI_T_MTAPE) {
		return -EMI_E_ACCESS;
	}

	// read block header
	res = emi_mtape_header_read(e, &hdr);
	if (res != EMI_E_OK) {
		return res;
	}

	switch (hdr.type) {
		case EMI_MT_DATA:
			res = fread(buf, 1, hdr.size, e->image);
			if (res != hdr.size) {
				return -EMI_E_READ;
			}
			// skip to next block start
			res = fseek(e->image, EM_MT_HDR_SIZE, SEEK_CUR);
			if (res < 0) {
				return -EMI_E_SEEK;
			}
			break;
		case EMI_MT_EOF:
			return -EMI_E_EOF;
		case EMI_MT_BOT:
			return -EMI_E_BOT;
		case EMI_MT_EOT:
			return -EMI_E_EOT;
		default:
			return -EMI_E_READ;
	}

	return hdr.size;
}

// -----------------------------------------------------------------------
int emi_mtape_write(struct emi *e, uint8_t *buf, unsigned size)
{
	int res;
	struct emi_mtape_header hdr;

	if (e->type != EMI_T_MTAPE) {
		return -EMI_E_ACCESS;
	}

	if (e->flags & EMI_WRPROTECT) {
		return -EMI_E_WRPROTECT;
	}

	// tape full?
	if (emi_mtape_full(e)) {
		return -EMI_E_EOT;
	}

	hdr.type = EMI_MT_DATA;
	hdr.size = size;

	// write header
	res = emi_mtape_header_write(e, &hdr);
	if (res != EMI_E_OK) {
		return -EMI_E_WRITE;
	}

	// write data
	res = fwrite(buf, 1, hdr.size, e->image);
	if (res != hdr.size) {
		return -EMI_E_WRITE;
	}

	// write footer
	res = emi_mtape_header_write(e, &hdr);
	if (res != EMI_E_OK) {
		return -EMI_E_WRITE;
	}

	// write EOT
	hdr.type = EMI_MT_EOT;
	hdr.size = 0;
	res = emi_mtape_header_write(e, &hdr);
	if (res != EMI_E_OK) {
		return -EMI_E_WRITE;
	}

	// rewind before EOT
	res = fseek(e->image, -EM_MT_HDR_SIZE, SEEK_CUR);
	if (res < 0) {
		return -EMI_E_SEEK;
	}

	return EMI_E_OK;
}

// -----------------------------------------------------------------------
int emi_mtape_write_eof(struct emi *e)
{
	int res;
	struct emi_mtape_header hdr;

	if (e->type != EMI_T_MTAPE) {
		return -EMI_E_ACCESS;
	}

	// tape full?
	if (emi_mtape_full(e)) {
		return -EMI_E_EOT;
	}

	if (e->flags & EMI_WRPROTECT) {
		return -EMI_E_WRPROTECT;
	}

	// write header
	hdr.type = EMI_MT_EOF;
	hdr.size = 0;
	res = emi_mtape_header_write(e, &hdr);
	if (res != EMI_E_OK) {
		return -EMI_E_WRITE;
	}

	// write EOT
	hdr.type = EMI_MT_EOT;
	hdr.size = 0;
	res = emi_mtape_header_write(e, &hdr);
	if (res != EMI_E_OK) {
		return -EMI_E_WRITE;
	}

	return EMI_E_OK;
}

// -----------------------------------------------------------------------
int emi_mtape_fwd(struct emi *e)
{
	int res;
	struct emi_mtape_header hdr;

	if (e->type != EMI_T_MTAPE) {
		return -EMI_E_ACCESS;
	}

	// read block header
	res = emi_mtape_header_read(e, &hdr);
	if (res != EMI_E_OK) {
		return res;
	}

	switch (hdr.type) {
		case EMI_MT_DATA:
			// skip to next block header
			res = fseek(e->image, hdr.size + EM_MT_HDR_SIZE, SEEK_CUR);
			if (res < 0) {
				return -EMI_E_SEEK;
			}
			break;
		case EMI_MT_EOF:
			break;
		case EMI_MT_EOT:
			// skip back to EOT block header
			res = fseek(e->image, -EM_MT_HDR_SIZE, SEEK_CUR);
			if (res < 0) {
				return -EMI_E_SEEK;
			}
			return -EMI_E_EOT;
		case EMI_MT_BOT:
			// shouldn't happen
			return -EMI_E_BOT;
	}

	return EMI_E_OK;
}

// -----------------------------------------------------------------------
int emi_mtape_rew(struct emi *e)
{
	int res;
	struct emi_mtape_header hdr;

	// skip to previous block footer
	res = fseek(e->image, -EM_MT_HDR_SIZE, SEEK_CUR);
	if (res < 0) {
		return -EMI_E_SEEK;
	}

	// read block footer
	res = emi_mtape_header_read(e, &hdr);
	if (res != EMI_E_OK) {
		return res;
	}

	int seek = 0;

	switch (hdr.type) {
		case EMI_MT_BOT:
			return -EMI_E_BOT;
		case EMI_MT_EOT:
			// shouldn't happen
			return -EMI_E_EOT;
		case EMI_MT_EOF:
			seek = EM_MT_HDR_SIZE;
			break;
		case EMI_MT_DATA:
			// + data block
			seek = EM_MT_HDR_SIZE + hdr.size + EM_MT_HDR_SIZE;
			break;
	}

	// skip to previous block start
	res = fseek(e->image, -seek, SEEK_CUR);
	if (res < 0) {
		return -EMI_E_SEEK;
	}

	return EMI_E_OK;
}

// -----------------------------------------------------------------------
int emi_mtape_bot(struct emi *e)
{
	int res;

	// seek to tape start
	res = fseek(e->image, EMI_HEADER_SIZE+EM_MT_HDR_SIZE, SEEK_SET);
	if (res < 0) {
		return -EMI_E_SEEK;
	}

	return EMI_E_OK;
}

// vim: tabstop=4 shiftwidth=4 autoindent
