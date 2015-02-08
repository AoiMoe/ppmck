#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"
#include "externs.h"
#include "protos.h"
#include "pce.h"

static void pce_write_header(FILE *f, int banks);
static int  pce_pack_8x8_tile(unsigned char *buffer, void *data, int line_offset, int format);
static int  pce_pack_16x16_tile(unsigned char *buffer, void *data, int line_offset, int format);
static int  pce_pack_16x16_sprite(unsigned char *buffer, void *data, int line_offset, int format);
static void pce_defchr(int *ip);
static void pce_defpal(int *ip);
static void pce_defspr(int *ip);
static void pce_incbat(int *ip);
static void pce_incpal(int *ip);
static void pce_incspr(int *ip);
static void pce_inctile(int *ip);
static void pce_incmap(int *ip);
static void pce_vram(int *ip);
static void pce_pal(int *ip);
static void pce_mml(int *ip);

/* PCE specific instructions */
static struct t_opcode pce_inst[] = {
	{NULL, "BBR",  class10,0, 0x0F, 0},
	{NULL, "BBR0", class5, 0, 0x0F, 0},
	{NULL, "BBR1", class5, 0, 0x1F, 0},
	{NULL, "BBR2", class5, 0, 0x2F, 0},
	{NULL, "BBR3", class5, 0, 0x3F, 0},
	{NULL, "BBR4", class5, 0, 0x4F, 0},
	{NULL, "BBR5", class5, 0, 0x5F, 0},
	{NULL, "BBR6", class5, 0, 0x6F, 0},
	{NULL, "BBR7", class5, 0, 0x7F, 0},
	{NULL, "BBS",  class10,0, 0x8F, 0},
	{NULL, "BBS0", class5, 0, 0x8F, 0},
	{NULL, "BBS1", class5, 0, 0x9F, 0},
	{NULL, "BBS2", class5, 0, 0xAF, 0},
	{NULL, "BBS3", class5, 0, 0xBF, 0},
	{NULL, "BBS4", class5, 0, 0xCF, 0},
	{NULL, "BBS5", class5, 0, 0xDF, 0},
	{NULL, "BBS6", class5, 0, 0xEF, 0},
	{NULL, "BBS7", class5, 0, 0xFF, 0},
	{NULL, "BRA",  class2, 0, 0x80, 0},
	{NULL, "BSR",  class2, 0, 0x44, 0},
	{NULL, "CLA",  class1, 0, 0x62, 0},
	{NULL, "CLX",  class1, 0, 0x82, 0},
	{NULL, "CLY",  class1, 0, 0xC2, 0},
	{NULL, "CSH",  class1, 0, 0xD4, 0},
	{NULL, "CSL",  class1, 0, 0x54, 0},
	{NULL, "PHX",  class1, 0, 0xDA, 0},
	{NULL, "PHY",  class1, 0, 0x5A, 0},
	{NULL, "PLX",  class1, 0, 0xFA, 0},
	{NULL, "PLY",  class1, 0, 0x7A, 0},
	{NULL, "RMB",  class9, 0, 0x07, 0},
	{NULL, "RMB0", class4, ZP, 0x03, 0},
	{NULL, "RMB1", class4, ZP, 0x13, 0},
	{NULL, "RMB2", class4, ZP, 0x23, 0},
	{NULL, "RMB3", class4, ZP, 0x33, 0},
	{NULL, "RMB4", class4, ZP, 0x43, 0},
	{NULL, "RMB5", class4, ZP, 0x53, 0},
	{NULL, "RMB6", class4, ZP, 0x63, 0},
	{NULL, "RMB7", class4, ZP, 0x73, 0},
	{NULL, "SAX",  class1, 0, 0x22, 0},
	{NULL, "SAY",  class1, 0, 0x42, 0},
	{NULL, "SET",  class1, 0, 0xF4, 0},
	{NULL, "SMB",  class9, 0, 0x87, 0},
	{NULL, "SMB0", class4, ZP, 0x83, 0},
	{NULL, "SMB1", class4, ZP, 0x93, 0},
	{NULL, "SMB2", class4, ZP, 0xA3, 0},
	{NULL, "SMB3", class4, ZP, 0xB3, 0},
	{NULL, "SMB4", class4, ZP, 0xC3, 0},
	{NULL, "SMB5", class4, ZP, 0xD3, 0},
	{NULL, "SMB6", class4, ZP, 0xE3, 0},
	{NULL, "SMB7", class4, ZP, 0xF3, 0},
	{NULL, "ST0",  class4, IMM, 0x03, 1},
	{NULL, "ST1",  class4, IMM, 0x13, 1},
	{NULL, "ST2",  class4, IMM, 0x23, 1},
	{NULL, "STZ",  class4, ZP|ZP_X|ABS|ABS_X, 0x00, 0x05},
	{NULL, "SXY",  class1, 0, 0x02, 0},
	{NULL, "TAI",  class6, 0, 0xF3, 0},
	{NULL, "TAM",  class8, 0, 0x53, 0},
	{NULL, "TAM0", class3, 0, 0x53, 0x01},
	{NULL, "TAM1", class3, 0, 0x53, 0x02},
	{NULL, "TAM2", class3, 0, 0x53, 0x04},
	{NULL, "TAM3", class3, 0, 0x53, 0x08},
	{NULL, "TAM4", class3, 0, 0x53, 0x10},
	{NULL, "TAM5", class3, 0, 0x53, 0x20},
	{NULL, "TAM6", class3, 0, 0x53, 0x40},
	{NULL, "TAM7", class3, 0, 0x53, 0x80},
	{NULL, "TDD",  class6, 0, 0xC3, 0},
	{NULL, "TIA",  class6, 0, 0xE3, 0},
	{NULL, "TII",  class6, 0, 0x73, 0},
	{NULL, "TIN",  class6, 0, 0xD3, 0},
	{NULL, "TMA",  class8, 0, 0x43, 0},
	{NULL, "TMA0", class3, 0, 0x43, 0x01},
	{NULL, "TMA1", class3, 0, 0x43, 0x02},
	{NULL, "TMA2", class3, 0, 0x43, 0x04},
	{NULL, "TMA3", class3, 0, 0x43, 0x08},
	{NULL, "TMA4", class3, 0, 0x43, 0x10},
	{NULL, "TMA5", class3, 0, 0x43, 0x20},
	{NULL, "TMA6", class3, 0, 0x43, 0x40},
	{NULL, "TMA7", class3, 0, 0x43, 0x80},
	{NULL, "TRB",  class4, ZP|ABS, 0x10, 0},
	{NULL, "TSB",  class4, ZP|ABS, 0x00, 0},
	{NULL, "TST",  class7, 0, 0x00, 0},
	{NULL, NULL, NULL, 0, 0, 0}
};

