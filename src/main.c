#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "poly.h"
#include "stack.h"

#define COMMAND 1
#define POLYNOMIAL 2
#define MAX_MONOS_COUNT 200
#define MAX_SIZE_COMMAND 50
#define

#define UNDERFLOW_ERR "ERROR %d STACK UNDERFLOW\n"
#define PARSING_ERR "ERROR %d %d\n"
#define WRONG_COMMAND_ERR "ERROR %d WRONG COMMAND\n"
#define WRONG_VALUE_ERR "ERROR %d WRONG VALUE\n"
#define WRONG_VARIABLE_ERR "ERROR %d WRONG VARIABLE\n"


/* I/O AND STRING PROCESSING AUXILLIARY FUNCTIONS */

void PrintError(char* msg, unsigned row, int column) {
	fprintf(stderr, msg, row + 1, column + 1);
}

void BoolPrint(bool val) {
	if (val) {
		printf("1\n");
	} else {
		printf("0\n");
	}
}


bool IsLetter(int c) {
	if (('z' >= c && c >= 'a') || ('Z' >= c && c >= 'A')) {
		return true;
	}
	return false;
}

int SeeChar() {
	int c = getchar();
	ungetc(c, stdin);
	return c;
}

bool GetCharByReference();

/* I/O <-> INTERNAL REPR. CONVERSION FUNCTIONS */

void PolyPrint(Poly *p);

void MonoPrint(Mono *m) {
	printf("(");
	PolyPrintAux(&(m->p));
	printf(",%d)", m->exp);
}

void PolyPrintAux(Poly *p) {
	if (PolyIsCoeff(p)) {
		printf("%d", (int) p->scalar);
	} else {
		if (p->scalar != 0) {
			printf("(%d,0)", (int) p->scalar);
		}
		for (unsigned i = 0; i < p->monos_count; i++) {
			if (i > 0 || p->scalar != 0) {
				printf("+");
			}
			MonoPrint(&(p->monos[i]));
		}
	}
}

void PolyPrint(Poly *p) {
	PolyPrintAux(p);
	printf("\n");
}

Poly PolyParseStdin(char start, unsigned row, unsigned *column);

Mono MonoParse(char *in, unsigned row, unsigned *column) {
	(*column)++;
	Poly p = PolyParseStdin(in, column, row);
	if ((*column)[0] != ',') {
		PrintError(PARSING_ERR, row, (*column) - in);
	}
	(*column)++;
	poly_exp_t e = strtol(*column, column, 10);
	if ((*column)[0] != ')') {
		PrintError(PARSING_ERR, row, (*column) - in);
	}
	(*column)++;
	return MonoFromPoly(&p, e);
}

Poly PolyParseStdin(unsigned row, unsigned *column) {
	if (SeeChar() == '(') { /* array of monos */
		Mono monos[MAX_MONOS_COUNT]; // !! źle, ich moze byc w ch..
		unsigned monos_count = 0;

		while (true) {
			if (monos_count == MAX_MONOS_COUNT) {
				monos = GrowArray(&monos); // TODO
			}

			monos[monos_count] = MonoParse(in, row, column);
			monos_count++;
			if (() != '+') {
				break;
			}
			(*column)++;
		}
		return PolyAddMonos(monos_count, monos);

	} else /* a scalar */ {
		char number[MAX_SIZE_INT_BASE_10]; // max no of digit for an int in base 10
		int ch = getchar();
		for (unsigned i = 0; IsDigit(ch); i++) {
			number[i] = (char) ch;
			ch = getchar();
		}
		ungetc(stdin); // we leave the non-digit where we found it
		number[i + 1] = 0;
		poly_coeff_t r = strtol(*number, NULL, 10);
		return PolyFromCoeff(r);
	}
}


/* OPERATIONS ON POLYS ON THE STACK */

