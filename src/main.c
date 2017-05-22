#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "poly.h"
#include "stack.h"

#define COMMAND 1
#define POLYNOMIAL 2
#define MAX_MONOS_COUNT 200

void BoolPrint(bool val) {
	if (val) {
		fprintf(stdout, "1\n");
	} else {
		fprintf(stdout, "0\n");
	}
}

void ActOnTwoPolysOnStack(Stack *s,
						  Poly (*op)(const Poly *, const Poly *)) {
	Poly p = Pop(s);
	Poly q = Pop(q);
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

void MonoPrint(Mono *m) {
	fprintf(stdout, "(");
	PolyPrint(m->p);
	fprintf(stdout, ",%d)", m->exp);
}

void PolyPrint(Poly *p) {
	if (PolyIsCoeff(p)) {
		fprintf(stdout, "%d", p->scalar);
	} else {
		if (p->scalar != 0) {
			fprintf(stdout, "(%d,0)", p->scalar);
		}
		for (unsigned i = 0; i < p->monos_count; i++) {
			if (i > 0 || p->scalar != 0) {
				fprintf(stdout, "+");
			}
			MonoPrint(&(p->monos[i]));
		}
	}
}

void PrintError(int row, int column) {
	fprintf(stdout, "ERROR %d %d\n", row, column);
}

Mono MonoParse(char *in, char **pos, unsigned row) {
	char c;
	(*pos)++;
	Poly p = PolyParse(in, pos, row);
	if ((*pos)[0] != ',') {
		PrintError(row, (*pos) - in);
	}
	(*pos)++;
	poly_exp_t e = strtol(*pos, pos, 10);
	if ((*pos)[0] != ')') {
		PrintError(row, (*pos) - in);
	}
	(*pos)++;
	return MonoFromPoly(&p, e);
}

Poly PolyParse(char *in, char **pos, unsigned row) {
	if ((*pos)[0] == '(') {
		/* array of monos */
		Mono monos[MAX_MONOS_COUNT];
		unsigned monos_count = 0;
		char c;
		while (true) {
			monos[monos_count] = MonoParse(in, pos, row);
			monos_count++;
			if ((*pos)[0] != '+') {
				break;
			}
		}
		return PolyAddMonos(monos_count, monos);

	} else /* a scalar */ {
		poly_coeff_t r = strtol(*pos, pos, 10);
		return PolyFromCoeff(r);
	}
}

void ExecuteCommand(Stack *s, char *command, unsigned r) {
	Poly *top = &(GetTop(s));
	if (strcmp(command, "ZERO") == 0) {
		Push(s, PolyZero());
	} else if (strcmp(command, "IS_COEFF") == 0) {
		BoolPrint(PolyIsCoeff(top));
	} else if (strcmp(command, "IS_ZERO") == 0) {
		BoolPrint(PolyIsZero(top));
	} else if (strcmp(command, "CLONE") == 0) {
		Push(s, PolyClone(top));
	} else if (strcmp(command, "ADD") == 0) {
		AddTwoPolysFromStack(s);
	} else if (strcmp(command, "MUL") == 0) {
		MultiplyTwoPolysFromStack(s);
	} else if (strcmp(command, "NEG") == 0) {
		Push(s, PolyNeg(&(Pop(s))));
	} else if (strcmp(command, "SUB") == 0) {
		SubtractTwoPolysFromStack(s);
	} else if (strcmp(command, "IS_EQ") == 0) {
		BoolPrint(AreEqualTwoPolysFromStack(s));
	} else if (strcmp(command, "DEG") == 0) {
		fprintf(stdout, "%d\n", PolyDeg(top));
	} else if (strcmp(command, "PRINT") == 0) {
		PolyPrint(top);
		fprintf(stdout, "\n");
	} else if (strcmp(command, "POP") == 0) {
		Pop(s);
	} else if (strncmp(command, "DEG_BY ", 7) == 0) {
		fprintf(stdout, "%d\n", PolyDegBy(top, strtol(command + 7, NULL, 10)));
	} else if (strncmp(command, "AT ", 3) == 0) {
		PolyPrint(&(PolyAt(top, strtol(command + 3, NULL, 10))));
	} else {
		Poly p = PolyParse(command, &command, r);
		Push(s, &p);
	}
}

int main() {
	char buf[BUFSIZ];
	Stack s = StackEmpty();
	i = 0;
	while (fgets(buf, sizeof buf, stdin)) {
		ExecuteCommand(&s, buf, i);
		i++;
	}
	StackDestroy(&s);
}
