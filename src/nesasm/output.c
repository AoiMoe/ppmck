#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "externs.h"
#include "protos.h"


/*
 * ---
 * hexcon()
 * ---
 * convert integer to n digit hexadecimal, without null termination.
 */
static void
hexcon(char *buf, int n, int val)
{
	/* assumes CHAR_BIT == 8 */
	char tmp[sizeof (unsigned)*2+1];
	int col;

	col = sprintf(tmp, "%0*X", n, (unsigned)val);
	memcpy(buf, tmp+col-n, n);
}

/* ----
 * println()
 * ----
 * prints the contents of prlnbuf
 */

void
println(void)
{
	int nb, cnt;
	int i;

	/* check if output possible */
	if (list_level == 0)
		return;
	if (!xlist || !asm_opt[OPT_LIST] || (expand_macro && !asm_opt[OPT_MACRO]))
		return;

	/* update line buffer if necessary */
	if (continued_line)
		strcpy(prlnbuf, tmplnbuf);

	/* output */
	if (data_loccnt == -1)
		/* line buffer */
		fprintf(lst_fp, "%s\n", prlnbuf);
	else {
		/* line buffer + data bytes */
		loadlc(data_loccnt, 0);

		/* number of bytes */
		nb = loccnt - data_loccnt;

		/* check level */
		if ((data_level > list_level) && (nb > 3))
			/* doesn't match */
			fprintf(lst_fp, "%s\n", prlnbuf);
		else {
			/* ok */
			cnt = 0;
			for (i = 0; i < nb; i++) {
				if (bank >= RESERVED_BANK) {
					prlnbuf[16 + (3*cnt)] = '-';
					prlnbuf[17 + (3*cnt)] = '-';
				}
				else {
					hexcon(&prlnbuf[16 + (3*cnt)],
					       2, rom[bank][data_loccnt]);
				}
				data_loccnt++;
				cnt++;
				if (cnt == data_size) {
					cnt = 0;
					fprintf(lst_fp, "%s\n", prlnbuf);
					clearln();
					loadlc(data_loccnt, 0);
				}
			}
			if (cnt)
				fprintf(lst_fp, "%s\n", prlnbuf);
		}
	}
}


/* ----
 * clearln()
 * ----
 * clear prlnbuf
 */

void
clearln(void)
{
	int i;

	for (i = 0; i < SFIELD; i++)
		prlnbuf[i] = ' ';
	prlnbuf[i] = 0;
}


/* ----
 * loadlc()
 * ----
 * load 16 bit value in printable form into prlnbuf
 */

void
loadlc(int offset, int pos)
{
	int	i;

	if (pos)
		i = VFIELD;
	else
		i = AFIELD;

	if (pos == 0) {
		if (bank >= RESERVED_BANK) {
			prlnbuf[i++] = '-';
			prlnbuf[i++] = '-';
		}
		else {
			hexcon(&prlnbuf[i], 2, bank);
			i+=2;
		}
		prlnbuf[i++] = ':';
		offset += B2ADDR(page, 0);
	}
	hexcon(&prlnbuf[i], 4, offset);
}


/* ----
 * putbyte()
 * ----
 * store a byte in the rom
 */

void
putbyte(int offset, int data)
{
	if (bank >= RESERVED_BANK)
		return;
	if (offset < BANK_SIZE) {
		rom[bank][offset] = (data) & 0xFF;
		map[bank][offset] = section + (page << 5);

		/* update rom size */
		if (bank > max_bank)
			max_bank = bank;
	}
}


/* ----
 * putword()
 * ----
 * store a word in the rom
 */

void
putword(int offset, int data)
{
	if (bank >= RESERVED_BANK)
		return;
	if (offset < BANK_SIZE-1) {
		/* low byte */
		rom[bank][offset] = (data) & 0xFF;
		map[bank][offset] = section + (page << 5);

		/* high byte */
		rom[bank][offset+1] = (data >> 8) & 0xFF;
		map[bank][offset+1] = section + (page << 5);

		/* update rom size */
		if (bank > max_bank)
			max_bank = bank;
	}
}


/* ----
 * putbuffer()
 * ----
 * copy a buffer at the current location
 */

