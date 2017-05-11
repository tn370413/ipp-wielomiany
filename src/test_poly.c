#include "poly.h"
#include "const_arr.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define ALL_TESTS "all"
#define MEMORY "memory"
#define LONG_POLYNOMIAL "long-polynomial"
#define DEG "deg"
#define DEG_SIMPLE "deg-simple"
#define DEG_OP "deg-op"
#define DEG_BY "deg-by"
#define SIMPLE_AT "simple-at"
#define SIMPLE_AT2 "simple-at2"
#define AT "at"
#define MUL_SIMPLE "mul-simple"
#define MUL "mul"
#define ADD "add"
#define ADD_REQ "add-req"
#define SUB "sub"
#define SUB_REQ "sub-req"
#define EQ "eq"
#define EQ_SIMPLE "eq-simple"
#define RARE "rare"
#define MONO_ADD "mono-add"
#define OVERFLOW "overflow"
#define SIMPLE_ARITHMETIC "simple-aritmethic"
#define SIMPLE_ARITHMETIC2 "simple-aritmethic2"

int main() {
	Poly p0 = PolyFromCoeff(2);
	Mono m0 = MonoFromPoly(&p0, 1); // 2x
	Mono monos0[1];
	monos0[0] = m0;
	Poly p01 = PolyAddMonos(1, monos0);
	PolyDestroy(&p01);


	Poly p = PolyFromCoeff(7);
	Mono m = MonoFromPoly(&p, 2); // 7x^2
	Poly p1 = PolyFromCoeff(14);
	Mono m1 = MonoFromPoly(&p1, 3); // 14x^3
	Mono m2 = MonoClone(&m1); // 14x^3
	Mono monos[1];
	monos[0] = m2;
	Poly p3 = PolyAddMonos(1, monos); // p3(x) = 14x^3
	Mono m3 = MonoFromPoly(&p3, 4); // 14y^3x^4
	Mono monos1[2];
	monos1[0] = m3;
	monos1[1] = m;
	Mono monos2[1];
	monos2[0] = m1;
	Poly p4 = PolyAddMonos(1, monos2); // p4(x) = 14x^3
	Poly p5 = PolyAddMonos(2, monos1); // p5(x) = 7x^2 + 14y^3x^4

	Poly p6 = PolyAdd(&p5, &p4); // p6(x) = 7x^2 + 14x^3 + 14y^3x^4
	poly_coeff_t r = PolyAt(&p6, 3).scalar; // 9*7 + 14 * 27 = 441

	PolyDestroy(&p);
	PolyDestroy(&p1);
//	PolyDestroy(&p2);
	PolyDestroy(&p3);
	PolyDestroy(&p4);
	PolyDestroy(&p5);

	return r;
}