void ActOnTwoPolysOnStack(Stack *s, unsigned row,
						  Poly (*op)(const Poly *, const Poly *)) {
	if (IsEmpty(s)) {
		PrintError(UNDERFLOW_ERR, row, 0);
		return;
	}
	Poly p = Pop(s);

	if (IsEmpty(s)) {
		PrintError(UNDERFLOW_ERR, row, 0);
		return;
	}
	Poly q = Pop(s);

	Poly r = op(&p, &q);
	PolyDestroy(&p);
	PolyDestroy(&q);
	Push(s, &r);
}

void AddTwoPolysFromStack(Stack *s, unsigned row) {
	return ActOnTwoPolysOnStack(s, row, PolyAdd);
}

void MultiplyTwoPolysFromStack(Stack *s, unsigned row) {
	return ActOnTwoPolysOnStack(s, row, PolyMul);
}

void SubtractTwoPolysFromStack(Stack *s, unsigned row) {
	return ActOnTwoPolysOnStack(s, row, PolySub);
}

bool AreEqualTwoPolysFromStack(Stack *s) {
	Poly p = Pop(s);
	Poly q = GetTop(s);
	bool r = PolyIsEq(&p, &q);
	Push(s, &p);
	return r;
}


/* THE PROGRAM */

void ExecuteCommand(Stack *s, char *command, unsigned row) {
	if (strcmp(command, "ZERO") == 0) {
		Poly p = PolyZero();
		Push(s, &p);

	} else if (strcmp(command, "IS_COEFF") == 0) {
		Poly top = GetTop(s);
		BoolPrint(PolyIsCoeff(&top));

	} else if (strcmp(command, "IS_ZERO") == 0) {
		Poly top = GetTop(s);
		BoolPrint(PolyIsZero(&top));

	} else if (strcmp(command, "CLONE") == 0) {
		Poly top = GetTop(s);
		Poly p = PolyClone(&top);
		Push(s, &p);

	} else if (strcmp(command, "ADD") == 0) {
		AddTwoPolysFromStack(s, row);

	} else if (strcmp(command, "MUL") == 0) {
		MultiplyTwoPolysFromStack(s, row);

	} else if (strcmp(command, "NEG") == 0) {
		Poly p = Pop(s);
		Poly q = PolyNeg(&p);
		PolyDestroy(&p);
		Push(s, &q);

	} else if (strcmp(command, "SUB") == 0) {
		SubtractTwoPolysFromStack(s, row);

	} else if (strcmp(command, "IS_EQ") == 0) {
		BoolPrint(AreEqualTwoPolysFromStack(s));

	} else if (strcmp(command, "DEG") == 0) {
		Poly top = GetTop(s);
		printf("%d\n", PolyDeg(&top));

	} else if (strcmp(command, "PRINT") == 0) {
		Poly top = GetTop(s);
		PolyPrint(&top);
		printf("\n");

	} else if (strcmp(command, "POP") == 0) {
		Poly p = Pop(s);
		PolyDestroy(&p);

	} else if (strncmp(command, "DEG_BY ", 7) == 0) { // TODO ERROR MSGS
		Poly top = GetTop(s);
		printf("%d\n", PolyDegBy(&top, strtol(command + 7, NULL, 10)));

	} else if (strncmp(command, "AT ", 3) == 0) { // TODO ERROR MSGS
		Poly top = GetTop(s);
		Poly p = PolyAt(&top, strtol(command + 3, NULL, 10));
		PolyPrint(&p);

	} else {
		PrintError(WRONG_COMMAND_ERR, row, 0);
	}
}


int main() {
	Stack s = StackEmpty();

	// debug
	FILE *file;
	file = fopen("/home/tom/Programy/C/wielomiany/test.txt", "r");

	//while (fgets(buf, sizeof buf, stdin)) {
	int ch;
	char command[MAX_SIZE_COMMAND];

	unsigned row = 0;
	unsigned column = 1;

	while ((ch = SeeChar()) != EOF) {

		if (IsLetter(ch)) {
			fgets(command, MAX_SIZE_COMMAND, stdin);
			command[strcspn(command, "\r\n")] = 0; // \r for portability
			ExecuteCommand(&s, command, row);

		} else {
			Poly p = PolyParseStdin(row, &column);
			Push(&s, &p);
		}

		row++;
	}
	StackDestroy(&s);
}