void
putbuffer(void *data, int size)
{
	int addr;

	/* check size */
	if (size == 0)
		return;

	/* check if the buffer will fit in the rom */
	if (bank >= RESERVED_BANK) {
		addr  = loccnt + size;

		if (addr >= BANK_SIZE) {
			fatal_error("PROC overflow!");
			return;
		}
	}
	else {
		/* loccnt+size may be larger than bank size */
		addr  = B2ADDR(bank, loccnt) + size;

		if (addr > rom_limit) {
			fatal_error("ROM overflow!");
			return;
		}

		/* copy the buffer */
		if (pass == LAST_PASS) {
			if (data) {
				memcpy(&rom[bank][loccnt], data, size);
				memset(&map[bank][loccnt], section + (page << 5), size);
			}
			else {
				memset(&rom[bank][loccnt], 0, size);
				memset(&map[bank][loccnt], section + (page << 5), size);
			}
		}
	}

	/* update the location counter */
	bank  += ADDR2BNUM(loccnt + size);
	loccnt = ADDR2BOFS(loccnt + size);

	/* update rom size */
	if (bank < RESERVED_BANK) {
		if (bank > max_bank) {
			if (loccnt)
				max_bank = bank;
			else
				max_bank = bank - 1;
		}
	}
}


/* ----
 * write_srec()
 * ----
 */

void
write_srec(char *file, char *ext, int base)
{
	unsigned char data, chksum;
	char  fname[128];
	int   addr, dump, cnt, pos, i, j;
	FILE *fp;

	/* status message */
	if (!strcmp(ext, "mx"))
		printf("writing mx file... ");
	else
		printf("writing s-record file... ");

	/* flush output */
	fflush(stdout);

	/* add the file extension */
	strcpy(fname, file);
	strcat(fname, ".");
	strcat(fname, ext);

	/* open the file */
	if ((fp = fopen(fname, "w")) == NULL) {
		printf("can not open file '%s'!\n", fname);
		return;
	}

	/* dump the rom */
	dump = 0;
	cnt  = 0;
	pos  = 0;

	for (i = 0; i <= max_bank; i++) {
		for (j = 0; j < BANK_SIZE; j++) {
			if (map[i][j] != 0xFF) {
				/* data byte */
				if (cnt == 0)
					pos = j;
				cnt++;
				if (cnt == 32)
					dump = 1;
			}
			else {
				/* free byte */
				if (cnt)
					dump = 1;
			}
			if (j == BANK_SIZE-1)
				if (cnt)
					dump = 1;

			/* dump */
			if (dump) {
				dump = 0;
				addr = base + B2ADDR(i, pos);
				chksum = cnt + ((addr >> 16) & 0xFF) +
							   ((addr >> 8) & 0xFF) +
							   ((addr) & 0xFF) +
							   4;

				/* number, address */
				fprintf(fp, "S2%02X%06X", cnt + 4, addr);

				/* code */
				while (cnt) {
					data = rom[i][pos++];
					chksum += data;
					fprintf(fp, "%02X", data);
					cnt--;
				}

				/* chksum */
				fprintf(fp, "%02X\n", (~chksum) & 0xFF);
			}
		}
	}

	/* starting address */
	addr   = B2ADDR(map[0][0] >> 5, 0);
	chksum = ((addr >> 8) & 0xFF) + (addr & 0xFF) + 4;
	fprintf(fp, "S804%06X%02X", addr, (~chksum) & 0xFF);

	/* ok */
	fclose(fp);
	printf("OK\n");
}


/* ----
 * fatal_error()
 * ----
 * stop compilation
 */

void
fatal_error(char *stptr)
{
	error(stptr);
	stop_pass = 1;
}


/* ----
 * error()
 * ----
 * error printing routine
 */

void
error(char *stptr)
{
	warning(stptr);
	errcnt++;
}


/* ----
 * warning()
 * ----
 * warning printing routine
 */

void
warning(char *stptr)
{
	int i, temp;

	/* put the source line number into prlnbuf */
	i = LFIELD_LEN-1;
	temp = slnum;
	while (temp != 0) {
		prlnbuf[i--] = temp % 10 + '0';
		temp /= 10;
	}

	/* update the current file name */
	if (infile_error != infile_num) {
		infile_error  = infile_num;
		printf("#[%i]   %s\n", infile_num, input_file[infile_num].name);
	}

	/* output the line and the error message */
	loadlc(loccnt, 0);
	printf("%s\n", prlnbuf);
	printf("       %s\n", stptr);
}

