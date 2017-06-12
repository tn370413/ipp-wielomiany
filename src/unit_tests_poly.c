/** @file
   Testy jednostkowe funkcji PolyCompose i polecenia "COMPOSE"
   kalkulatora wielomianów

   Użyto fragmentów "calculator_test.c" by
 * Copyright 2008 Google Inc.
 * Copyright 2015 Tomasz Kociumaka
 * Copyright 2016, 2017 IPP team
   udostępnionego na licencji Apache (http://www.apache.org/licenses/LICENSE-2.0)

   @author Tomasz Necio <Tomasz.Necio@fuw.edu.pl>
   @copyright Uniwersytet Warszawski
   @date 2017-06-06
*/

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "cmocka.h"

#include "poly.h"

/**
  * oblicza wielkość tablicy w jednostce rozmiaru pojedynczego elementu zamiast w bajtach
  */
#define array_length(x) (sizeof(x) / sizeof((x)[0]))

extern int calculator_main(); ///< umożliwia używanie main'a z głównego pliku

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
	Poly expected = PolyZero();

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


static jmp_buf jmp_at_exit; ///< pozwala przywrócić stan środowiska po wywołaniu głównego maina
static int exit_status; ///< wartość zwracana przez main po wykonaniu


/**
 * Atrapa funkcji main
 */
int mock_main() {
	if (!setjmp(jmp_at_exit))
		return calculator_main();
	return exit_status;
}

/**
 * Atrapa funkcji exit
 */
void mock_exit(int status) {
	exit_status = status;
	longjmp(jmp_at_exit, 1);
}


/* Pomocnicze bufory, do których piszą atrapy funkcji printf i fprintf oraz
pozycje zapisu w tych buforach. Pozycja zapisu wskazuje bajt o wartości 0. */
static char fprintf_buffer[256]; ///< bufer pseudo wyjścia błędu
static char printf_buffer[256]; ///< bufer pseudo wyjścia stdout
static int fprintf_position = 0; ///< aktualna pozycja na pseudobuf. błędu
static int printf_position = 0; ///< aktualna pozycja na pseudobuf. stdout


/**
 * Atrapa funkcji printf
 */
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

/**
 * Atrapa funkcji fprintf
 */
int mock_fprintf(FILE* const file, const char *format, ...) {
	int return_value;
	va_list args;

	if (file == stdout) {
		return mock_printf(format, args);
	}

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
	assert_true((size_t)fprintf_position < sizeof(fprintf_buffer));
	return return_value;
}


/*
 *  Pomocniczy bufor, z którego korzystają atrapy funkcji operujących na stdin.
 */
static char input_stream_buffer[256]; ///< pseudobufor wejścia
static int input_stream_position = 0; ///< pozycja na pseudobuf. wejścia
static int input_stream_end = 0; ///< liczba wskazująca gdzie kończy się pseudowejście
int read_char_count; ///< licznik wczytanych z pseudowejścia znaków

/**
 * Atrapa funkcji scanf używana do przechwycenia czytania z stdin.
 */
int mock_scanf(const char *format, ...) {
	va_list fmt_args;
	int ret;

	va_start(fmt_args, format);
	ret = vsscanf(input_stream_buffer + input_stream_position, format, fmt_args);
	va_end(fmt_args);

	if (ret < 0) { /* ret == EOF */
		input_stream_position = input_stream_end;
	}
	else {
		assert_true(read_char_count >= 0);
		input_stream_position += read_char_count;
		if (input_stream_position > input_stream_end) {
			input_stream_position = input_stream_end;
		}
	}
	return ret;
}

/**
 * Atrapa funkcji fgets używana do przechwycenia czytania z stdin.
 */
char *mock_fgets(char *__s, int __n, FILE *__stream) {
	assert_true(__stream == stdin);

	char *in = input_stream_buffer + input_stream_position;
	FILE *stream = fmemopen(in, strlen(in), "r");
	int new_line_pos = strcspn(in, "\n");
	input_stream_position += (new_line_pos + 1);

	return fgets(__s, __n, stream);
}


/**
 * Atrapa funkcji getchar używana do przechwycenia czytania z stdin.
 */
int mock_getchar() {
	if (input_stream_position < input_stream_end)
		return input_stream_buffer[input_stream_position++];
	else
		return EOF;
}

/**
 * Atrapa funkcji ungetc.
 * Obsługiwane jest tylko standardowe wejście.
 */
int mock_ungetc(int c, FILE *stream) {
	assert_true(stream == stdin);
	if (input_stream_position > 0)
		return input_stream_buffer[--input_stream_position] = c;
	else
		return EOF;
}

/**
 * Funkcja inicjująca dane wejściowe dla programu korzystającego ze stdin.
 */
static void init_input_stream(const char *str) {
	memset(input_stream_buffer, 0, sizeof(input_stream_buffer));
	input_stream_position = 0;
	input_stream_end = strlen(str);
	assert_true((size_t)input_stream_end < sizeof(input_stream_buffer));
	strcpy(input_stream_buffer, str);
}

/**
 * Funkcja wołana przed każdym testem.
 */
static int test_setup(void **state) {
	(void)state;

	memset(fprintf_buffer, 0, sizeof(fprintf_buffer));
	memset(printf_buffer, 0, sizeof(printf_buffer));
	printf_position = 0;
	fprintf_position = 0;

	/* Zwrócenie zera oznacza sukces. */
	return 0;
}

