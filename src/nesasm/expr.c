#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "defs.h"
#include "externs.h"
#include "protos.h"

/* value types */
#define T_DECIMAL	0
#define T_HEXA		1
#define T_BINARY	2
#define T_CHAR		3
#define T_SYMBOL	4
#define T_PC		5

/* operators */
#define OP_START 0
#define OP_OPEN	 1
#define OP_ADD	 2
#define OP_SUB	 3
#define OP_MUL	 4
#define OP_DIV	 5
#define OP_MOD	 6
#define OP_NEG	 7
#define OP_SHL	 8
#define OP_SHR	 9
#define OP_OR	10
#define OP_XOR	11
#define OP_AND	12
#define OP_COM	13
#define OP_NOT	14
#define OP_EQUAL		15
#define OP_NOT_EQUAL	16
#define OP_LOWER		17
#define OP_LOWER_EQUAL	18
#define OP_HIGHER		19
#define OP_HIGHER_EQUAL	20
#define OP_DEFINED	21
#define OP_HIGH		22
#define OP_LOW		23
#define OP_PAGE		24
#define OP_BANK		25
#define OP_VRAM		26
#define OP_PAL		27
#define OP_SIZEOF	28

/* operator priority */
int op_pri[] = {
	 0 /* START */,  0 /* OPEN  */,
	 7 /* ADD   */,  7 /* SUB   */,  8 /* MUL   */,  8 /* DIV   */,
	 8 /* MOD   */, 10 /* NEG   */,  6 /* SHL   */,  6 /* SHR   */,
	 1 /* OR    */,  2 /* XOR   */,  3 /* AND   */, 10 /* COM   */,
	 9 /* NOT   */,  4 /* =     */,  4 /* <>    */,  5 /* <     */,
	 5 /* <=    */,  5 /* >     */,  5 /* >=    */,
	10 /* DEFIN.*/, 10 /* HIGH  */, 10 /* LOW   */, 10 /* PAGE  */,
	10 /* BANK  */, 10 /* VRAM  */, 10 /* PAL   */, 10 /* SIZEOF*/
};

unsigned int  op_stack[64] = { OP_START };	/* operator stack */
unsigned int val_stack[64];	/* value stack */
int op_idx, val_idx;	/* index in the operator and value stacks */
int need_operator;		/* when set await an operator, else await a value */
unsigned char *expr;	/* pointer to the expression string */
unsigned char *expr_stack[16];	/* expression stack */
struct t_symbol *expr_lablptr;	/* pointer to the lastest label */
int expr_lablcnt;		/* number of label seen in an expression */
char *keyword[8] = {	/* predefined functions */
	"\7DEFINED",
	"\4HIGH", "\3LOW",
	"\4PAGE", "\4BANK",
	"\4VRAM", "\3PAL",
	"\6SIZEOF"
};

/* ----
 * evaluate()
 * ----
 * evaluate an expression
 */

