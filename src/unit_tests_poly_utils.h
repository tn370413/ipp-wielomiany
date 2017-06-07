/*
 * Copyright 2008 Google Inc.
 * Copyright 2015 Tomasz Kociumaka
 * Copyright 2016, 2017 IPP team
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UTILS_H
#define UTILS_H

/* If this is being built for a unit test. */
#ifdef UNIT_TESTING

#include <stdio.h>

/* Redirect printf to a function in the test application so it's possible to
 * test the standard output. *//*
#ifdef printf
#undef printf
#endif  printf
#define printf(...) mock_printf(__VA_ARGS__)
extern int mock_printf(const char *format, ...); */

/* Redirect fprintf to a function in the test application so it's possible to
 * test error messages. */
#ifdef fprintf
#undef fprintf
#endif /* fprintf */
#define fprintf(...) mock_fprintf(__VA_ARGS__)
extern int mock_fprintf(FILE * const file, const char *format, ...);

#ifdef getchar
#undef getchar
#endif /* getchar */
#define getchar(...) mock_getchar(__VA_ARGS__)
extern int mock_getchar(void);

#ifdef ungetc
#undef ungetc
#endif /* ungetc */
#define ungetc(...) mock_ungetc(__VA_ARGS__)
extern int mock_ungetc(int __c, FILE *__stream);

#ifdef fgets
#undef fgets
#endif /* fgets */
#define fgets(...) mock_fgets(__VA_ARGS__)
extern int mock_fgets(char *__s, int __n, FILE *__stream);

/* Redirect assert to mock_assert() so assertions can be caught by cmocka. */
#ifdef assert
#undef assert
#endif /* assert */
#define assert(expression) \
	mock_assert((int)(expression), #expression, __FILE__, __LINE__)
void mock_assert(const int result, const char* expression, const char *file,
				 const int line);

/* Redirect calloc, malloc, realloc and free to _test_malloc, _test_calloc,
 * _test_realloc and _test_free, respectively, so cmocka can check for memory
 * leaks. TODO */
#ifdef calloc
#undef calloc
#endif /* calloc */
#define calloc(num, size) _test_calloc(num, size, __FILE__, __LINE__)
#ifdef malloc
#undef malloc
#endif /* malloc */
#define malloc(size) _test_malloc(size, __FILE__, __LINE__)
#ifdef realloc
#undef realloc
#endif /* realloc */
#define realloc(ptr, size) _test_realloc(ptr, size, __FILE__, __LINE__)
#ifdef free
#undef free
#endif /* free */
#define free(ptr) _test_free(ptr, __FILE__, __LINE__)
void* _test_calloc(size_t number_of_elements, size_t size, const char* file, int line);
void* _test_malloc(size_t size, char* const file, int line);
void* _test_realloc(void* const ptr, size_t size, char* const file, int line);
void _test_free(void* const ptr, char* const file, const int line);

/* Function main is defined in the unit test so redefine name of the main
 * function here. TODO */
#define main(...) calculator_main(__VA_ARGS__)
int calculator_main();

/* All functions in this object need to be exposed to the test application,
 * so redefine static to nothing. Do not do it - it dangerous! */
/* #define static */

#endif /* UNIT_TESTING */

#endif /* UTILS_H*/