/* PCE specific pseudos */
static struct t_opcode pce_pseudo[] = {
	{NULL,  "DEFCHR", pce_defchr, PSEUDO, P_DEFCHR, 0},
	{NULL,  "DEFPAL", pce_defpal, PSEUDO, P_DEFPAL, 0},
	{NULL,  "DEFSPR", pce_defspr, PSEUDO, P_DEFSPR, 0},
	{NULL,  "INCBAT", pce_incbat, PSEUDO, P_INCBAT, 0xD5},
	{NULL,  "INCSPR", pce_incspr, PSEUDO, P_INCSPR, 0xEA},
	{NULL,  "INCPAL", pce_incpal, PSEUDO, P_INCPAL, 0xF8},
	{NULL,  "INCTILE",pce_inctile,PSEUDO, P_INCTILE,0xEA},
	{NULL,  "INCMAP", pce_incmap, PSEUDO, P_INCMAP, 0xD5},
	{NULL,  "MML",    pce_mml,    PSEUDO, P_MML,    0},
	{NULL,  "PAL",    pce_pal,    PSEUDO, P_PAL,    0},
	{NULL,  "VRAM",   pce_vram,   PSEUDO, P_VRAM,   0},

	{NULL, ".DEFCHR", pce_defchr, PSEUDO, P_DEFCHR, 0},
	{NULL, ".DEFPAL", pce_defpal, PSEUDO, P_DEFPAL, 0},
	{NULL, ".DEFSPR", pce_defspr, PSEUDO, P_DEFSPR, 0},
	{NULL, ".INCBAT", pce_incbat, PSEUDO, P_INCBAT, 0xD5},
	{NULL, ".INCSPR", pce_incspr, PSEUDO, P_INCSPR, 0xEA},
	{NULL, ".INCPAL", pce_incpal, PSEUDO, P_INCPAL, 0xF8},
	{NULL, ".INCTILE",pce_inctile,PSEUDO, P_INCTILE,0xEA},
	{NULL, ".INCMAP", pce_incmap, PSEUDO, P_INCMAP, 0xD5},
	{NULL, ".MML",    pce_mml,    PSEUDO, P_MML,    0},
	{NULL, ".PAL",    pce_pal,    PSEUDO, P_PAL,    0},
	{NULL, ".VRAM",   pce_vram,   PSEUDO, P_VRAM,   0},
	{NULL, NULL, NULL, 0, 0, 0}
};

