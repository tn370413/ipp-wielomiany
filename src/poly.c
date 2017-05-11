#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>

#define uint unsigned int // TODO STG WITH THIS

#include "poly.h"


Mono GetNthMono(Mono *list, int n) {
	return list[n];
}

Mono *GetNthMonoPtr(Mono *list, int n) {
	return list + n * sizeof(Mono);
}

void InsertNthMono(Mono list[], int n, Mono m) {
	list[n] = m;
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

void SortMonosByExp(unsigned count, Mono monos[]) {
	qsort(monos, count, sizeof(Mono), MonoExpCompare);
}


/**
 * Usuwa wielomian z pamięci.
 * @param[in] p : wielomian
 */
void PolyDestroy(Poly *p) {
	if (PolyIsCoeff(p)) {
		return;
	}

	for (uint i = 0; i < p->monos_count; i++) {
		MonoDestroy(GetNthMonoPtr(p->monos, i));
	}

	free(p->monos);
	p->monos = NULL;
	p->monos_count = 0;
}

/**
 * Robi pełną, głęboką kopię wielomianu.
 * @param[in] p : wielomian
 * @return skopiowany wielomian
 */
Poly PolyClone(const Poly *p){
	Poly np;
	np.scalar = p->scalar;
	np.monos_count = p->monos_count;
	np.monos = (Mono *) calloc(p->monos_count, sizeof(Mono));
	for (uint i = 0; i < p->monos_count; i++) {
		np.monos[i] = MonoClone(GetNthMonoPtr(p->monos, i));
	}
	return np;
}

/**
 * Dodaje dwa wielomiany.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p + q`
 */

Poly PolyAdd(const Poly *p, const Poly *q) { // O(n)
	Poly r;
	r.scalar = p->scalar + q->scalar;
	r.monos_count = 0;
	r.monos = (Mono *) calloc(p->monos_count + q->monos_count, sizeof(Mono));
	// TODO realloc

	uint i = 0;
	uint j = 0;

	Mono *pm;
	Mono *qm;

	while (true) {
		if (i == p->monos_count) {
			for (; j < q->monos_count; j++) {
				InsertNthMono(r.monos, r.monos_count,
							  MonoClone(GetNthMonoPtr(q->monos, j)));
				r.monos_count++;
			}
			break; // TODO BREAK
		} else if (j == q->monos_count) {
			for (; i < p->monos_count; i++) {
				InsertNthMono(r.monos, r.monos_count,
							  MonoClone(GetNthMonoPtr(p->monos, i)));
				r.monos_count++;
			}
			break;
		} else {

			pm = GetNthMonoPtr(p->monos, i);
			qm = GetNthMonoPtr(q->monos, j);

			if (pm->exp == qm->exp) {
				Poly m_coeff = PolyAdd(&(pm->p), &(qm->p));
				InsertNthMono(r.monos, r.monos_count,
							  MonoFromPoly(&m_coeff, pm->exp));
				i++;
				j++;
			} else if (pm->exp > qm->exp) {
				InsertNthMono(r.monos, r.monos_count, MonoClone(qm));
				j++;
			} else /* pm->exp < pq->exp */ {
				InsertNthMono(r.monos, r.monos_count, MonoClone(pm));
				i++;
			}
		}

		r.monos_count++;
	}
	//SortMonosByExp(r.monos_count, r.monos);
	return r;
}

/**
 * Sumuje listę jednomianów i tworzy z nich wielomian.
 * Przejmuje na własność zawartość tablicy @p monos.
 * @param[in] count : liczba jednomianów
 * @param[in] monos : tablica jednomianów
 * @return wielomian będący sumą jednomianów
 */
Poly PolyAddMonos(unsigned count, const Mono *monos){
	Poly p;
	p.scalar = 0;
	p.monos = calloc(count, sizeof(Mono));
	for (uint i = 0; i < count; i++) {
		p.monos[i] = monos[i];
	}
	SortMonosByExp(count, p.monos);
	p.monos_count = count;
	return p;
}

/**
 * Mnoży dwa wielomiany.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p * q`
 */

Mono MonoScalarMul(const Mono *m, poly_coeff_t scalar);

Poly PolyScalarMul(const Poly *p, poly_coeff_t scalar) {
	if (PolyIsCoeff(p)) {
		return PolyFromCoeff(p->scalar * scalar);
	}

	Poly r;
	r.scalar = p->scalar * scalar;
	r.monos_count = p->monos_count;
	r.monos = (Mono *) calloc(r.monos_count, sizeof(Mono));
	for (uint i = 0; i < p->monos_count; i++) {
		InsertNthMono(r.monos, i,
					  MonoScalarMul(GetNthMonoPtr(p->monos, i), scalar));
	}
	return r;
}

Mono MonoScalarMul(const Mono *m, poly_coeff_t scalar) {
	Mono r;
	r.exp = m->exp;
	r.p = PolyScalarMul(&(m->p), scalar);
	return r;
}


void SimplifyPoly(Poly *p) {
	SortMonosByExp(p->monos_count, p->monos);
	uint index = 0;
	/* TODO: take care of monos with exp=0 e.g. y, yz, 7x^0 etc. */
	for (uint i = 1; i < p->monos_count; i++) {
		Mono mi = GetNthMono(p->monos, i);
		Mono mp = GetNthMono(p->monos, index);
		if (mi.exp == mp.exp) {
			Poly np = PolyAdd(&mi.p, &mp.p);
			PolyDestroy(&mi.p);
			PolyDestroy(&mp.p);
			p->monos[index].p = np;
		} else {
			index++;
			p->monos[index] = mi;
		}
	}
	p->monos_count = index + 1;
}

Poly PolyMul(const Poly *p, const Poly *q);

Mono MonoMul(const Mono *m, const Mono *o) {
	Mono r;
	r.exp = m->exp + o->exp;
	r.p = PolyMul(&(m->p), &(o->p));
	return r;
}

Poly PolyMul(const Poly *p, const Poly *q) {
	if (PolyIsCoeff(p)) {
		return PolyScalarMul(q, p->scalar);
	}
	if (PolyIsCoeff(q)) {
		return PolyScalarMul(p, q->scalar);
	}

	Poly r1 = PolyScalarMul(p, q->scalar);
	Poly r2 = PolyScalarMul(q, p->scalar);

	Poly r12 = PolyAdd(&r1, &r2);
	r12.scalar = p->scalar * q->scalar;

	Poly r3;
	r3.scalar = 0;
	r3.monos_count = p->monos_count * q->monos_count;
	r3.monos = (Mono *) calloc(r3.monos_count, sizeof(Mono));

	for (uint i = 0; i < p->monos_count; i++) {
		for (uint j = 0; j < q->monos_count; j++) {
			Mono *mp = GetNthMonoPtr(p->monos, i);
			Mono *mq = GetNthMonoPtr(q->monos, j);
			InsertNthMono(r3.monos, j + q->monos_count * i,
						  MonoMul(mp, mq));
		}
	}
	SimplifyPoly(&r3);

	Poly r = PolyAdd(&r12, &r3);

	PolyDestroy(&r1);
	PolyDestroy(&r2);
	PolyDestroy(&r12);
	PolyDestroy(&r3);

	return r;
}


Mono MonoNeg(const Mono *m) {
	Mono r;
	r.exp = m->exp;
	r.p = PolyNeg(&(m->p));
	return r;
}

/**
 * Zwraca przeciwny wielomian.
 * @param[in] p : wielomian
 * @return `-p`
 */
Poly PolyNeg(const Poly *p) {
	if (PolyIsCoeff(p)) {
		return PolyFromCoeff(-p->scalar);
	}

	Poly r;
	r.scalar = -p->scalar;
	r.monos_count = p->monos_count;
	r.monos = (Mono *) calloc(r.monos_count, sizeof(Mono));
	for (uint i = 0; i < p->monos_count; i++) {
		InsertNthMono(r.monos, i, MonoNeg(GetNthMonoPtr(p->monos, i)));
	}
	return r;
}

/**
 * Odejmuje wielomian od wielomianu.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p - q`
 */
Poly PolySub(const Poly *p, const Poly *q) {
	Poly n = PolyNeg(q);
	Poly r = PolyAdd(p, &n);
	PolyDestroy(&n);
	return r;
}

/**
 * Zwraca stopień wielomianu ze względu na zadaną zmienną (-1 dla wielomianu
 * tożsamościowo równego zeru).
 * Zmienne indeksowane są od 0.
 * Zmienna o indeksie 0 oznacza zmienną główną tego wielomianu.
 * Większe indeksy oznaczają zmienne wielomianów znajdujących się
 * we współczynnikach.
 * @param[in] p : wielomian
 * @param[in] var_idx : indeks zmiennej
 * @return stopień wielomianu @p p z względu na zmienną o indeksie @p var_idx
 */
poly_exp_t PolyDegBy(const Poly *p, unsigned var_idx){
	if (PolyIsZero(p)) {
		return -1;
	}
	if (PolyIsCoeff(p)) {
		return 0;
	}

	if (var_idx == 0) {
		return GetNthMonoPtr(p->monos, p->monos_count - 1)->exp;
	}

	poly_exp_t max_deg = 0;
	poly_exp_t curr_deg = 0;

	for (uint i = 0; i < p->monos_count; i++) {
		curr_deg = PolyDegBy(&(GetNthMonoPtr(p->monos, i)->p), var_idx - 1);
		if (curr_deg > max_deg) {
			max_deg = curr_deg;
		}
	}

	return max_deg;
}

/**
 * Zwraca stopień wielomianu (-1 dla wielomianu tożsamościowo równego zeru).
 * @param[in] p : wielomian
 * @return stopień wielomianu @p p
 */
poly_exp_t PolyDeg(const Poly *p) {
	if (PolyIsZero(p)) {
		return -1;
	}
	if (PolyIsCoeff(p)) {
		return 0;
	}

	poly_exp_t max_deg = 0;
	poly_exp_t curr_deg = 0;
	Mono *m;

	for (uint i = 0; i < p->monos_count; i++) {
		m = GetNthMonoPtr(p->monos, i);
		curr_deg = PolyDeg(&(m->p)) + m->exp;
		if (curr_deg > max_deg) {
			max_deg = curr_deg;
		}
	}

	return max_deg;
}


bool PolyIsEq(const Poly *p, const Poly *q);

bool MonoIsEq(const Mono *m, const Mono *o) {
	if (m->exp != o->exp) {
		return false;
	}
	return PolyIsEq(&(m->p), &(o->p));
}

/**
 * Sprawdza równość dwóch wielomianów.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p = q`
 */
bool PolyIsEq(const Poly *p, const Poly *q) {
	if (p->scalar != q->scalar || p->monos_count != q->monos_count) {
		return false;
	}

	for (uint i = 0; i < p->monos_count; i++) {
		if (!MonoIsEq(GetNthMonoPtr(p->monos, i), GetNthMonoPtr(q->monos, i))) {
			return false;
		}
	}

	return true;
}

/**
 * Wylicza wartość wielomianu w punkcie @p x.
 * Wstawia pod pierwszą zmienną wielomianu wartość @p x.
 * W wyniku może powstać wielomian, jeśli współczynniki są wielomianem
 * i zmniejszane są indeksy zmiennych w takim wielomianie o jeden.
 * Formalnie dla wielomianu @f$p(x_0, x_1, x_2, \ldots)@f$ wynikiem jest
 * wielomian @f$p(x, x_0, x_1, \ldots)@f$.
 * @param[in] p
 * @param[in] x
 * @return @f$p(x, x_0, x_1, \ldots)@f$
 */
Poly PolyAt(const Poly *p, poly_coeff_t x) {
	if (PolyIsCoeff(p)) {
		return *p;
	}

	Poly r = PolyFromCoeff(p->scalar);
	Poly nr = r;
	Poly q;
	poly_coeff_t val;
	poly_exp_t exponent;

	for (uint i = 0; i < p->monos_count; i++) {
		exponent = p->monos[i].exp;
		val = pow(x, exponent);
		q = PolyScalarMul(&(p->monos[i].p), val);
		nr = PolyAdd(&r, &q);
	//	PolyDestroy(&q); WTF
	//	PolyDestroy(&r); WTF
		r = nr;
	}
	return nr;
}
