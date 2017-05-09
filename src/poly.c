#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "poly.h"

struct MonosWithCount {
	int count;
	Mono *list;
};

void PolyDestroy(Poly *p) {
	for (int i = 0; i < p->monos_count; i++) {
		MonoDestroy(&(p->monos[i]));
	}
	free(p->monos);
	p->constant = 0;
	p->monos_count = 0;
}

Poly PolyClone(const Poly *p) {
	Poly new_p;
	new_p.constant = p->constant;
	new_p.monos = calloc(sizeof(Mono), p->monos_count);
	for (int i = 0; i < p->monos_count; i++) { /* MAKE INTO FUNCT. */
		new_p.monos[i] = MonoClone(&(p->monos[i]));
	}
	new_p.monos_count = p->monos_count;
	return new_p;
}

Mono MonoAdd(const Mono *a, const Mono *b) {
	assert(a->exp == b->exp);
	Mono m;
	m.exp = a->exp;
	m.p = PolyAdd(&(a->p), &(b->p));
	return m;
}

Poly PolyAdd(const Poly *p, const Poly *q) {
	Poly r;

	if (PolyIsCoeff(p)) {
		r = PolyClone(q);
		r.constant += p->constant;
		return r;
	} else if (PolyIsCoeff(q)) {
		r = PolyClone(p);
		r.constant += q->constant;
		return r;
	}

	r.monos = calloc(sizeof(Mono), p->monos_count + q->monos_count);

	int i = 0;
	int j = 0;
	bool end_of_polynomial;

	while (!end_of_polynomial) {
		if (p->monos[i].exp < (q->monos[j]).exp) {
			r.monos[i + j] = MonoClone(&(p->monos[i]));
			i++;
		} else if (&(p->monos[i]).exp > &(q->monos[j]).exp) {
			r.monos[i + j] = MonoClone(&(q->monos[j]));
			j++;
		} else {
			r.monos[i + j] = MonoAdd(&(p->monos[i]), &(q->monos[j]));
			i++;
			j++;
		}

		if (i == p->monos_count) {
			end_of_polynomial = true;
			for (; j < q->monos_count; j++) {
				r.monos[i + j] = MonoClone(&(q->monos[j]));
			}
		} else if (j == q->monos_count) {
			end_of_polynomial = true;
			for (; i < p->monos_count; i++){
				r.monos[i + j] = MonoClone(&(p->monos[i]));
			}
		}
	}
	r.monos_count = i + j;
	return r;
}

int MonoExpCompare(const Mono *a, const Mono *b) {
	if (a->exp == b->exp) {
		return 0;
	} else if (a->exp < b->exp) {
		return -1;
	} else {
		return 1;
	}
}

Mono *SortMonosByExp(unsigned count, Mono monos[]) {
	qsort(monos, count, sizeof(Mono), MonoExpCompare);
	return monos;
}
/* MAKE FUNCTION FOR SIMPLIFYING MONOS */
Poly PolyAddMonos(unsigned count, const Mono monos[]){
	Poly p;
	p.constant = 0;
	p.monos_count = count;
	p.monos = calloc(sizeof(Mono), count);

	monos = SortMonosByExp(count, monos);

	int j = 0;
	for (int i = 0; i < count; i++) {

		if (monos[i].exp == 0) {
			if (PolyIsCoeff(&(monos[i].p))){
				p.constant += monos[i].p.constant;
				continue;
			}
			p.monos[0] = monos[0];
			continue;
		}

		if (p.monos != NULL && monos[i].exp == p.monos[j].exp) {
			p.monos[j].p = PolyAdd(&(p.monos[j].p), &(monos[i].p));
		} else {
			j++;
			p.monos[j] = monos[i];
		}
	}

	return p;
}

Mono MonoScalarMul(const Mono *a, const poly_coeff_t scalar) {
	Mono b = MonoClone(a);
	b.p.constant *= scalar;
	for (int i = 0; i < b.p.monos_count; i++) {
		b.p.monos[i] = MonoScalarMul(&(b.p.monos[i]), scalar);
	}
		 // TYPES – POINTER OR COPY??
	return b;
}