int
evaluate(int *ip, char last_char)
{
	int end, level;
	int op, type;
	int arg;
	int i;
	unsigned char c;

	end = 0;
	level = 0;
	undef = 0;
	op_idx = 0;
	val_idx = 0;
	value = 0;
	val_stack[0] = 0;
	need_operator = 0;
	expr_lablptr = NULL;
	expr_lablcnt = 0;
	op = OP_START;
	func_idx = 0;

	/* array index to pointer */
	expr = (unsigned char *)&prlnbuf[*ip];

	/* skip spaces */
cont:
	while (isspace(*expr))
		expr++;

	/* search for a continuation char */
	if (*expr == '\\') {
		/* skip spaces */
		i = 1;
		while (isspace(expr[i]))
			i++;

		/* check if end of line */
		if (expr[i] == ';' || expr[i] == '\0') {
			/* output */
			if (!continued_line) {
				/* replace '\' with three dots */
				strcpy((char *)expr, "...");

				/* store the current line */
				strcpy(tmplnbuf, prlnbuf);
			}

			/* ok */
			continued_line++;

			/* read a new line */
			if (readline() == -1)
				return (0);

			/* rewind line pointer and continue */
			expr = (unsigned char *)&prlnbuf[SFIELD];
			goto cont;
		}
	}

	/* parser main loop */
	while (!end) {
		c = *expr;

		/* number */
		if (isdigit(c)) {
			if (need_operator)
				goto error;
			if (!push_val(T_DECIMAL))
				return (0);
		} 

		/* symbol */
		else
		if (isalpha(c) || c == '_' || c == '.') {
			if (need_operator)
				goto error;
			if (!push_val(T_SYMBOL))
				return (0);
		} 

		/* operators */
		else {
			switch (c) {
			/* function arg */
			case '\\':
				if (func_idx == 0) {
					error("Syntax error in expression!");
					return (0);
				}
				expr++;
				c = *expr++;
				if (c < '1' || c > '9') {
					error("Invalid function argument index!");
					return (0);
				}
				arg = c - '1';
				expr_stack[func_idx++] = expr;
				expr = (unsigned char *)func_arg[func_idx - 2][arg];
				break;

			/* hexa prefix */
 			case '$':
				if (need_operator)
					goto error;
				if (!push_val(T_HEXA))
					return (0);
				break;

			/* character prefix */
			case '\'':
				if (need_operator)
					goto error;
				if (!push_val(T_CHAR))
					return (0);
				break;

			/* round brackets */
			case '(':
				if (need_operator)
					goto error;
				if (!push_op(OP_OPEN))
					return (0);
				level++;
				expr++;
				break;
			case ')':
				if (!need_operator)
					goto error;
				if (level == 0)
					goto error;
				while (op_stack[op_idx] != OP_OPEN) {
					if (!do_op())
						return (0);
				}
				op_idx--;
				level--;
				expr++;
				break;

			/* not equal, left shift, lower, lower or equal */
			case '<':
				if (!need_operator)
					goto error;
				expr++;
				switch (*expr) {
				case '>':
					op = OP_NOT_EQUAL;
					expr++;
					break;
				case '<':
					op = OP_SHL;
					expr++;
					break;
				case '=':
					op = OP_LOWER_EQUAL;
					expr++;
					break;
				default:
					op = OP_LOWER;
					break;
				}
				if (!push_op(op))
					return (0);
				break;

			/* right shift, higher, higher or equal */
			case '>':
				if (!need_operator)
					goto error;
				expr++;
				switch (*expr) {
				case '>':
					op = OP_SHR;
					expr++;
					break;
				case '=':
					op = OP_HIGHER_EQUAL;
					expr++;
					break;
				default:
					op = OP_HIGHER;
					break;
				}
				if (!push_op(op))
					return (0);
				break;

			/* equal */
			case '=':
				if (!need_operator)
					goto error;
				if (!push_op(OP_EQUAL))
					return (0);
				expr++;
				break;

			/* one complement */
			case '~':
				if (need_operator)
					goto error;
				if (!push_op(OP_COM))
					return (0);
				expr++;
				break;

			/* sub, neg */
			case '-':
				if (need_operator)
					op = OP_SUB;
				else
					op = OP_NEG;
				if (!push_op(op))
					return (0);
				expr++;
				break;

			/* not, not equal */
			case '!':
				if (!need_operator)
					op = OP_NOT;
				else {
					op = OP_NOT_EQUAL;
					expr++;
					if (*expr != '=')
						goto error;
				}
				if (!push_op(op))
					return (0);
				expr++;
				break;

			/* binary prefix, current PC */
			case '%':
			case '*':
				if (!need_operator) {
					if (c == '%')
						type = T_BINARY;
					else
						type = T_PC;
					if (!push_val(type))
						return (0);
					break;
				}

			/* modulo, mul, add, div, and, xor, or */
			case '+':
			case '/':
			case '&':
			case '^':
			case '|':
				if (!need_operator)
					goto error;
				switch (c) {
				case '%': op = OP_MOD; break;
				case '*': op = OP_MUL; break;
				case '+': op = OP_ADD; break;
				case '/': op = OP_DIV; break;
				case '&': op = OP_AND; break;
				case '^': op = OP_XOR; break;
				case '|': op = OP_OR;  break;
				}
				if (!push_op(op))
					return (0);
				expr++;
				break;

			/* skip immediate operand prefix if in macro */
			case '#':
				if (expand_macro)
					expr++;
				else
					end = 3;
				break;

			/* space or tab */
			case ' ':
			case '\t':
				expr++;
				break;

			/* end of line */
			case '\0':
				if (func_idx) {
					func_idx--;
					expr = expr_stack[func_idx];
					break;
				}
			case ';':
				end = 1;
				break;
			case ',':
				end = 2;
				break;
			default:
				end = 3;
				break;
			}
		}
	}

	if (!need_operator)
		goto error;
	if (level != 0)
		goto error;
	while (op_stack[op_idx] != OP_START) {
		if (!do_op())
			return (0);
	}

	/* get the expression value */
	value = val_stack[val_idx];

	/* any undefined symbols? trap that if in the last pass */
	if (undef) {
		if (pass == LAST_PASS)
			error("Undefined symbol in operand field!");
	}

	/* check if the last char is what the user asked for */
	switch (last_char) {
	case ';':
		if (end != 1)
			goto error;
		expr++;
		break;		
	case ',':
		if (end != 2) {
			error("Argument missing!");
			return (0);
		}
		expr++;
		break;		
	}

	/* convert back the pointer to an array index */
   *ip = (int)((void *)expr - (void *)prlnbuf);

	/* ok */
	return (1);

	/* syntax error */
error:
	error("Syntax error in expression!");
	return (0);
}


