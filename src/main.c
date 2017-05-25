#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "poly.h"
#include "stack.h"

#define INITIAL_MONOS_SIZE 200
#define MAX_COMMAND_LENGTH 24

#define POLY_EXP_T 1
#define POLY_COEFF_T 2
#define UNSIGNED 3

#define UNDERFLOW_ERR_MSG "ERROR %d STACK UNDERFLOW\n"
#define PARSING_ERR_MSG "ERROR %d %d\n"
#define WRONG_COMMAND_ERR_MSG "ERROR %d WRONG COMMAND\n"
#define WRONG_VALUE_ERR_MSG "ERROR %d WRONG VALUE\n"
#define WRONG_VARIABLE_ERR_MSG "ERROR %d WRONG VARIABLE\n"

#define EOF_FLAG 1
#define UNDERFLOW_ERR_FLAG 2
#define PARSING_ERR_FLAG 3
#define WRONG_COMMAND_ERR_FLAG 4
#define WRONG_VALUE_ERR_FLAG 5
#define WRONG_VARIABLE_ERR_FLAG 6

/* Ideas on how to get cleaner code:
 * 1. join common parts of NumberParser and StringToLong
 * 2. have a common way of checking if the next characer \in allowed characters
 *    if last_character != '\n'
 */


/* * * PARSER STATE * * */

/* These variables will hold the state of the whole parser */
unsigned row = 0;
unsigned column = 0;
int error_flag = 0;
char last_character;

/* Dummy structs for quick escape from functions on error */
Poly DUMMY_POLY;
Mono DUMMY_MONO;

void ErrorSetFlag(int flag) {
	error_flag = flag;
}

int Error() {
	return error_flag;
}

void ErrorPrint(char* msg) {
	fprintf(stderr, msg, row + 1, column);
}

void ErrorHandle() {
	switch (error_flag) {
	case UNDERFLOW_ERR_FLAG:
		ErrorPrint(UNDERFLOW_ERR_MSG);
		break;
	case PARSING_ERR_FLAG:
		ErrorPrint(PARSING_ERR_MSG);
		break;
	case WRONG_COMMAND_ERR_FLAG:
		ErrorPrint(WRONG_COMMAND_ERR_MSG);
		break;
	case WRONG_VALUE_ERR_FLAG:
		ErrorPrint(WRONG_VALUE_ERR_MSG);
		break;
	case WRONG_VARIABLE_ERR_FLAG:
		ErrorPrint(WRONG_VARIABLE_ERR_MSG);
		break;
	default:
		break;
	}
	error_flag = 0;
}

/* * * OUTPUT FUNCTIONS * * */

void BoolPrint(bool val) {
	if (val) {
		printf("1\n");
	} else {
		printf("0\n");
	}
}

void PolyPrint(Poly *p);

void MonoPrint(Mono *m) {
	printf("(");
	PolyPrint(&(m->p));
	printf(",%d)", m->exp);
}

// move to poly.h
Poly PolyPrepareForPrint(Poly *p, bool *memory_flag) {
	if (p->scalar != 0 && p->monos_count > 1 && p->monos[0].exp == 0) {
		Poly r = PolyClone(p);
		r.scalar = 0;
		r.monos[0].p.scalar += p->scalar;
		*memory_flag = true;
		return r;
	}
	return *p;
}

void PolyPrint(Poly *p) {
	if (PolyIsCoeff(p)) { // check this code if bugs happen
		printf("%ld", p->scalar);
	} else {
		bool was_memory_allocated = false;
		Poly q = PolyPrepareForPrint(p, &was_memory_allocated);

		if (q.scalar != 0) {
			printf("(%ld,0)", q.scalar);
		}
		for (unsigned i = 0; i < q.monos_count; i++) {
			if (i > 0 || q.scalar != 0) {
				printf("+");
			}
			MonoPrint(&(q.monos[i]));
		}

		if (was_memory_allocated) {
			PolyDestroy(&q);
		}
	}
}