/* PCE machine description */
struct t_machine pce = {
	MACHINE_PCE,   /* type */
	"PCEAS", /* asm_name */
	"PC-Engine Assembler (" VERSION_STRING ")", /* asm_title */
	".pce",  /* rom_ext */
	"PCE_INCLUDE", /* include_env */
	0xD8,    /* zp_limit */
	0x2000,  /* ram_limit */
	0x2000,  /* ram_base */
	1,       /* ram_page */
	0xF8,    /* ram_bank */
	pce_inst,   /* inst */
	pce_pseudo, /* pseudo_inst */
	pce_pack_8x8_tile,     /* pack_8x8_tile */
	pce_pack_16x16_tile,   /* pack_16x16_tile */
	pce_pack_16x16_sprite, /* pack_16x16_sprite */
	pce_write_header /* write_header */
};

/* locals */
static unsigned char buffer[16384];	/* buffer for .inc and .def directives */
static unsigned char header[512];	/* rom header */


void pce_init()
{
	MEMCLR(buffer);
	MEMCLR(header);
}

/* ----
 * write_header()
 * ----
 * generate and write rom header
 */

void
pce_write_header(FILE *f, int banks)
{
	/* setup header */
	memset(header, 0, 512);
	header[0] = banks;

	/* write */
	fwrite(header, 512, 1, f);
}


/* ----
 * pack_8x8_tile()
 * ----
 * encode a 8x8 tile
 */

int
pce_pack_8x8_tile(unsigned char *buffer, void *data, int line_offset, int format)
{
	int i, j;
	int cnt;
	unsigned int   pixel, mask;
	unsigned char *ptr;
	unsigned int  *packed;

	/* pack the tile only in the last pass */
	if (pass != LAST_PASS)
		return (32);

	/* clear buffer */
	memset(buffer, 0, 32);

	/* encode the tile */
	switch (format) {
	case CHUNKY_TILE:
		/* 8-bit chunky format - from a pcx */
		cnt = 0;
		ptr = (unsigned char *)data;

		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				pixel = ptr[j ^ 0x07];
				mask  = 1 << j;
				buffer[cnt]    |= (pixel & 0x01) ? mask : 0;
				buffer[cnt+1]  |= (pixel & 0x02) ? mask : 0;
				buffer[cnt+16] |= (pixel & 0x04) ? mask : 0;
				buffer[cnt+17] |= (pixel & 0x08) ? mask : 0;
			}				
			ptr += line_offset;
			cnt += 2;
		}
		break;

	case PACKED_TILE:
		/* 4-bit packed format - from an array */
		cnt = 0;
		packed = (unsigned int *)data;
	
		for (i = 0; i < 8; i++) {
			pixel = packed[i];
	
			for (j = 0; j < 8; j++) {
				mask = 1 << j;
				buffer[cnt]    |= (pixel & 0x01) ? mask : 0;
				buffer[cnt+1]  |= (pixel & 0x02) ? mask : 0;
				buffer[cnt+16] |= (pixel & 0x04) ? mask : 0;
				buffer[cnt+17] |= (pixel & 0x08) ? mask : 0;
				pixel >>= 4;
			}
			cnt += 2;
		}
		break;

	default:
		/* other formats not supported */
		error("Internal error: unsupported format passed to 'pack_8x8_tile'!");
		break;
	}

	/* ok */
	return (32);
}


/* ----
 * pack_16x16_tile()
 * ----
 * encode a 16x16 tile
 */

