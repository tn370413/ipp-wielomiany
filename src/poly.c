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

/**
 * Usuwa wielomian z pamięci.
 * @param[in] p : wielomian
 */
void PolyDestroy(Poly *p) {
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
	bool end_of_list = false;
	Mono pm, pq;

	while (!end_of_list) {
		if (i == p->monos_count) {
			for (; j < q->monos_count; j++) {
				InsertNthMono(r.monos, r.monos_count, q->monos[j]);
			}
			end_of_list = true; // TODO BREAK
		} else if (j == q->monos_count) {
			for (; i < p->monos_count; i++) {
				InsertNthMono(r.monos, r.monos_count, p->monos[i]);
			}
			end_of_list = true;
		} else {

			pm = GetNthMono(p->monos, i);
			pq = GetNthMono(p->monos, j);

			if (pm.exp == pq.exp) {
				Poly m_coeff = PolyAdd(&(pm.p), &(pq.p));
				InsertNthMono(r.monos, r.monos_count, MonoFromPoly(&m_coeff, pm.exp));
				i++;
				j++;
			} else if (pm.exp > pq.exp) {
				InsertNthMono(r.monos, r.monos_count, pq);
				j++;
			} else /* pm.exp < pq.exp */ {
				InsertNthMono(r.monos, r.monos_count, pm);
				i++;
			}
		}

		r.monos_count++;
	}
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
	p.monos = monos;
	p.monos_count = count;
	return p;
}

/**
 * Mnoży dwa wielomiany.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p * q`
 */

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

Poly PolyMul(const Poly *p, const Poly *q) {
	Poly r1 = PolyScalarMul(p, q->scalar);
	Poly r2 = PolyScalarMul(q, p->scalar);

	Poly r = PolyAdd(r1, r2);
	r.scalar = p->scalar * q->scalar;

	Poly r3;
	r3.scalar = 0;
	r3.monos_count = 0;
	r3.monos = (Mono *) calloc(p->monos_count * q->monos_count, sizeof(Mono));

	for (uint i = 0; i < p->monos_count, i++) {
		for (uint j = 0; j < q->monos_count; j++) {

		}
	}

	PolyDestroy(r1);
	PolyDestroy(r2);
}

/**
 * Zwraca przeciwny wielomian.
 * @param[in] p : wielomian
 * @return `-p`
 */
Poly PolyNeg(const Poly *p);

/**
 * Odejmuje wielomian od wielomianu.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p - q`
 */
Poly PolySub(const Poly *p, const Poly *q);

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
poly_exp_t PolyDegBy(const Poly *p, unsigned var_idx);

/**
 * Zwraca stopień wielomianu (-1 dla wielomianu tożsamościowo równego zeru).
 * @param[in] p : wielomian
 * @return stopień wielomianu @p p
 */
poly_exp_t PolyDeg(const Poly *p);

/**
 * Sprawdza równość dwóch wielomianów.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p = q`
 */
bool PolyIsEq(const Poly *p, const Poly *q);

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
Poly PolyAt(const Poly *p, poly_coeff_t x);