/* * * INPUT FUNCTIONS * * */


int SeeChar() {
	int r = getchar();
	ungetc(r, stdin);
	return r;
}


int GetChar() {
	column++;
	last_character = getchar();
	return last_character;
}

void UnGetChar(int ch) {
	column--;
	ungetc(ch, stdin);
}

bool IsLetter(int ch) {
	return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}

bool IsDigit(int ch, bool exclude_minus) {
	return ('0' <= ch && ch <= '9') || (!exclude_minus && (ch == '-'));
}

void GoToNextLine(bool *eof_flag) {
	if (last_character == '\n') {
		getchar();
		return;
	}

	int ch = SeeChar();

	if (!Error() && ch != '\n') {
		column++;
		ErrorSetFlag(PARSING_ERR_FLAG);
	}

	while (Error() && ch != '\n') {
		getchar();
		ch = SeeChar();
		if (ch == EOF) {
			*eof_flag = EOF_FLAG;
			return;
		}
	}

	ch = getchar(); // should be '\n'
	if (ch == EOF) {
		*eof_flag = true;
	}
}

Poly PolyParse();

void ValidateNextDigit(long r, int digit, long max_val) {
	if (r > max_val / 10 ||
			(r == max_val / 10 && digit > max_val % 10)) {
		ErrorSetFlag(PARSING_ERR_FLAG);
	}
}

long NumberParse(int type) {
	if (!IsDigit(SeeChar(), false)) {
		GetChar();
		ErrorSetFlag(PARSING_ERR_FLAG);
		return 0;
	}

	bool minus = false;
	if (SeeChar() == '-') {
		if (type == POLY_EXP_T || type == UNSIGNED) {
			ErrorSetFlag(PARSING_ERR_FLAG);
			return 0;
		}
		minus = true;
		GetChar();
	}

	int ch;
	int digit;
	unsigned i = 0;
	long r = 0;
	while (IsDigit(ch = GetChar(), true)) {
		digit = ch - 48;

		switch (type) {
		case POLY_EXP_T:
			ValidateNextDigit(r, digit, POLY_EXP_MAX);
			break;
		case POLY_COEFF_T:
			ValidateNextDigit(r, digit, POLY_COEFF_MAX);
			break;
		case UNSIGNED:
			ValidateNextDigit(r, digit, UINT32_MAX);
			break;
		default:
			assert(false);
			break;
		}
		if (Error()) {
			return 0;
		}

		r = 10 * r + digit;
		i++;
	}
	UnGetChar(ch);

	if (minus) {
		return -r;
	}
	return r;
}

Mono MonoParse() {
	if (GetChar() != '(') {
		ErrorSetFlag(PARSING_ERR_FLAG);
		return DUMMY_MONO;
	}

	Poly p = PolyParse();
	if (Error()) { return DUMMY_MONO; }

	if (GetChar() != ',') {
		ErrorSetFlag(PARSING_ERR_FLAG);
		return DUMMY_MONO;
	}

	int ch = SeeChar();
	if (!IsDigit(ch, false)) {
		GetChar();
		ErrorSetFlag(PARSING_ERR_FLAG);
		return DUMMY_MONO;
	}

	poly_exp_t e = NumberParse(POLY_EXP_T);
	if (Error()) { return DUMMY_MONO; }

	if (GetChar() != ')') {
		ErrorSetFlag(PARSING_ERR_FLAG);
		return DUMMY_MONO;
	}

	return MonoFromPoly(&p, e);
}

