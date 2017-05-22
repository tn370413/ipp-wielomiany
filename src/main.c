#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "poly.h"
#include "stack.h"

#define COMMAND 1
#define POLYNOMIAL 2
#define MAX_MONOS_COUNT 200

#define UNDERFLOW_ERR "ERROR %d STACK UNDERFLOW\n"
#define PARSING_ERR "ERROR %d %d\n"
#define WRONG_COMMAND_ERR "ERROR %d WRONG COMMAND\n"
#define WRONG_VALUE_ERR "ERROR %d WRONG VALUE\n"
#define WRONG_VARIABLE_ERR "ERROR %d WRONG VARIABLE\n"


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

Poly PolyParse(char *in, char **pos, unsigned row);

Mono MonoParse(char *in, char **pos, unsigned row) {
	(*pos)++;
	Poly p = PolyParse(in, pos, row);
	if ((*pos)[0] != ',') {
		PrintError(PARSING_ERR, row, (*pos) - in);
	}
	(*pos)++;
	poly_exp_t e = strtol(*pos, pos, 10);
	if ((*pos)[0] != ')') {
		PrintError(PARSING_ERR, row, (*pos) - in);
	}
	(*pos)++;
	return MonoFromPoly(&p, e);
}

Poly PolyParse(char *in, char **pos, unsigned row) {
	if ((*pos)[0] == '(') {
		/* array of monos */
		Mono monos[MAX_MONOS_COUNT];
		unsigned monos_count = 0;
		while (true) {
			monos[monos_count] = MonoParse(in, pos, row);
			monos_count++;
			if ((*pos)[0] != '+') {
				break;
			}
			(*pos)++;
		}
		return PolyAddMonos(monos_count, monos);

	} else /* a scalar */ {
		poly_coeff_t r = strtol(*pos, pos, 10);
		return PolyFromCoeff(r);
	}
}

void ExecuteCommand(Stack *s, char *command, unsigned row) {
	char start = command[0];
	if (('z' >= start && start >= 'a') || ('Z' >= start && start >= 'A')) {
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
	} else {
		Poly p = PolyParse(command, &command, row);
		Push(s, &p);
	}
}

int main() {
	char buf[BUFSIZ];
	Stack s = StackEmpty();
	unsigned i = 0;

	// debug
	FILE *file;
	file = fopen("/home/tom/Programy/C/wielomiany/test.txt", "r");

	//while (fgets(buf, sizeof buf, stdin)) {
	while (fgets(buf, sizeof buf, file)) {
		buf[strcspn(buf, "\r\n")] = 0; // portability..
		ExecuteCommand(&s, buf, i);
		i++;
	}
	StackDestroy(&s);
}
