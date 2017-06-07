/** @file
   Testy jednostkowe funkcji PolyCompose i polecenia "COMPOSE"
   kalkulatora wielomianów

   @author Tomasz Necio <Tomasz.Necio@fuw.edu.pl>
   @copyright Uniwersytet Warszawski
   @date 2017-06-06
*/

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "cmocka.h"

#include "poly.h"

#define array_length(x) (sizeof(x) / sizeof((x)[0]))

extern int calculator_main();

/* * * TESTY FUNKCJI PolyCompose * * */


/** Test: p wielomian zerowy, count równe 0 */
static void test_poly_zero(void **state) {
	(void) state;

	Poly p = PolyZero();
	Poly r = PolyCompose(&p, 0, NULL);

	assert_true(PolyIsZero(&r));
	PolyDestroy(&p);
	PolyDestroy(&r);
}

/** Test: p wielomian zerowy, count równe 1, x[0] wielomian stały */
static void test_poly_zero_compose_scalar(void **state) {
	(void) state;

	Poly p = PolyZero();
	Poly q = PolyFromCoeff(2817);
	Poly r = PolyCompose(&p, 1, &q);

	assert_true(PolyIsZero(&r));
	PolyDestroy(&p);
	PolyDestroy(&q);
	PolyDestroy(&r);
}

/** Test: p wielomian stały, count równe 0 */
static void test_poly_scalar(void **state) {
	(void) state;

	int val = 3829;
	Poly p = PolyFromCoeff(val);
	Poly r = PolyCompose(&p, 0, NULL);

	Poly expected = PolyFromCoeff(val);
	assert_true(PolyIsEq(&r, &expected));
	PolyDestroy(&p);
	PolyDestroy(&r);
	PolyDestroy(&expected);
}

/** Test: p wielomian stały, count równe 1, x[0] wielomian stały =/= p */
static void test_poly_scalar_compose_scalar(void **state) {
	(void) state;

	int val = 1209;
	Poly p = PolyFromCoeff(val);
	Poly q = PolyFromCoeff(2817);
	Poly r = PolyCompose(&p, 1, &q);

	Poly expected = PolyFromCoeff(val);
	assert_true(PolyIsEq(&r, &expected));
	PolyDestroy(&p);
	PolyDestroy(&q);
	PolyDestroy(&r);
	PolyDestroy(&expected);
}

/** Pomocnicza funkcja, tworzy wielomian x0 */
Poly poly_x0() {
	Poly mp = PolyFromCoeff(1);
	Mono m = MonoFromPoly(&mp, 1);
	return PolyAddMonos(1, &m);
}

/** Test: p wielomian x0, count równe 0 */
static void test_poly_x(void **state) {
	(void) state;

	Poly p = poly_x0();
	Poly r = PolyCompose(&p, 0, NULL);
	Poly expected = poly_x0();

	assert_true(PolyIsEq(&r, &expected));
	PolyDestroy(&p);
	PolyDestroy(&r);
	PolyDestroy(&expected);
}

/** Test: p wielomian x0, count równe 1, x[0] wielomian stały */
static void test_poly_x_compose_scalar(void **state) {
	(void) state;

	int val = 21989;
	Poly p = poly_x0();
	Poly q = PolyFromCoeff(val);
	Poly r = PolyCompose(&p, 1, &q);
	Poly expected = PolyFromCoeff(val);

	assert_true(PolyIsEq(&r, &expected));
	PolyDestroy(&p);
	PolyDestroy(&q);
	PolyDestroy(&r);
	PolyDestroy(&expected);
}

/** Test: p wielomian x0, count równe 1, x[0] wielomian x0 */
static void test_poly_x_compose_x(void **state) {
	(void) state;

	Poly p = poly_x0();
	Poly q = poly_x0();
	Poly r = PolyCompose(&p, 1, &q);
	Poly expected = poly_x0();

	assert_true(PolyIsEq(&p, &expected));
	PolyDestroy(&p);
	PolyDestroy(&q);
	PolyDestroy(&r);
	PolyDestroy(&expected);
}


/* * * TESTY PARSERA * * */


/* Pomocnicze bufory, do których piszą atrapy funkcji printf i fprintf oraz
pozycje zapisu w tych buforach. Pozycja zapisu wskazuje bajt o wartości 0. TODO*/
static char fprintf_buffer[256];
static char printf_buffer[256];
static int fprintf_position = 0;
static int printf_position = 0;

