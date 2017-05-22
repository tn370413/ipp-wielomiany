#ifndef __STACK_H__
#define __STACK_H__

#include <stdlib.h>

#include "poly.h"

#define STACK_SIZE 1000

typedef struct Stack {
	size_t element_count;
	Poly* elements;
} Stack;

static inline Stack StackEmpty() {
	Stack s;
	s.element_count = 0;
	s.elements = calloc(STACK_SIZE, sizeof(Poly));
	return s;
}

static inline void StackDestroy(Stack *s) {
	free(s->elements);
}

static inline void Push(Stack *s, Poly *p) {
	s->elements[s->element_count] = *p;
	s->element_count++;
}

static inline Poly Pop(Stack *s) {
	s->element_count--;
	return s->elements[s->element_count];
}

static inline Poly GetTop(Stack *s) {
	return s->elements[s->element_count - 1];
}

static inline bool IsEmpty(Stack *s) {
	return (s->element_count == 0);
}

#endif