int
pce_pack_16x16_tile(unsigned char *buffer, void *data, int line_offset, int format)
{
	int i, j;
	int cnt;
	unsigned int   pixel, mask;
	unsigned char *ptr;

	/* pack the tile only in the last pass */
	if (pass != LAST_PASS)
		return (128);

	/* clear buffer */
	memset(buffer, 0, 128);

	/* encode the tile */
	switch (format) {
	case CHUNKY_TILE:
		/* 8-bit chunky format - from a pcx */
		cnt = 0;
		ptr = (unsigned char *)data;

		for (i = 0; i < 16; i++) {
			for (j = 0; j < 8; j++) {
				pixel = ptr[j ^ 0x07];
				mask  = 1 << j;
				buffer[cnt]    |= (pixel & 0x01) ? mask : 0;
				buffer[cnt+1]  |= (pixel & 0x02) ? mask : 0;
				buffer[cnt+16] |= (pixel & 0x04) ? mask : 0;
				buffer[cnt+17] |= (pixel & 0x08) ? mask : 0;

				pixel = ptr[(j + 8) ^ 0x07];
				buffer[cnt+32] |= (pixel & 0x01) ? mask : 0;
				buffer[cnt+33] |= (pixel & 0x02) ? mask : 0;
				buffer[cnt+48] |= (pixel & 0x04) ? mask : 0;
				buffer[cnt+49] |= (pixel & 0x08) ? mask : 0;
			}				
			if (i == 7)
				cnt += 48;
			ptr += line_offset;
			cnt += 2;
		}
		break;

	default:
		/* other formats not supported */
		error("Internal error: unsupported format passed to 'pack_16x16_tile'!");
		break;
	}

	/* ok */
	return (128);
}


/* ----
 * pack_16x16_sprite()
 * ----
 * encode a 16x16 sprite
 */

int
pce_pack_16x16_sprite(unsigned char *buffer, void *data, int line_offset, int format)
{
	int i, j;
	int cnt;
	unsigned int   pixel, mask;
	unsigned char *ptr;
	unsigned int  *packed;

	/* pack the sprite only in the last pass */
	if (pass != LAST_PASS)
		return (128);

	/* clear buffer */
	memset(buffer, 0, 128);

	/* encode the sprite */
	switch (format) {
	case CHUNKY_TILE:
		/* 8-bit chunky format - from pcx */
		cnt = 0;
		ptr = (unsigned char *)data;

		for (i = 0; i < 16; i++) {
			/* right column */
			for (j = 0; j < 8; j++) {
				pixel = ptr[j ^ 0x0F];
				mask  = 1 << j;
				buffer[cnt]    |= (pixel & 0x01) ? mask : 0;
				buffer[cnt+32] |= (pixel & 0x02) ? mask : 0;
				buffer[cnt+64] |= (pixel & 0x04) ? mask : 0;
				buffer[cnt+96] |= (pixel & 0x08) ? mask : 0;
			}

			/* left column */
			for (j = 0; j < 8; j++) {
				pixel = ptr[j ^ 0x07];
				mask  = 1 << j;
				buffer[cnt+1]  |= (pixel & 0x01) ? mask : 0;
				buffer[cnt+33] |= (pixel & 0x02) ? mask : 0;
				buffer[cnt+65] |= (pixel & 0x04) ? mask : 0;
				buffer[cnt+97] |= (pixel & 0x08) ? mask : 0;
			}				
			ptr += line_offset;
			cnt += 2;
		}
		break;

	case PACKED_TILE:
		/* 4-bit packed format - from array */
		cnt = 0;
		packed = (unsigned int *)data;
	
		for (i = 0; i < 16; i++) {
			/* left column */
			pixel = packed[cnt];
	
			for (j = 0; j < 8; j++) {
				mask = 1 << j;
				buffer[cnt+1]  |= (pixel & 0x01) ? mask : 0;
				buffer[cnt+33] |= (pixel & 0x02) ? mask : 0;
				buffer[cnt+65] |= (pixel & 0x04) ? mask : 0;
				buffer[cnt+97] |= (pixel & 0x08) ? mask : 0;
				pixel >>= 4;
			}

			/* right column */
			pixel = packed[cnt + 1];
	
			for (j = 0; j < 8; j++) {
				mask = 1 << j;
				buffer[cnt]    |= (pixel & 0x01) ? mask : 0;
				buffer[cnt+32] |= (pixel & 0x02) ? mask : 0;
				buffer[cnt+64] |= (pixel & 0x04) ? mask : 0;
				buffer[cnt+96] |= (pixel & 0x08) ? mask : 0;
				pixel >>= 4;
			}
			cnt += 2;
		}
		break;

	default:
		/* other formats not supported */
		error("Internal error: unsupported format passed to 'pack_16x16_sprite'!");
		break;
	}

	/* ok */
	return (128);
}


/* ----
 * do_vram()
 * ----
 * .vram pseudo
 */