struct MonosWithCount SimplifyMonoArray(unsigned count, Mono *monos) {
	monos = SortMonosByExp(count, monos);
	int i = 0;
	for (int j = 1; j < count; j++) {
		Poly coeff = monos[j].p;
		if (PolyIsZero(&coeff)) {
			continue;
		}

		if (monos[j].exp == monos[i].exp) {
			monos[i] = MonoAdd(&(monos[i]), &(monos[j]));
		} else {
			i++;
			monos[i] = monos[j];
		}
	}

	struct MonosWithCount r;
	r.list = monos;
	r.count = i + 1;
	return r;
}

Mono MonoMul(const Mono *a, const Mono *b) {
	Mono r;
	r.p = PolyMul(&(a->p), &(b->p));
	r.exp = a->exp + b->exp;
	return r;
}

Poly PolyMul(const Poly *p, const Poly *q) {
	Poly r;
	r.constant = p->constant * q->constant;
	int monos_count = (p->monos_count + 1) * (q->monos_count + 1);
	Mono monos[monos_count];
	int index = 0;

	for (int i = 0; i < p->monos_count; i++) {
		for (int j = 0; j < q->monos_count; j++) {
			monos[index] = MonoMul(&(p->monos[i]), &(q->monos[j]));
			index++;
		}
	}

	for (int i = 0; i < p->monos_count; i++) {
		monos[index] = MonoScalarMul(&(p->monos[i]), q->constant);
		index++;
	}

	for (int j = 0; j < q->monos_count; j++) {
		monos[index] = MonoScalarMul(&(q->monos[j]), p->constant);
		index++;
	}

	struct MonosWithCount m = SimplifyMonoArray(monos_count, monos);
	r = PolyAddMonos(m.count, m.list);
	return r;
}

Poly PolyNeg(const Poly *p) {
	Poly r = PolyClone(p);
	r.constant = -p->constant;
	for (int i = 0; i < r.monos_count; i++) {
		r.monos[i].p = PolyNeg(&(r.monos[i]).p);
	}
	return r;
}

Poly PolySub(const Poly *p, const Poly *q) {
	Poly minus_q = PolyNeg(q);
	return PolyAdd(p, &minus_q);
}

Poly PolyScalarMul(const Poly *p, poly_coeff_t scalar) {
	Poly r = PolyClone(p);
	for (int i = 0; i < p->monos_count; i++) {
		r.monos[i] = MonoScalarMul(&(p->monos[i]), scalar);
	}
	return r;
}

Poly PolyAt(const Poly *p, poly_coeff_t x) {
	Poly r = PolyZero();
	for (int i = 0; i < p->monos_count; i++) {
		poly_exp_t exponent = p->monos[i].exp;
		Poly q = PolyScalarMul(&(p->monos[i].p), pow(x, exponent));
		r = PolyAdd(&r, &q);
	}
	return r;
}

poly_exp_t PolyDegBy(const Poly *p, unsigned var_idx) {
	Poly r;

	if (PolyIsZero(p)) {
		return -1;
	} else if (PolyIsCoeff(p)) {
		return 0;
	}

	for (int i = 0; i < var_idx; i++) {
		r = PolyAt(p, 1);
	}

	return r.monos[r.monos_count - 1].exp;
}

poly_exp_t PolyDeg(const Poly *p) {
	if (PolyIsZero(p)) {
		return -1;
	}
	int r = 0;
	poly_exp_t m;
	for (int i = 0; i < p->monos_count; i++) {
		m = PolyDeg(&(p->monos[i]).p) + &(p->monos[i]).exp;
		if (m > r) {
			r = m;
		}
	}
	return r;
}

bool MonoIsEq(const Mono *p, const Mono *q) {
	if (p->exp != q->exp) {
		return false;
	}
	return PolyIsEq(&(p->p), &(q->p));
}

bool PolyIsEq(const Poly *p, const Poly *q) {
	if (p->constant != q->constant || p->monos_count != q->monos_count) {
		return false;
	}

	for (int i = 0; i < p->monos_count; i++) {
		if (!MonoIsEq(&(p->monos[i]), &(q->monos[i]))) {
			return false;
		}
	}

	return true;
}
