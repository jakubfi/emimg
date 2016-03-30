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

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <stdarg.h>

#include "emimg.h"

enum long_opts {
	OPT_PROTECT = 1000,
	OPT_NOPROTECT,
	OPT_HELP,
	OPT_HELP_PRESETS,
};

struct preset {
	int media_type;
	char *name;
	char *description;
	int cyls;
	int heads;
	int spt;
	int sector;
};

static const struct preset known_presets[] = {
	{ EMI_T_DISK, "disk", "Generic disk", 0, 0, 0, 0 },
	{ EMI_T_DISK, "win20a", "Amepol Winchester 20MB", 615, 4, 16, 512 },
	{ EMI_T_DISK, "win20", "Winchester 20MB", 615, 4, 17, 512 },
	{ EMI_T_DISK, "m9425", "MERA 9425 (IBM 5440) 14\" disk", 203, 2, 12, 512 },
	{ EMI_T_DISK, "flop5dsdd", "Floppy 5.25\" DSDD 360KB", 40, 2, 9, 512 },
	{ EMI_T_DISK, "flop5dshd", "Floppy 5.25\" DSHD 1.2MB", 80, 2, 15, 512 },
	{ EMI_T_MTAPE, "mtape", "Magnetic tape", 0, 0, 0, 0 },
	{ EMI_T_PTAPE, "ptape", "Punched tape", 0, 0, 0, 0 },
	{ EMI_T_NONE, NULL, NULL, 0, 0, 0, 0 }
};

static char *image, *src;
static int type, cyls, heads, spt, sector, size;
static int flags_set, flags_clear;

// -----------------------------------------------------------------------
void error(char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	printf("Error: ");
	vprintf(format, ap);
	printf("\nUse --help for help\n");
	va_end(ap);
	exit(1);
}
// -----------------------------------------------------------------------
void print_help_presets()
{
	const struct preset *p = known_presets;

	printf("Known image presets (--preset <name>) (cylinders/heads/sectors/sector size):\n");
	while (p && p->name) {
		printf("  * %-10s %4i /%2i /%2i /%4i : %s\n", p->name, p->cyls, p->heads, p->spt, p->sector, p->description);
		p++;
	}
	printf("\nYou can modify preset parameters with -c, -h, -s, -l and --size\n");
}

// -----------------------------------------------------------------------
void print_help()
{
	printf("emimg %i.%i.%i - media image management tool\n", EMIMG_VERSION_MAJOR, EMIMG_VERSION_MINOR, EMIMG_VERSION_PATCH);
	printf("\nOptions:\n");
	printf("  --help                  : print help\n");
	printf("  --help-preset           : show available media presets\n");
	printf("  --image, -i <filename>  : image file name\n");
	printf("  --preset, -p <name>     : select media preset (see --help-preset)\n");
	printf("  --src, -r <filename>    : import image contents from raw file <filename>\n");
	printf("  --cyls, -c <cylinders>  : number of cylinders\n");
	printf("  --heads, -h <heads>     : number of heads\n");
	printf("  --spt, -s <sectors>     : sectors per track\n");
	printf("  --sector, -l <bytes>    : sector length (bytes)\n");
	printf("  --size <bytes>          : media size (only for magnetic tape)\n");
	printf("  --protect|--no-protect  : set media write-protected/-unprotected\n");
	printf("\nUsage:\n");
	printf("  * Show the header of an existing media:\n");
	printf("      emimg -i <filename>\n");
	printf("  * Create empty disk:\n");
	printf("      emimg -i <filename> -p disk -c <cylinders> -h <heads> -s <sectors_per_track> -l <bytes>\n");
	printf("      emimg -i <filename> -p <name> [-c <cylinders>] [-h <heads>] [-s <sectors_per_track>] [-l <bytes>]\n");
	printf("  * Create new disk and import raw image data:\n");
	printf("      emimg -i <filename> -p disk -r <source> -c <cylinders> -h <heads> -s <sectors> -l <bytes>\n");
	printf("  * Set/clear write protection:\n");
	printf("      emimg -i <filename> --protect|--no-protect\n");
	printf("\n");
}

// -----------------------------------------------------------------------
const struct preset * get_preset(char *name)
{
	const struct preset *p = known_presets;

	while (p && p->name) {
		if (!strcasecmp(name, p->name)) {
			return p;
		}
		p++;
	}

	return NULL;
}