void
pce_vram(int *ip)
{
	/* define label */
	labldef(loccnt, 1);

	/* check if there's a label */
	if (lastlabl == NULL) {
		error("No label!");
		return;
	}

	/* get the VRAM address */
	if (!evaluate(ip, ';'))
		return;
	if (value >= 0x7F00) {
		error("Incorrect VRAM address!");
		return;
	}
	lastlabl->vram = value;

	/* output line */
	if (pass == LAST_PASS) {
		loadlc(value, 1);
		println();
	}
}


/* ----
 * do_pal()
 * ----
 * .pal pseudo
 */

void
pce_pal(int *ip)
{
	/* define label */
	labldef(loccnt, 1);

	/* check if there's a label */
	if (lastlabl == NULL) {
		error("No label!");
		return;
	}

	/* get the palette index */
	if (!evaluate(ip, ';'))
		return;
	if (value > 15) {
		error("Incorrect palette index!");
		return;
	}
	lastlabl->pal = value;

	/* output line */
	if (pass == LAST_PASS) {
		loadlc(value, 1);
		println();
	}
}


/* ----
 * do_defchr()
 * ----
 * .defchr pseudo
 */

void
pce_defchr(int *ip)
{
	unsigned int data[8];
	int size;
	int i;

	/* output infos */
	data_loccnt = loccnt;
	data_size   = 3;
	data_level  = 3;

	/* check if there's a label */
	if (lablptr) {
		/* define label */
		labldef(loccnt, 1);

		/* get the VRAM address */
		if (!evaluate(ip, ','))
			return;
		if (value >= 0x7F00) {
			error("Incorrect VRAM address!");
			return;
		}
		lablptr->vram = value;
	
		/* get the default palette */
		if (!evaluate(ip, ','))
			return;
		if (value > 0x0F) {
			error("Incorrect palette index!");
			return;
		}
		lablptr->pal = value;
	}

	/* get tile data */
	for (i = 0; i < 8; i++) {
		/* get value */
		if (!evaluate(ip, (i < 7) ? ',' : ';'))
			return;

		/* store value */
		data[i] = value;
	}

	/* encode tile */
	size = pce_pack_8x8_tile(buffer, data, 0, PACKED_TILE);

	/* store tile */
	putbuffer(buffer, size);

	/* output line */
	if (pass == LAST_PASS)
		println();
}


/* ----
 * do_defpal()
 * ----
 * .defpal pseudo
 */

void
pce_defpal(int *ip)
{
	unsigned int color;
	unsigned int r, g, b;
	char c;

	/* define label */
	labldef(loccnt, 1);

	/* output infos */
	data_loccnt = loccnt;
	data_size   = 2;
	data_level  = 3;

	/* get data */
	for (;;) {
		/* get color */
		if (!evaluate(ip, 0))
			return;

		/* store data on last pass */
		if (pass == LAST_PASS) {
			/* convert color */
			r = (value >> 8) & 0x7;
			g = (value >> 4) & 0x7;
			b = (value) & 0x7;
			color = (g << 6) + (r << 3) + (b);

			/* store color */
			putword(loccnt, color);
		}

		/* update location counter */
		loccnt += 2;

		/* check if there's another color */
		c = prlnbuf[(*ip)++];

		if (c != ',')
			break;
	}

	/* output line */
	if (pass == LAST_PASS)
		println();

	/* check errors */
	if (c != ';' && c != '\0')
		error("Syntax error!");
}


/* ----
 * do_defspr()
 * ----
 * .defspr pseudo
 */

void
pce_defspr(int *ip)
{
	unsigned int data[32];
	int size;
	int i;

	/* output infos */
	data_loccnt = loccnt;
	data_size   = 3;
	data_level  = 3;

	/* check if there's a label */
	if (lablptr) {
		/* define label */
		labldef(loccnt, 1);
	
		/* get the VRAM address */
		if (!evaluate(ip, ','))
			return;
		if (value >= 0x7F00) {
			error("Incorrect VRAM address!");
			return;
		}
		lablptr->vram = value;
	
		/* get the default palette */
		if (!evaluate(ip, ','))
			return;
		if (value > 0x0F) {
			error("Incorrect palette index!");
			return;
		}
		lablptr->pal = value;
	}

	/* get sprite data */
	for (i = 0; i < 32; i++) {
		/* get value */
		if (!evaluate(ip, (i < 31) ? ',' : ';'))
			return;

		/* store value */
		data[i] = value;
	}

	/* encode sprite */
	size = pce_pack_16x16_sprite(buffer, data, 0, PACKED_TILE);

	/* store sprite */
	putbuffer(buffer, size);

	/* output line */
	if (pass == LAST_PASS)
		println();
}