/* ----
 * push_val()
 * ----
 * extract a number and push it on the value stack
 */

int
push_val(int type)
{
	unsigned int mul, val;
	int op;
	char c;

	val = 0;
	c = *expr;

	switch (type) {
	/* program counter */
	case T_PC:
		if (data_loccnt == -1)
			val = B2ADDR(page, loccnt);
		else
			val = B2ADDR(page, data_loccnt);
		expr++;
		break;

	/* char ascii value */
	case T_CHAR:
		expr++;
		val = *expr++;
		if ((*expr != c) || (val == 0)) {
			error("Syntax Error!");
			return (0);
		}
		expr++;
		break;

	/* symbol */
	case T_SYMBOL:
		/* extract it */
		if (!getsym())
			return (0);

		/* an user function? */
		if (func_look()) {
			if (!func_getargs())
				return (0);

			expr_stack[func_idx++] = expr;
			expr = (unsigned char *)func_ptr->line;
			return (1);
		}

		/* a predefined function? */
		op = check_keyword();
		if (op) {
			if (!push_op(op))
				return (0);
			else
				return (1);
		}

		/* search the symbol */
		expr_lablptr = stlook(1);

		/* check if undefined, if not get its value */
		if (expr_lablptr == NULL)
			return (0);
		else if (expr_lablptr->type == UNDEF)
			undef++;
		else if (expr_lablptr->type == IFUNDEF)
			undef++;
		else
			val = expr_lablptr->value;

		/* remember we have seen a symbol in the expression */
		expr_lablcnt++;
		break;

	/* binary number %1100_0011 */
	case T_BINARY:
		mul = 2;
		goto extract;

	/* hexa number $15AF */
	case T_HEXA:
		mul = 16;
		goto extract;

	/* decimal number 48 (or hexa 0x5F) */
	case T_DECIMAL:
		if((c == '0') && (toupper(expr[1]) == 'X')) {
			mul = 16;
			expr++;
		}
		else {
			mul = 10;
			val = c - '0';
		}
		/* extract a number */
	extract:
		for (;;) {
			expr++;
			c = *expr;
			
			if (isdigit(c))
				c -= '0';
			else if (isalpha(c)) {
				c = toupper(c);
				if (c < 'A' && c > 'F')
					break;
				else {
					c -= 'A';
					c += 10;
				}
			}
			else if (c == '_' && mul == 2)
				continue;
			else
				break;
			if (c >= mul)
				break;
			val = (val * mul) + c;
		}
		break;
	}

	/* check for too big expression */
	if (val_idx == 63) {
		error("Expression too complex!");
		return (0);
	}

	/* push the result on the value stack */
	val_idx++;
	val_stack[val_idx] = val;

	/* next must be an operator */
	need_operator = 1;

	/* ok */
	return (1);
}