// -----------------------------------------------------------------------
void parse_opts(int argc, char **argv)
{
	int opt, idx;
	const struct preset *p;

	static struct option opts[] = {
		{ "image",		1,	0, 'i' },
		{ "preset",		1,	0, 'p' },
		{ "src",		1,	0, 'r' },
		{ "cyls",		1,	0, 'c' },
		{ "heads",		1,	0, 'h' },
		{ "spt",		1,	0, 's' },
		{ "sector",		1,	0, 'l' },
		{ "protect",	0,	0, OPT_PROTECT },
		{ "no-protect",	0,	0, OPT_NOPROTECT },
		{ "help",		0,	0, OPT_HELP },
		{ "help-preset",0,	0, OPT_HELP_PRESETS},
		{ NULL,			0,	0, 0 }
	};

	while (1) {
		opt = getopt_long(argc, argv,"i:p:r:c:h:s:l:", opts, &idx);
		if (opt == -1) {
			break;
		}
		switch (opt) {
			case OPT_HELP:
				print_help();
				exit(0);
				break;
			case OPT_HELP_PRESETS:
				print_help_presets();
				exit(0);
				break;
			case OPT_PROTECT:
				flags_set = EMI_WRPROTECT;
				break;
			case OPT_NOPROTECT:
				flags_clear = EMI_WRPROTECT;
				break;
			case 'i':
				image = optarg;
				break;
			case 'p':
				p = get_preset(optarg);
				if (!p) {
					error("Media preset '%s' is unknown", optarg);
				} else {
					cyls = p->cyls;
					heads = p->heads;
					spt = p->spt;
					sector = p->sector;
					type = p->media_type;
				}
				break;
			case 'r':
				src = optarg;
				break;
			case 'c':
				cyls = atoi(optarg);
				break;
			case 'h':
				heads = atoi(optarg);
				break;
			case 's':
				spt = atoi(optarg);
				break;
			case 'l':
				sector = atoi(optarg);
				break;
			default:
				error("Wrong usage.");
				break;
		}
	}

	if (!image) {
		error("Image name is required");
	}

	if ((type != EMI_T_MTAPE) && (size != 0)) {
		error("You can only set size for magnetic tape images");
	}

	if ((type != EMI_T_DISK) && (cyls || heads || spt)) {
		error("Options: --cyls, --heads, --spt can be used only for disj images");
	}
}

// -----------------------------------------------------------------------
int import_raw(struct emi *e, char *src_name)
{
	int ret = -1000;
	int bufsize = e->block_size;
	uint8_t *buf = calloc(bufsize, 1);

	if (!buf) {
		printf("Cannot allocate memory for image import buffer\n");
		goto fin;
	}

	// open
	FILE *source = fopen(src_name, "r");
	if (!source) {
		printf("Cannot open source image \"%s\"\n", src_name);
		goto fin;
	}

	if (fseek(source, 0, SEEK_END)) {
		printf("Seek failed for source image \"%s\"\n", src_name);
		goto fin;
	}

	long source_len = ftell(source);

	// check source size
	if (source_len != e->cylinders * e->heads * e->spt * e->block_size) {
		printf("Source image \"%s\" size %li is not equal to disk image capacity.\n", src_name, source_len);
		goto fin;
	}

	if (fseek(source, 0, SEEK_SET)) {
		printf("Seek failed for source image \"%s\"\n", src_name);
		goto fin;
	}

	for (int c=0 ; c<e->cylinders ; c++) {
		for (int h=0 ; h<e->heads ; h++) {
			for (int s=0 ; s<e->spt ; s++) {
				if (fread(buf, 1, bufsize, source) != bufsize) {
					printf("Source image \"%s\" read failed.\n", src_name);
					goto fin;
				}
				ret = emi_disk_write(e, buf, c, h, s);
				if (ret != EMI_E_OK) {
					printf("Image write failed during import of source image \"%s\"\n", src_name);
					goto fin;
				}
			}
		}
	}

	ret = EMI_E_OK;

fin:
	if (source) fclose(source);
	free(buf);

	return ret;
}

// -----------------------------------------------------------------------
// ---- MAIN -------------------------------------------------------------
// -----------------------------------------------------------------------
int main(int argc, char **argv)
{
	struct emi *e = NULL;
	int res;

	parse_opts(argc, argv);

	// create media?
	if (type != 0) {
		// create as requested
		if (type == EMI_T_DISK) {
			e = emi_disk_create(image, sector, cyls, heads, spt);
		} else {
		}

		// ok?
		if (!e) {
			error("Could not create image: %s", emi_get_err(emi_err));
		}

		// source given?
		if (src) {
			res = import_raw(e, src);
			if (res == EMI_E_OK) {
				printf("Image contents imported.\n");
			} else {
				error("Could not import image contents.");
			}
		}

		printf("Image ready.\n");

	} else {
		e = emi_open(image);
		if (!e) {
			error("Could not open image: %s", emi_get_err(emi_err));
		}
	}

	// any flags to set?
	if (flags_set) {
		res = emi_flag_set(e, flags_set);
		if (res != EMI_E_OK) {
			error("Could not set flags: %s", emi_get_err(res));
		}
	}
	// any flags to clear?
	if (flags_clear) {
		res = emi_flag_clear(e, flags_clear);
		if (res != EMI_E_OK) {
			error("Could not clear flags: %s", emi_get_err(res));
		}
	}

	emi_header_print(e);
	emi_close(e);

	return 0;
}

// vim: tabstop=4 shiftwidth=4 autoindent
