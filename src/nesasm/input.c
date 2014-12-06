#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"
#include "externs.h"
#include "protos.h"

int infile_error;
int infile_num;
struct t_input_info input_file[MAX_INCS+1];
static char incpath[MAX_INCDIRS][MAX_PATH_LEN+1];


void input_init()
{
	infile_error = infile_num = 0;
	MEMCLR(input_file);
	MEMCLR(incpath);
}

/* ----
 * init_path()
 * ----
 * init the include path
 */

void
init_path(void)
{
	char *p,*pl;
	int	i, l;

	p = getenv(machine->include_env);

	if (p == NULL)
		return;

	for (i = 0; i < MAX_INCDIRS; i++) {

		while (*p == INC_SEPARATOR) p++;

		pl = strchr(p, INC_SEPARATOR);

		if (pl == NULL)
			l = strlen(p);
		else
			l = pl-p;

		if (l == 0)
		    break;

		if (l >= MAX_PATH_LEN-1) { /* -1 for PATH_SEPARATOR */
			printf("Too long incpath: %.*s (ignored)\n", l, p);
		} else if (l > 0) {
			memcpy(incpath[i], p, l);
			if (incpath[i][l-1] != PATH_SEPARATOR) {
			    incpath[i][l] = PATH_SEPARATOR;
			    incpath[i][l+1] = '\0';
			} else
			    incpath[i][l] = '\0';
		}
		p += l;
	}
}


/* ----
 * readline()
 * ----
 * read and format an input line.
 */

int
readline(void)
{
	char *ptr, *arg, num[8];
	int j, n;
	int	i;		/* pointer into prlnbuf */
	int	c;		/* current character		*/
	int	temp;	/* temp used for line number conversion */

start:
	for (i = 0; i < LAST_CH_POS; i++)
		prlnbuf[i] = ' ';

	/* if 'expand_macro' is set get a line from macro buffer instead */
	if (expand_macro) {
		if (mlptr == NULL) {
			while (mlptr == NULL) {
				midx--;
				mlptr = mstack[midx];
				mcounter = mcntstack[midx];
				if (midx == 0) {
					mlptr = NULL;
					expand_macro = 0;
					break;
				}
			}
		}

		/* expand line */
		if (mlptr) {
			i = SFIELD;
			ptr = mlptr->data;
			for (;;) {
				c = *ptr++;
				if (c == '\0')
					break;
				if (c != '\\')
					prlnbuf[i++] = c;
				else {
					c = *ptr++;
					prlnbuf[i] = '\0';

					/* \@ */
					if (c == '@') {
						n = 5;
						sprintf(num, "%05i", mcounter);
						arg = num;
					}

					/* \# */
					else if (c == '#') {
						for (j = 9; j > 0; j--)
							if (strlen(marg[midx][j - 1]))
								break;
						n = 1;
						sprintf(num, "%i", j);
						arg = num;
					}

					/* \?1 - \?9 */
					else if (c == '?') {
						c = *ptr++;
						if (c >= '1' && c <= '9') {
							n = 1;
							sprintf(num, "%i", macro_getargtype(marg[midx][c - '1']));
							arg = num;
						}
						else {
							error("Invalid macro argument index!");
							return (-1);
						}
					}

					/* \1 - \9 */
					else if (c >= '1' && c <= '9') {
						j   = c - '1';
						n   = strlen(marg[midx][j]);
						arg = marg[midx][j];
					}

					/* unknown macro special command */
					else {
						error("Invalid macro argument index!");
						return (-1);
					}

					/* check for line overflow */
					if ((i + n) >= LAST_CH_POS - 1) {
						error("Invalid line length!");
						return (-1);
					}

					/* copy macro string */
					strncpy(&prlnbuf[i], arg, n);
					i += n;
				}
				if (i >= LAST_CH_POS - 1)
					i  = LAST_CH_POS - 1;
			}
			prlnbuf[i] = '\0';
			mlptr = mlptr->next;
			return (0);
		}
	}

	/* put source line number into prlnbuf */
	i = 4;
	temp = ++slnum;
	while (temp != 0) {
		prlnbuf[i--] = temp % 10 + '0';
		temp /= 10;
	}

	/* get a line */
	i = SFIELD;
	c = getc(in_fp);
	if (c == EOF) {
		if (close_input())
			return (-1);
		goto start;
	}
	for (;;) {
		/* check for the end of line */
		if (c == '\r') {
			c = getc(in_fp);
			if (c == '\n' || c == EOF)
				break;
			ungetc(c, in_fp);
			break;
		}
		if (c == '\n' || c == EOF)
			break;

		/* store char in the line buffer */
		prlnbuf[i] = c;
		i += (i < LAST_CH_POS) ? 1 : 0;

		/* expand tab char to space */
		if (c == '\t') {
			prlnbuf[--i] = ' ';
			i += (8 - ((i - SFIELD) % 8));
		}

		/* get next char */
		c = getc(in_fp);
	}
	prlnbuf[i] = '\0';
	return(0);
}