/**
 * Funkcja wołana po każdym teście.
 */
static int test_teardown(void **state) {
	(void)state;


	/* Zwrócenie zera oznacza sukces. */
	return 0;
}


/** Test: brak parametru */
static void test_no_argument(void **state) {
	(void) state;

	init_input_stream("COMPOSE");

	assert_int_equal(mock_main(), 0);

	assert_int_equal(strcmp(printf_buffer, ""), 0);
	assert_int_equal(strcmp(fprintf_buffer, "ERROR 1 WRONG COUNT\n"), 0);
}

/** Test: minimalna wartość, czyli 0, gdy na stosie nie ma wielomianu */
static void test_compose_0_empty(void **state) {
	(void) state;

	init_input_stream("COMPOSE 0");

	assert_int_equal(mock_main(), 0);

	assert_int_equal(strcmp(printf_buffer, ""), 0);
	assert_int_equal(strcmp(fprintf_buffer, "ERROR 1 STACK UNDERFLOW\n"), 0);
}

/** Test: minimalna wartość, czyli 0, gdy na stosie jest wielomian */
static void test_compose_0_full(void **state) {
	(void) state;

	init_input_stream("(1,1)\nCOMPOSE 0\nPRINT");

	assert_int_equal(mock_main(), 0);

	assert_int_equal(strcmp(printf_buffer, "0\n"), 0);
	assert_int_equal(strcmp(fprintf_buffer, ""), 0);
}


/** Test: maksymalna wartość reprezentowana w typie unsigned */
static void test_compose_unsigned_max(void **state) {
	(void) state;

	char command[30];
	sprintf(command, "(1,1)\nCOMPOSE %lu\n", (long unsigned) UINT_MAX);
	init_input_stream(command);

	assert_int_equal(mock_main(), 0);

	assert_int_equal(strcmp(printf_buffer, ""), 0);
	assert_int_equal(strcmp(fprintf_buffer, "ERROR 2 STACK UNDERFLOW\n"), 0);
}

/** Test: wartość o jeden mniejsza od minimalnej, czyli −1 */
static void test_compose_unsigned_max_minus_1(void **state) {
	(void) state;

	char command[30];
	sprintf(command, "(1,1)\nCOMPOSE %lu\n",(long unsigned) UINT_MAX - 1);
	init_input_stream(command);

	assert_int_equal(mock_main(), 0);

	assert_int_equal(strcmp(printf_buffer, ""), 0);
	assert_int_equal(strcmp(fprintf_buffer, "ERROR 2 STACK UNDERFLOW\n"), 0);
}

/** Test: wartość o jeden większa od maksymalnej reprezentowanej w typie unsigned */
static void test_compose_unsigned_max_plus_1(void **state) {
	(void) state;

	char command[30];
	sprintf(command, "(1,1)\nCOMPOSE %lu\n", ((unsigned long) UINT_MAX) + 1);
	init_input_stream(command);

	assert_int_equal(mock_main(), 0);

	assert_int_equal(strcmp(printf_buffer, ""), 0);
	assert_int_equal(strcmp(fprintf_buffer, "ERROR 2 WRONG COUNT\n"), 0);
}

/** Test: duża dodatnia wartość, znacznie przekraczająca zakres typu unsigned */
static void test_compose_big_int(void **state) {
	(void) state;

	char command[30];
	sprintf(command, "(1,1)\nCOMPOSE 20000%d\n", UINT_MAX);
	init_input_stream(command);

	assert_int_equal(mock_main(), 0);

	assert_int_equal(strcmp(printf_buffer, ""), 0);
	/* Wyjaśnienie: program akceptuje jedynie komendy do 25 znaków.
	 * komendy o większej liczbie znaków są traktowane od razu
	 * jako błędne i niebadane pod kątem zawierania liczby.*/
	assert_int_equal(strcmp(fprintf_buffer, "ERROR 2 WRONG COUNT\n"), 0);
}

/** Test: kombinacja liter */
static void test_compose_letters(void **state) {
	(void) state;

	init_input_stream("(1,1)\nCOMPOSE bc\n");

	assert_int_equal(mock_main(), 0);

	assert_int_equal(strcmp(printf_buffer, ""), 0);
	assert_int_equal(strcmp(fprintf_buffer, "ERROR 2 WRONG COUNT\n"), 0);
}

/** Test: kombinacja cyfr i liter, rozpoczynająca się cyfrą */
static void test_compose_digits_and_letters(void **state) {
	(void) state;

	init_input_stream("(1,1)\nCOMPOSE 7bc\n");

	assert_int_equal(mock_main(), 0);

	assert_int_equal(strcmp(printf_buffer, ""), 0);
	assert_int_equal(strcmp(fprintf_buffer, "ERROR 2 WRONG COUNT\n"), 0);
}

/**
 * Główna funkcja tetsująca
 * @return sumaryczny wynik testów (>0 = źle)
 */
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
		cmocka_unit_test_setup_teardown(test_no_argument, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_compose_0_empty, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_compose_0_full, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_compose_unsigned_max, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_compose_unsigned_max_minus_1, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_compose_unsigned_max_plus_1, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_compose_big_int, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_compose_letters, test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_compose_digits_and_letters, test_setup, test_teardown)
	};

	int r = cmocka_run_group_tests(tests_poly, NULL, NULL);
	r += cmocka_run_group_tests(tests_parser, NULL, NULL);
	return r;
}
