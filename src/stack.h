#ifndef __STACK_H__
#define __STACK_H__

#include <stdlib.h>

#include "poly.h"

#define STACK_SIZE 1000

typedef struct Stack {
	size_t element_count;
	size_t size;
	Poly* elements;
} Stack;

static inline Stack StackEmpty() {
	Stack s;
	s.element_count = 0;
	s.size = STACK_SIZE;
	s.elements = calloc(STACK_SIZE, sizeof(Poly));
	return s;
}

static inline void StackDestroy(Stack *s) {
	free(s->elements);
	s->element_count = 0;
	s->size = 0;
}

static inline void GrowStack(Stack *s) {
	s->size *= 2;
	s->elements = (Poly *) realloc(s->elements, s->size * sizeof(Poly));
}

static inline void Push(Stack *s, Poly *p) {
	if (s->element_count == s->size) {
		GrowStack(s);
	}
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