/* ----
 * getsym()
 * ----
 * extract a symbol name from the input string
 */

int
getsym(void)
{
	int	valid;
	int	i;
	char c;

	valid = 1;
	i = 0;

	/* get the symbol, stop to the first 'non symbol' char */
	while (valid) {
		c = *expr;
		if (isalpha(c) || c == '_' || c == '.' || (isdigit(c) && i >= 1)) {
			if (i < SBOLSZ - 1)
				symbol[++i] = c;
			expr++;
		}
		else {
			valid = 0;
		}
	}

	/* is it a reserved symbol? */
	if (i == 1) {
		switch (toupper(symbol[1])) {
		case 'A':
		case 'X':
		case 'Y':
			error("Symbol is reserved (A, X or Y)!");
			i = 0;
		}
	}

	/* store symbol length */	
#if SBOLSZ > CHAR_MAX
# error SBOLSZ must be less than or equal to CHAR_MAX.
#endif
	symbol[0] = i;
	symbol[i+1] = '\0';
	return (i);
}


/* ----
 * check_keyword()
 * ----
 * verify if the current symbol is a reserved function
 */

int
check_keyword(void)
{
	int op = 0;

#define IS_SAME(s1, s2)														\
((s1)[0] == (s2)[0] && !strcasecmp(&(s1)[1], &(s2)[1]))
	/* check if its an assembler function */
	if (IS_SAME(symbol, keyword[0]))
		op = OP_DEFINED;
	else if (IS_SAME(symbol, keyword[1]))
		op = OP_HIGH;
	else if (IS_SAME(symbol, keyword[2]))
		op = OP_LOW;
	else if (IS_SAME(symbol, keyword[3]))
		op = OP_PAGE;
	else if (IS_SAME(symbol, keyword[4]))
		op = OP_BANK;
	else if (IS_SAME(symbol, keyword[7]))
		op = OP_SIZEOF;
	else {
		if (machine->type == MACHINE_PCE) {
			/* PCE specific functions */
			if (IS_SAME(symbol, keyword[5]))
				op = OP_VRAM;
			else if (IS_SAME(symbol, keyword[6]))
				op = OP_PAL;
		}
	}

	/* extra setup for functions that send back symbol infos */
	switch (op) {
	case OP_DEFINED:
	case OP_HIGH:
	case OP_LOW:
	case OP_PAGE:
	case OP_BANK:
	case OP_VRAM:
	case OP_PAL:
	case OP_SIZEOF:
		expr_lablptr = NULL;
		expr_lablcnt = 0;
		break;
	}

	/* ok */
	return (op);
}


/* ----
 * push_op()
 * ----
 * push an operator on the stack
 */

int
push_op(int op)
{
	if (op != OP_OPEN) {
		while (op_pri[op_stack[op_idx]] >= op_pri[op]) {
			if (!do_op())
				return (0);
		}
	}
	if (op_idx == 63) {
		error("Expression too complex!");
		return (0);
	}
	op_idx++;
	op_stack[op_idx] = op;
	need_operator = 0;
	return (1);
}


/* ----
 * do_op()
 * ----
 * apply an operator to the value stack
 */