/* ----
 * do_incbat()
 * ----
 * build a BAT
 */

void
pce_incbat(int *ip)
{
	unsigned char *ptr, ref;
	unsigned int i, j, k, l, x, y, w, h;
	unsigned int base, index, flag;
	unsigned int temp;

	/* define label */
	labldef(loccnt, 1);

	/* output */
	if (pass == LAST_PASS)
		loadlc(loccnt, 0);

	/* get args */
	if (!pcx_get_args(ip))
		return;
	if (!pcx_parse_args(1, pcx_nb_args - 1, (int *)&x, (int *)&y, (int *)&w, (int *)&h, 8))
		return;

	/* build the BAT */
	if (pass == LAST_PASS) {
		index = 0;
		flag  = 0;
		base  = (pcx_arg[0] >> 4);

		for (i = 0; i < h; i++) {
			for (j = 0; j < w; j++) {
				ptr = pcx_buf + (x + (j * 8)) + ((y + (i * 8)) * pcx_w);
				ref = ptr[0] & 0xF0;

				/* check colors */
				for (k = 0; k < 8; k++) {
					for (l = 0; l < 8; l++) {
						if ((ptr[l] & 0xF0) != ref)
							flag = 1;
					}				
					ptr += pcx_w;
				}
				temp = (base & 0xFFF) | ((ref & 0xF0) << 8);
				buffer[2*index]   = temp & 0xff;
				buffer[2*index+1] = temp >> 8;

				index++;
				base++;
			}
		}

		/* errors */
		if (flag)
			error("Invalid color index found!");
	}

	/* store data */
	putbuffer(buffer, 2 * w * h);

	/* output */
	if (pass == LAST_PASS)
		println();
}


/* ----
 * do_incpal()
 * ----
 * extract the palette of a PCX file
 */

void
pce_incpal(int *ip)
{
	unsigned int temp;
	int i, start, nb;
	int r, g, b;

	/* define label */
	labldef(loccnt, 1);

	/* output */
	if (pass == LAST_PASS)
		loadlc(loccnt, 0);

	/* get args */
	if (!pcx_get_args(ip))
		return;

	start = 0;
	nb = pcx_nb_colors;

	if (pcx_nb_args) {
		start = pcx_arg[0] << 4;

		if (pcx_nb_args == 1)
			nb = 16;
		else {
			nb = pcx_arg[1] << 4;
		}
	}

	/* check args */
	if (((start + nb) > 256) || (nb == 0)) {
		error("Palette index out of range!");
		return;
	}

	/* make data */
	if (pass == LAST_PASS) {
		for (i = 0; i < nb; i++) {
			r = pcx_pal[start + i][0];
			g = pcx_pal[start + i][1];
			b = pcx_pal[start + i][2];
			temp = ((r & 0xE0) >> 2) | ((g & 0xE0) << 1) | ((b & 0xE0) >> 5);
			buffer[2*i]   = temp & 0xff;
			buffer[2*i+1] = temp >> 8;
		}
	}

	/* store data */
	putbuffer(buffer, nb << 1);

	/* output */
	if (pass == LAST_PASS)
		println();
}


/* ----
 * do_incspr()
 * ----
 * PCX to sprites
 */

void
pce_incspr(int *ip)
{
	unsigned int i, j;
	unsigned int x, y, w, h;
	unsigned int sx, sy;
	unsigned int size;

	/* define label */
	labldef(loccnt, 1);

	/* output */
	if (pass == LAST_PASS)
		loadlc(loccnt, 0);

	/* get args */
	if (!pcx_get_args(ip))
		return;
	if (!pcx_parse_args(0, pcx_nb_args, (int *)&x, (int *)&y, (int *)&w, (int *)&h, 16))
		return;

	/* pack sprites */
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			/* sprite coordinates */
			sx = x + (j << 4);
			sy = y + (i << 4);

			/* encode sprite */
			size = pcx_pack_16x16_sprite(buffer, sx, sy);

			/* store sprite */
			putbuffer(buffer, size);
		}
	}

	/* output */
	if (pass == LAST_PASS)
		println();
}


/* ----
 * do_inctile()
 * ----
 * PCX to 16x16 tiles
 */