/* ----
 * open_input()
 * ----
 * open input files - up to MAX_INCS levels.
 */

int
open_input(char *name)
{
	FILE *fp;
	char *p;
	char  temp[MAX_FILENAME_LEN+1];
	int   i;

	/* only MAX_INCS nested input files */
	if (infile_num == MAX_INCS) {
		error("Too many include levels, max. "
		      MACROVAL_TO_STR(MAX_INCS) "!");
		return (1);
	}

	/* backup current input file infos */
	if (infile_num) {
		input_file[infile_num].lnum = slnum;
		input_file[infile_num].fp = in_fp;
	}

	/* get a copy of the file name */
	if (strlen(name) > MAX_FILENAME_LEN) {
	    error("Too long file name.");
	    return (1);
	}
	strcpy(temp, name);

	/* auto add the .asm file extension */
	if ((p = strrchr(temp, '.')) == NULL || strchr(p, PATH_SEPARATOR)) {
	    if (strlen(temp)+4 > MAX_FILENAME_LEN) {
		error("Too long file name.");
		return (1);
	    }
	    strcat(temp, ".asm");
	}

	/* check if this file is already opened */
	if (infile_num) {
		for (i = 1; i < infile_num; i++) {
			if (!strcmp(input_file[i].name, temp)) {
				error("Repeated include file!");
				return (1);
			}
		}
	}

	/* open the file */
	if ((fp = open_file(temp, "r")) == NULL)
		return (-1);

	/* update input file infos */
	in_fp = fp;
	slnum = 0;
	infile_num++;
	input_file[infile_num].fp = fp;
	input_file[infile_num].if_level = if_level;
	strcpy(input_file[infile_num].name, temp);
	if ((pass == LAST_PASS) && (xlist) && (list_level))
		fprintf(lst_fp, "#[%i]   %s\n", infile_num, input_file[infile_num].name);

	/* ok */
	return (0);
}


/* ----
 * close_input()
 * ----
 * close an input file, return -1 if no more files in the stack.
 */

int
close_input(void)
{
	if (proc_ptr) {
		fatal_error("Incomplete PROC!");
		return (-1);
	}
	if (in_macro) {
		fatal_error("Incomplete MACRO definition!");
		return (-1);
	}
	if (input_file[infile_num].if_level != if_level) {
		fatal_error("Incomplete IF/ENDIF statement!");
		return (-1);
	}
	if (infile_num <= 1)
		return (-1);

	fclose(in_fp);
	infile_num--;
	infile_error = -1;
	slnum = input_file[infile_num].lnum;
	in_fp = input_file[infile_num].fp;
	if ((pass == LAST_PASS) && (xlist) && (list_level))
		fprintf(lst_fp, "#[%i]   %s\n", infile_num, input_file[infile_num].name);

	/* ok */
	return (0);
}


/* ----
 * open_file()
 * ----
 * open a file - browse paths
 */

FILE *
open_file(char *name, char *mode)
{
	FILE	*fileptr;
	char	testname[MAX_PATH_LEN+1];
	int	i;
	size_t	dirlen, namelen;

	fileptr = fopen(name, mode);
	if (fileptr != NULL) return(fileptr);

	namelen = strlen(name);
	for (i = 0; i < 10; i++) {
		if ((dirlen = strlen(incpath[i]))) {
			if (dirlen+namelen > MAX_PATH_LEN) {
			    error("Too long path name.");
			    return (NULL);
			}
			strcpy(testname, incpath[i]);
			strcat(testname, name);

			fileptr = fopen(testname, mode);
			if (fileptr != NULL) break;
		}
	}

	return (fileptr);
}