Poly PolyParse() {
	if (SeeChar() == '(') { /* array of monos */
		size_t monos_size = INITIAL_MONOS_SIZE;
		Mono *monos = calloc(monos_size, sizeof(Mono));
		assert(monos != NULL);
		unsigned monos_count = 0;

		int c;
		while (true) {
			if (monos_count == monos_size) {
				monos_size *= 2;
				monos = realloc(monos, monos_size * sizeof(Mono));
				assert(monos != NULL);
			}

			monos[monos_count] = MonoParse();
			if (Error()) {
				free(monos);
				return DUMMY_POLY;
			}

			monos_count++;
			if ((c = SeeChar()) != '+') {
				break;
			} else {
				GetChar(); // +
			}
		}

		Poly p = PolyAddMonos(monos_count, monos);
		free(monos);
		return p;

	} else /* a scalar */ {
		poly_coeff_t r = NumberParse(POLY_COEFF_T);
		if (Error()) { return DUMMY_POLY; }
		return PolyFromCoeff(r);
	}
}


/* * * POLY OPERATIONS * * */

Poly GetTopSafely(Stack *s) {
	if (IsEmpty(s)) {
		ErrorSetFlag(UNDERFLOW_ERR_FLAG);
		return DUMMY_POLY;
	}
	return GetTop(s);
}

Poly PopSafely(Stack *s) {
	if (IsEmpty(s)) {
		ErrorSetFlag(UNDERFLOW_ERR_FLAG);
		return DUMMY_POLY;
	}
	return Pop(s);
}

void ActOnTwoPolysOnStack(Stack *s,
						  Poly (*op)(const Poly *, const Poly *)) {
	Poly p = PopSafely(s);
	if (Error()) { return; }

	Poly q = PopSafely(s);
	if (Error()) {
		Push(s, &p);
		return;
	}

	Poly r = op(&p, &q);
	PolyDestroy(&p);
	PolyDestroy(&q);

	Push(s, &r);
}

void AddTwoPolysFromStack(Stack *s) {
	return ActOnTwoPolysOnStack(s, PolyAdd);
}

void MultiplyTwoPolysFromStack(Stack *s) {
	return ActOnTwoPolysOnStack(s, PolyMul);
}

void SubtractTwoPolysFromStack(Stack *s) {
	return ActOnTwoPolysOnStack(s, PolySub);
}

void PrintAreEqualTwoPolysFromStack(Stack *s) {
	Poly p = PopSafely(s);
	if (Error()) { return; }

	Poly q = GetTopSafely(s);
	if (Error()) {
		Push(s, &p);
		return;
	}

	bool r = PolyIsEq(&p, &q);
	Push(s, &p);
	BoolPrint(r);
}

void IsCoeffPolyOnStack(Stack *s) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	BoolPrint(PolyIsCoeff(&top));
}

void IsZeroPolyOnStack(Stack *s) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	BoolPrint(PolyIsZero(&top));
}

void ClonePolyOnStack(Stack *s) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	Poly p = PolyClone(&top);
	Push(s, &p);
}

void NegatePolyOnStack(Stack *s) {
	Poly p = PopSafely(s);
	if (Error()) { return; }

	Poly q = PolyNeg(&p);
	PolyDestroy(&p);
	Push(s, &q);
}

void PrintDegreePolyOnStack(Stack *s) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	printf("%d\n", PolyDeg(&top));
}

void PrintPolyOnStack(Stack *s) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	PolyPrint(&top);
	printf("\n");
}

long StringToLong(char *c, int type) {
	if (!IsDigit(c[0], false)) {
		ErrorSetFlag(PARSING_ERR_FLAG);
		return 0;
	}

	bool minus = false;
	if (c[0] == '-') {
		if (type == POLY_EXP_T || type == UNSIGNED) {
			ErrorSetFlag(PARSING_ERR_FLAG);
			return 0;
		}
		minus = true;
		c++;
	}

	int ch;
	int digit;
	unsigned i = 0;
	long r = 0;
	while (IsDigit(ch = c[i], true)) {
		digit = ch - 48;

		switch (type) {
		case POLY_EXP_T:
			ValidateNextDigit(r, digit, POLY_EXP_MAX);
			break;
		case POLY_COEFF_T:
			ValidateNextDigit(r, digit, POLY_COEFF_MAX);
			break;
		case UNSIGNED:
			ValidateNextDigit(r, digit, UINT32_MAX);
			break;
		default:
			assert(false);
			break;
		}
		if (Error()) {
			return 0;
		}

		r = 10 * r + digit;
		i++;
	}

	if (ch != '\0') {
		ErrorSetFlag(PARSING_ERR_FLAG);
		return 0;
	}

	if (minus) {
		return -r;
	}
	return r;
}