void
pce_inctile(int *ip)
{
	unsigned int i, j;
	unsigned int x, y, w, h;
	unsigned int tx, ty;
	int nb_tile = 0;
	int size = 0;

	/* define label */
	labldef(loccnt, 1);

	/* output */
	if (pass == LAST_PASS)
		loadlc(loccnt, 0);

	/* get args */
	if (!pcx_get_args(ip))
		return;
	if (!pcx_parse_args(0, pcx_nb_args, (int *)&x, (int *)&y, (int *)&w, (int *)&h, 16))
		return;

	/* pack tiles */
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			/* tile coordinates */
			tx = x + (j << 4);
			ty = y + (i << 4);

			/* get tile */
			size = pcx_pack_16x16_tile(buffer, tx, ty);

			/* store tile */
			putbuffer(buffer, size);
			nb_tile++;
		}
	}

	/* attach the number of loaded tiles to the label */
	if (lastlabl) {
		if (nb_tile) {
			lastlabl->nb = nb_tile;
			if (pass == LAST_PASS)
				lastlabl->size = size;
		}
	}

	/* output */
	if (pass == LAST_PASS)
		println();
}


/* ----
 * do_incmap()
 * ----
 * .incmap pseudo - convert a tiled PCX into a map
 */

void
pce_incmap(int *ip)
{
	unsigned int i, j;
	unsigned int x, y, w, h;
	unsigned int tx, ty;
	int tile, size;
	int err = 0;

	/* define label */
	labldef(loccnt, 1);

	/* output */
	if (pass == LAST_PASS)
		loadlc(loccnt, 0);

	/* get args */
	if (!pcx_get_args(ip))
		return;
	if (!pcx_parse_args(0, pcx_nb_args - 1, (int *)&x, (int *)&y, (int *)&w, (int *)&h, 16))
		return;

	/* pack map */
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			/* tile coordinates */
			tx = x + (j << 4);
			ty = y + (i << 4);

			/* get tile */
			size = pcx_pack_16x16_tile(buffer, tx, ty);

			/* search tile */
			tile = pcx_search_tile(buffer, size);

			if (tile == -1) {
				/* didn't find the tile */
				tile = 0;
				err++;
			}

			/* store tile index */
			if (pass == LAST_PASS)
				putbyte(loccnt, tile & 0xFF);

			/* update location counter */
			loccnt++;
		}
	}

	/* error */
	if (err)
		error("One or more tiles didn't match!");

	/* output */
	if (pass == LAST_PASS)
		println();
}


/* ----
 * do_mml()
 * ----
 * .mml pseudo - music/sound macro language
 */

void
pce_mml(int *ip)
{
	int offset, bufsize, size;
	char mml[128];
	char c;

	/* define label */
	labldef(loccnt, 1);

	/* output */
	if (pass == LAST_PASS)
		loadlc(loccnt, 0);

	/* start */
	bufsize = 8192;
	offset = mml_start(buffer);
	bufsize -= offset;

	/* extract and parse mml string(s) */
	for (;;) {
		/* get string */
		if (!getstring(ip, mml, 127))
			return;

		/* parse string */
		size = mml_parse(buffer + offset, bufsize, mml);

		if (size == -1)
			return;

		/* adjust buffer size */
		offset  += size;
		bufsize -= size;

		/* next string */
		c = prlnbuf[(*ip)++];

		if ((c != ',') && (c != ';') && (c != '\0')) {
			error("Syntax error!");
			return;
		}
		if (c != ',')
			break;

		/* skip spaces */
		while (isspace(prlnbuf[*ip]))
			(*ip)++;

		/* continuation char */
		if (prlnbuf[*ip] == '\\') {
			(*ip)++;

			/* skip spaces */
			while (isspace(prlnbuf[*ip]))
				(*ip)++;

			/* check */
			if (prlnbuf[*ip] == ';' || prlnbuf[*ip] == '\0') {
				/* output line */
				if (pass == LAST_PASS) {
					println();
					clearln();
				}

				/* read a new line */
				if (readline() == -1)
					return;
				
				/* rewind line pointer and continue */
				*ip = SFIELD;
			}
			else {
				error("Syntax error!");
				return;
			}
		}
	}			

	/* stop */
	offset += mml_stop(buffer + offset);

	/* store data */
	putbuffer(buffer, offset);

	/* output */
	if (pass == LAST_PASS)
		println();
}

