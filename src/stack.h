/** @file
   Implementacja i interfejs stosu wielomianów rzadkich wielu zmiennych.

   @author Tomasz Necio <Tomasz.Necio@fuw.edu.pl>
   @copyright Uniwersytet Warszawski
   @date 2017-05-27
*/


#ifndef __STACK_H__
#define __STACK_H__

#include <assert.h>
#include <stdlib.h>

#include "poly.h"

#define STACK_SIZE 65536 ///< Początkowy rozmiar stosu

/** Struktura przechowująca stos wielomianów **/
typedef struct Stack {
	size_t element_count; ///< liczba elementów obecnie na stosie
	size_t size; ///< obecna pojemność stosu
	Poly* elements; ///< tablica elementów stosu
} Stack;

/**
 * Tworzy pusty stos
 * @return pusty stos
 */
static inline Stack StackEmpty() {
	Stack s;
	s.element_count = 0;
	s.size = STACK_SIZE;
	s.elements = calloc(STACK_SIZE, sizeof(Poly));
	assert(s.elements != NULL);
	return s;
}

/**
 * Usuwa stos z pamięci
 * @param[in] s : stos
 */
static inline void StackDestroy(Stack *s) {
	for (unsigned i = 0; i < s->element_count; i++) {
		PolyDestroy(&(s->elements[i]));
	}
	free(s->elements);
	s->element_count = 0;
	s->size = 0;
}

/**
 * Powiększa maksymalną pojemność stosu.
 * @param[in] s : stos
 */
static inline void GrowStack(Stack *s) {
	s->size *= 2;
	s->elements = (Poly *) realloc(s->elements, s->size * sizeof(Poly));
	assert(s->elements != NULL);
}

/**
 * Wrzuca wielomian na stos.
 * @param[in] s : stos
 * @param[in] p : wielomian do wrzucenia na stos
 */
static inline void Push(Stack *s, Poly *p) {
	if (s->element_count == s->size) {
		GrowStack(s);
	}
	s->elements[s->element_count] = *p;
	s->element_count++;
}

/**
 * Zrzuca wielomian ze stosu i zwraca go.
 * @param[in] s : stos
 * @return wielomian z wierzchu stosu
 */
static inline Poly Pop(Stack *s) {
	s->element_count--;
	return s->elements[s->element_count];
}

/**
 * Zwraca wielomian leżąy na wierzchu stosu.
 * @param[in] s : stos
 * @return wielomian na wierzchu stosu
 */
static inline Poly GetTop(Stack *s) {
	return s->elements[s->element_count - 1];
}

/**
 * Sprawdza, czy stos jest pusty.
 * @param[in] s : stos
 * @return Czy stos jest pusty?
 */
static inline bool IsEmpty(Stack *s) {
	return (s->element_count == 0);
}

#endif