void PrintDegBy(Stack *s, char *command) {
	unsigned arg = StringToLong(command + 7, UNSIGNED);
	if (Error()) {
		ErrorSetFlag(WRONG_VARIABLE_ERR_FLAG);
		return;
	}

	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	int r = PolyDegBy(&top, arg);
	printf("%d\n", r);
}

void CalculatePolyAt(Stack *s, char *command) {
	poly_coeff_t arg = StringToLong(command + 3, POLY_COEFF_T);
	if (Error()) {
		ErrorSetFlag(WRONG_VALUE_ERR_FLAG);
		return;
	}

	Poly top = PopSafely(s);
	if (Error()) { return; }

	Poly p = PolyAt(&top, arg);
	PolyDestroy(&top);
	Push(s, &p);
}

/* * * THE PROGRAM * * */


void ExecuteCommand(Stack *s, char *command) {
	if (strcmp(command, "ZERO") == 0) {
		Poly p = PolyZero();
		Push(s, &p);

	} else if (strcmp(command, "IS_COEFF") == 0) {
		IsCoeffPolyOnStack(s);

	} else if (strcmp(command, "IS_ZERO") == 0) {
		IsZeroPolyOnStack(s);

	} else if (strcmp(command, "CLONE") == 0) {
		ClonePolyOnStack(s);

	} else if (strcmp(command, "ADD") == 0) {
		AddTwoPolysFromStack(s);

	} else if (strcmp(command, "MUL") == 0) {
		MultiplyTwoPolysFromStack(s);

	} else if (strcmp(command, "NEG") == 0) {
		NegatePolyOnStack(s);

	} else if (strcmp(command, "SUB") == 0) {
		SubtractTwoPolysFromStack(s);

	} else if (strcmp(command, "IS_EQ") == 0) {
		PrintAreEqualTwoPolysFromStack(s);

	} else if (strcmp(command, "DEG") == 0) {
		PrintDegreePolyOnStack(s);

	} else if (strcmp(command, "PRINT") == 0) {
		PrintPolyOnStack(s);

	} else if (strcmp(command, "POP") == 0) {
		Poly p = PopSafely(s);
		if (Error()) { return; }
		PolyDestroy(&p);

	} else if (strncmp(command, "DEG_BY ", 7) == 0) {
		PrintDegBy(s, command);

	} else if (strncmp(command, "AT ", 3) == 0) {
		CalculatePolyAt(s, command);

	} else {
		ErrorSetFlag(WRONG_COMMAND_ERR_FLAG);
	}
}

int main() {
	Stack s = StackEmpty();

	char command_buf[MAX_COMMAND_LENGTH] = {'\0'};
	int ch;
	bool eof_flag = false;

	while (!eof_flag) {
		column = 0;
		ch = SeeChar();
		if (ch == EOF) {
			break;
		} else if (IsLetter(ch)) {
			fgets(command_buf, sizeof command_buf, stdin);
			command_buf[strcspn(command_buf, "\r\n")] = 0;
			ExecuteCommand(&s, command_buf);
			ErrorHandle();
		} else {
			Poly p = PolyParse();
			GoToNextLine(&eof_flag);
			if (!(Error())) {
				Push(&s, &p);
			} else {
				ErrorHandle();
			}
		}
		row++;
	}

	StackDestroy(&s);
}