/* Atrapa stdin */
static int stdin_buffer[256] = {'\0'};
static int stdin_position = 0;

/* Atrapa funkcji printf TODO */
int mock_printf(const char *format, ...) {
	int return_value;
	va_list args;

	/* Poniższa asercja sprawdza też, czy printf_position jest nieujemne.
	W buforze musi zmieścić się kończący bajt o wartości 0. */
	assert_true((size_t)printf_position < sizeof(printf_buffer));

	va_start(args, format);
	return_value = vsnprintf(printf_buffer + printf_position,
							 sizeof(printf_buffer) - printf_position,
							 format,
							 args);
	va_end(args);

	printf_position += return_value;
	assert_true((size_t)printf_position < sizeof(printf_buffer));
	return return_value;
}

/* Atrapa funkcji fprintf TODO */
int mock_fprintf(FILE* const file, const char *format, ...) {
	int return_value;
	va_list args;

	assert_true(file == stderr);
	/* Poniższa asercja sprawdza też, czy fprintf_position jest nieujemne.
	W buforze musi zmieścić się kończący bajt o wartości 0. */
	assert_true((size_t)fprintf_position < sizeof(fprintf_buffer));

	va_start(args, format);
	return_value = vsnprintf(fprintf_buffer + fprintf_position,
							 sizeof(fprintf_buffer) - fprintf_position,
							 format,
							 args);
	va_end(args);

	fprintf_position += return_value;
	printf(format);
	assert_true((size_t)fprintf_position < sizeof(fprintf_buffer));
	return return_value;
}

/** Atrapa getchar */
int mock_getchar(void) {
	assert_true((size_t)stdin_position < sizeof(stdin_buffer));

	int r = stdin_buffer[stdin_position];
	stdin_position++;
	return r;
}

/** Atrapa ungetc */
int mock_ungetc(int __c, FILE *__stream) {
	assert_true(stdin_position > 0);
	stdin_position--;
	stdin_buffer[stdin_position] = __c;
	return 0;
}

/** Atrapa fgets */
int mock_fgets(char *__s, int __n, FILE *__stream) {
	assert_true(__n < 256 - stdin_position);
	int ch;
	for (unsigned i = 0; i < __n; i++){
		ch = stdin_buffer[stdin_position + i];
		__s[i] = ch;
		if (ch == '\n') {
			__s[i + 1] = '\0';
			break;
		}
	}
}

/** Test: brak parametru */
static void test_no_argument(void **state) {
	(void) state;

	strcpy(stdin_buffer, "COMPOSE\n");
	stdin_buffer[0] = 'C';
	stdin_buffer[1] = 'O';
	stdin_buffer[2] = 'M';
	stdin_buffer[3] = 'P';
	stdin_buffer[4] = 'O';
	stdin_buffer[5] = 'S';
	stdin_buffer[6] = 'E';
	stdin_buffer[7] = '\n';
	stdin_buffer[8] = EOF;
	printf(stdin_buffer);
	calculator_main();
	printf(fprintf_buffer);
	assert_int_equal(strcmp(printf_buffer, ""), 0);
	assert_int_equal(strcmp(fprintf_buffer, "ERROR 1 WRONG COMMAND\n"), 0);
}

/** Test: minimalna wartość, czyli 0 */
/** Test: maksymalna wartość reprezentowana w typie unsigned */
/** Test: wartość o jeden mniejsza od minimalnej, czyli −1 */
/** Test: wartość o jeden większa od maksymalnej reprezentowanej w typie unsigned */
/** Test: duża dodatnia wartość, znacznie przekraczająca zakres typu unsigned */
/** Test: kombinacja liter */
/** Test: kombinacja cyfr i liter, rozpoczynająca się cyfrą */


int main(void) {
	const struct CMUnitTest tests_poly[] = {
		cmocka_unit_test(test_poly_zero),
		cmocka_unit_test(test_poly_zero_compose_scalar),
		cmocka_unit_test(test_poly_scalar),
		cmocka_unit_test(test_poly_scalar_compose_scalar),
		cmocka_unit_test(test_poly_x),
		cmocka_unit_test(test_poly_x_compose_scalar),
		cmocka_unit_test(test_poly_x_compose_x),
	};

	const struct CMUnitTest tests_parser[] = {
		cmocka_unit_test(test_no_argument)
	};

	int r = cmocka_run_group_tests(tests_poly, NULL, NULL);
	r += cmocka_run_group_tests(tests_parser, NULL, NULL);
	return r;
}