int
do_op(void)
{
	int val[2] = {0, 0};
	int op;

	/* operator */
	op = op_stack[op_idx--];

	/* first arg */
	val[0] = val_stack[val_idx];

	/* second arg */
	if (op_pri[op] < 9)
		val[1] = val_stack[--val_idx];

	switch (op) {
	/* BANK */
	case OP_BANK:
		if (!check_func_args("BANK"))
			return (0);
		if (pass == LAST_PASS) {
			if (expr_lablptr->bank == RESERVED_BANK) {
				error("No BANK index for this symbol!");
				val[0] = 0;
				break;
			}
		}
		val[0] = expr_lablptr->bank;
		break;

	/* PAGE */
	case OP_PAGE:
		if (!check_func_args("PAGE"))
			return (0);
		val[0] = expr_lablptr->page;
		break;

	/* VRAM */
	case OP_VRAM:
		if (!check_func_args("VRAM"))
			return (0);
		if (pass == LAST_PASS) {
			if (expr_lablptr->vram == -1)
				error("No VRAM address for this symbol!");
		}
		val[0] = expr_lablptr->vram;
		break;

	/* PAL */
	case OP_PAL:
		if (!check_func_args("PAL"))
			return (0);
		if (pass == LAST_PASS) {
			if (expr_lablptr->pal == -1)
				error("No palette index for this symbol!");
		}
		val[0] = expr_lablptr->pal;
		break;

	/* DEFINED */
	case OP_DEFINED:
		if (!check_func_args("DEFINED"))
			return (0);
		if ((expr_lablptr->type != IFUNDEF) && (expr_lablptr->type != UNDEF))
			val[0] = 1;
		else {
			val[0] = 0;
			undef--;
		}
		break;

	/* SIZEOF */
	case OP_SIZEOF:
		if (!check_func_args("SIZEOF"))
			return (0);
		if (pass == LAST_PASS) {
			if (expr_lablptr->data_type == -1) {
				error("No size attributes for this symbol!");
				return (0);
			}
		}
		val[0] = expr_lablptr->data_size;
		break;

	/* HIGH */
	case OP_HIGH:
		val[0] = (val[0] & 0xFF00) >> 8;
		break;		

	/* LOW */
	case OP_LOW:
		val[0] = val[0] & 0xFF;
		break;

	case OP_ADD:
		val[0] = val[1] + val[0];
        break;

	case OP_SUB:
		val[0] = val[1] - val[0];
		break;

	case OP_MUL:
		val[0] = val[1] * val[0];
        break;

    case OP_DIV:
        if (val[0] == 0) {
			error("Divide by zero!");
			return (0);
		}
        val[0] = val[1] / val[0];
        break;

	case OP_MOD:
        if (val[0] == 0) {
			error("Divide by zero!");
			return (0);
		}
        val[0] = val[1] % val[0];
        break;

    case OP_NEG:
		val[0] = -val[0];
        break;

    case OP_SHL:
        val[0] = val[1] << (val[0] & 0x1F);
        break;

    case OP_SHR:
        val[0] = val[1] >> (val[0] & 0x1f);
        break;

	case OP_OR:
        val[0] = val[1] | val[0];
		break;

    case OP_XOR:
        val[0] = val[1] ^ val[0];
		break;

    case OP_AND:
        val[0] = val[1] & val[0];
        break;

    case OP_COM:
        val[0] = ~val[0];
        break;

    case OP_NOT:
        val[0] = !val[0];
        break;

    case OP_EQUAL:
        val[0] = (val[1] == val[0]);
        break;

    case OP_NOT_EQUAL:
        val[0] = (val[1] != val[0]);
        break;

    case OP_LOWER:
        val[0] = (val[1] < val[0]);
        break;

    case OP_LOWER_EQUAL:
        val[0] = (val[1] <= val[0]);
        break;

    case OP_HIGHER:
        val[0] = (val[1] > val[0]);
        break;

    case OP_HIGHER_EQUAL:
        val[0] = (val[1] >= val[0]);
        break;

    default:
		error("Invalid operator in expression!");
		return (0);
    }

	/* result */
    val_stack[val_idx] = val[0];
    return (1);
}


/* ----
 * check_func_args()
 * ----
 * check BANK/PAGE/VRAM/PAL function arguments
 */

int
check_func_args(char *func_name)
{
	char string[64];

	if (expr_lablcnt == 1)
		return (1);
	else if (expr_lablcnt == 0)
		sprintf(string, "No symbol in function %s!", func_name);
	else {
		sprintf(string, "Too many symbols in function %s!", func_name);
	}

	/* output message */
	error(string);
	return (0);
}

