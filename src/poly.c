#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include "poly.h"

/**
 * Używana konwencja:
 * list – nazwa tablicy
 * n – indeks na tablicy wejściowej
 * p, q – wielomiany wejściowe
 * m, o – jednomiany wejściowe
 * r – wynik działania funkcji
 * i, j – indeksy pętli for przechodzących przez listy jednomianów w p i q odp.
 * k – indeks na tablicy jednomianów wielomianu wynikowego
 */


/**
 * Zapisuje jednomian w n-tym polu tablicy.
 * @param[in] list : tablica jednomianów
 * @param[in] n : indeks pola tablicy
 * @param[in] m : jednomian do wstawienia
 */
void InsertNthMono(Mono list[], int n, Mono m) {
	list[n] = m;
}

/**
 * Porównuje jednomiany na podstawie ich wykładników
 * @param[in] a : jednomian
 * @param[in] b : jednomian
 * @return znak róznicy wykładników a i b
 */
int MonoExpCompare(const Mono *m, const Mono *o) {
	if (m->exp == o->exp) {
		return 0;
	} else if (m->exp < o->exp) {
		return -1;
	} else {
		return 1;
	}
}

/* Funkcja pomocnicza, żeby kompilator nie krzyczał o złych typach wskaźników */
int MonoExpCompareForQsort(const void *m, const void *o) {
	return MonoExpCompare((Mono *) m, (Mono *) o);
}

/**
 * Sortuje w miejscu tablicę jednomianów
 * @param[in] list : tablica jednomianów
 * @param[in] count : liczba elementów tablicy jednomianów
 */
void SortMonosByExp(Mono *list, unsigned count) {
	qsort(list, count, sizeof(Mono), MonoExpCompareForQsort);
}

/**
 * Ta funkcja ma sprowadzać wielomian do "standardowej" postaci.
 * Funkcja modyfikuje wielomian w miejscu, ze względu na jej charakter
 * jako funkcji która sprowadza wynik działania innej funkcji do unormowanej
 * postaci przed returnem.
 * @param[in] p : wielomian
 */
void SimplifyPoly(Poly *p) {
	SortMonosByExp(p->monos, p->monos_count);

	unsigned k = 0;

	for (unsigned i = 1; i < p->monos_count; i++) {
		Mono mi = p->monos[i];
		Mono mp = p->monos[k];

		/* usuwanie jednomianów skalarnych */
		if (mi.exp == 0 && PolyIsCoeff(&(mi.p))) {
			p->scalar += mi.p.scalar;
			continue;
		}

		if (mi.exp == 0 && mi.p.scalar != 0) {
			p->scalar += mi.p.scalar;
			mi.p.scalar = 0;
		}

		/* po usunięciu skalara na to miejsce wchodzi następny jednomian */
		if (mp.exp == 0 && PolyIsCoeff(&(mp.p))) {
			InsertNthMono(p->monos, k, mi);
			/*
			 * nie zwiększamy indeksu k, w ten sposób przesunięte zostaną
			 * wszystkie następne jednomiany (nie powstanie luka)
			 */
			continue;
		}

		/* łączymy jednomiany o tym samym wykładniku */
		if (mi.exp == mp.exp) {
			Poly np = PolyAdd(&mi.p, &mp.p);
			PolyDestroy(&(mi.p));
			PolyDestroy(&(mp.p));
			Mono nm = MonoFromPoly(&np, mi.exp);
			InsertNthMono(p->monos, k, nm);
			/* znowu nie zwiększamy k → jw. */

		/* jeżeli wszystko było w porządku to zostawiamy w spokoju */
		} else {
			k++;
			InsertNthMono(p->monos, k, mi);
		}
	}
	p->monos_count = k + 1;

	k = 0;
	for (unsigned i = 0; i < p->monos_count; i++) {
		if (PolyIsZero(&(p->monos[i].p))) {
			continue;
		}
		Mono m = p->monos[i];
		p->monos[k] = m;
		k++;
	}

	p->monos_count = k;
}


/**
 * Usuwa wielomian z pamięci.
 * @param[in] p : wielomian
 */
void PolyDestroy(Poly *p) {
	if (PolyIsCoeff(p)) {
		return;
	}

	for (unsigned i = 0; i < p->monos_count; i++) {
		MonoDestroy(&(p->monos[i]));
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
	if (PolyIsCoeff(p)) {
	    return *p;
    }
	Poly r;
	r.scalar = p->scalar;
	r.monos_count = p->monos_count;
	r.monos = (Mono *) calloc(p->monos_count, sizeof(Mono));
	assert(r.monos != NULL);

	for (unsigned i = 0; i < p->monos_count; i++) {
		r.monos[i] = MonoClone(&(p->monos[i]));
	}

	return r;
}

/**
 * Dodaje dwa wielomiany.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p + q`
 */

Poly PolyAdd(const Poly *p, const Poly *q) {

	if (PolyIsZero(p)) {
		return PolyClone(q);
	}

	if (PolyIsZero(q)) {
		return PolyClone(p);
	}

	Poly r;
	r.scalar = p->scalar + q->scalar;
	r.monos_count = 0;
	r.monos = (Mono *) calloc(p->monos_count + q->monos_count, sizeof(Mono));
	assert(r.monos != NULL);

	unsigned i = 0;
	unsigned j = 0;
	Mono *pm;
	Mono *qm;

	/*
	 * Główna pętla zbierająca jednomiany do nowego wielomianu.
	 *
	 * Pętla będzie przechodzić oba wielomiany jednocześnie, zawsze dobierając
	 * z każdego po jednomianie o najniższym wykładniku. Ten z tych jednomianów
	 * który ma mniejszy wykładnik jest dołączany do sumy, i indeks na tablicy
	 * na której się znajdował zwiększamy o 1. W ten sposób tworzony wielomian
	 * będzie automatycznie posortowany.
	 */
	while (true) {
		/*
		 * Jeżeli wysycyliśmy którąś listę, to pozostałe jednomiany z drugiej
		 * po prostu kopiujemy na koniec i wychodzimy z pętli.
		 */
		if (i == p->monos_count) {
			for (; j < q->monos_count; j++) {
				InsertNthMono(r.monos, r.monos_count,
							  MonoClone(&(q->monos[j])));
				r.monos_count++;
			}
			break;

		} else if (j == q->monos_count) {
			for (; i < p->monos_count; i++) {
				InsertNthMono(r.monos, r.monos_count,
							  MonoClone(&(p->monos[i])));
				r.monos_count++;
			}
			break;

		} else {

			pm = &(p->monos[i]);
			qm = &(q->monos[j]);

			/*
			 * Jeżeli dwa jednomiany mają ten sam wykładnik, to nie mogą trafić
			 * osobno do sumy, ale ich współczynniki muszą być dodane do siebie
			 */
			if (pm->exp == qm->exp) {
				Poly m_coeff = PolyAdd(&(pm->p), &(qm->p));

				/*
				 * Czasem może się zdarzyć, że suma współczynników = 0.
				 * Wtedy powstaje jednomian zerowy który należy odrzucić
				 */
				if (!(PolyIsZero(&m_coeff))) {
				    InsertNthMono(r.monos, r.monos_count,
							  MonoFromPoly(&m_coeff, pm->exp));
				    r.monos_count++;
				}

				i++;
				j++;

			/* Gdy wykładniki są różne, bierzemy jednomian z mniejszym. */
			} else if (pm->exp > qm->exp) {
				InsertNthMono(r.monos, r.monos_count, MonoClone(qm));
				r.monos_count++;
				j++;

			} else /* pm->exp < pq->exp */ {
				InsertNthMono(r.monos, r.monos_count, MonoClone(pm));
				r.monos_count++;
				i++;
			}
		}
	}
	
	if (PolyIsCoeff(&r)) {
	    free(r.monos);
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
	/**
	 * Uwaga. Przyjęto założenie, że jednomiany mają różne wykładniki. Jeżeli
	 * będą dwa z takim samym, w większości przypadków operacje na wielomianach
	 * będą działać normalnie, ale taka konstrukcja nie będzie działać np.
	 * przy PolyIsEq. Założenie wynika z tego, ze w momencie pisania tego
	 * komentarza jest juz sobota 13 maja.
	 */ /* TODO */
	Poly r;
	r.scalar = 0;
	r.monos_count = 0;
	r.monos = calloc(count, sizeof(Mono));
	assert(r.monos != NULL);

	unsigned k = 0;
	Mono *mptr; // wskaźnik na aktualnie dodawany jednomian

	Mono nmonos[count]; // tablica robocza
	for (unsigned i = 0; i < count; i++) {
		nmonos[i] = monos[i];
	}
	SortMonosByExp(nmonos, count);


	for (unsigned i = 0; i < count; i++) {
		mptr = &(nmonos[i]);

		/* Jednomiany–skalary nie mają prawa istnieć samodzielnie */
		if (mptr->exp == 0 && mptr->p.scalar != 0) {
			r.scalar += mptr->p.scalar;
			mptr->p.scalar = 0;
		}

		if (PolyIsZero(&(mptr->p))) {
			continue;
		}

		if (PolyIsCoeff(&(mptr->p)) && mptr->exp == 0) {
			r.scalar += mptr->p.scalar;
		} else if (k > 0 && mptr->exp == r.monos[k - 1].exp) {
			Poly m_coeff = PolyAdd(&(mptr->p), &(r.monos[k - 1].p));
			PolyDestroy(&(r.monos[k - 1].p));
			r.monos[k - 1].p = m_coeff;
		} else {
			r.monos[k] = *mptr;
			r.monos_count++;
			k++;
		}
	}

    if (PolyIsCoeff(&r)) {
        free(r.monos);
        return r;
    }

	/* Lista wejściowa mogła być nieposortowana. Trzeba więc wynik posortować */
	// SortMonosByExp(r.monos, r.monos_count);
	SimplifyPoly(&r);
	return r;
}


Mono MonoScalarMul(const Mono *m, poly_coeff_t scalar);

/**
 * Mnoży wielomian ze skalarem.
 * @param[in] p : wielomian
 * @param[in] scalar : skalar
 * @return `p * scalar`
 */
Poly PolyScalarMul(const Poly *p, poly_coeff_t scalar) {
	if (PolyIsCoeff(p)) {
		return PolyFromCoeff(p->scalar * scalar);
	}

	Poly r;
	r.scalar = p->scalar * scalar;
	r.monos_count = p->monos_count;
	r.monos = (Mono *) calloc(r.monos_count, sizeof(Mono));
	assert(r.monos != NULL);

	for (unsigned i = 0; i < p->monos_count; i++) {
		InsertNthMono(r.monos, i,
					  MonoScalarMul(&(p->monos[i]), scalar));
	}

	return r;
}

/**
 * Mnoży jednomian przez skalar.
 * @param[in] m : jednomian
 * @param[in] scalar : skalar.
 * @return `m * scalar`
 */
Mono MonoScalarMul(const Mono *m, poly_coeff_t scalar) {
	Mono r;
	r.exp = m->exp;
	r.p = PolyScalarMul(&(m->p), scalar);
	return r;
}

Poly PolyMul(const Poly *p, const Poly *q);

/**
 * Mnoży jednomiany
 * @param[in] m : jednomian
 * @param[in] o : jednomian
 * @return `m * o`
 */
Mono MonoMul(const Mono *m, const Mono *o) {
	Mono r;
	r.exp = m->exp + o->exp;
	r.p = PolyMul(&(m->p), &(o->p));
	return r;
}

/**
 * Mnoży dwa wielomiany.
 * @param[in] p : wielomian
 * @param[in] q : wielomian
 * @return `p * q`
 */
Poly PolyMul(const Poly *p, const Poly *q) {
	/* przypadki trywialne */
	if (PolyIsZero(p) || PolyIsZero(q)) {
		return PolyZero();
	}
	if (PolyIsCoeff(p)) {
		return PolyScalarMul(q, p->scalar);
	}
	if (PolyIsCoeff(q)) {
		return PolyScalarMul(p, q->scalar);
	}

	/* mnożenie przez wyrazy wolne */
	Poly r1 = PolyScalarMul(p, q->scalar);
	Poly r2 = PolyScalarMul(q, p->scalar);
	Poly r12 = PolyAdd(&r1, &r2);
	PolyDestroy(&r1);
	PolyDestroy(&r2);
	r12.scalar = p->scalar * q->scalar;

	/* mnożenie jednomianów */
	Poly r3;
	r3.scalar = 0;
	r3.monos_count = p->monos_count * q->monos_count;
	r3.monos = (Mono *) calloc(r3.monos_count, sizeof(Mono));
	assert(r3.monos != NULL);

	for (unsigned i = 0; i < p->monos_count; i++) {
		for (unsigned j = 0; j < q->monos_count; j++) {
			Mono *mp = &(p->monos[i]);
			Mono *mq = &(q->monos[j]);
			InsertNthMono(r3.monos, j + q->monos_count * i, MonoMul(mp, mq));
		}
	}
	/* w mnożeniu nie zachowane były zasady tworzenia poprawnych wielomianów */
	SimplifyPoly(&r3);

	Poly r = PolyAdd(&r12, &r3);
	PolyDestroy(&r12);
	PolyDestroy(&r3);

	SimplifyPoly(&r);

	return r;
}

/**
 * Zwraca przeciwny jednomian.
 * @param[in] p : jednomian
 * @return `-p`
 */
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
		return PolyFromCoeff(-(p->scalar));
	}

	Poly r;
	r.scalar = -(p->scalar);
	r.monos_count = p->monos_count;
	r.monos = (Mono *) calloc(r.monos_count, sizeof(Mono));
	assert(r.monos != NULL);

	for (unsigned i = 0; i < p->monos_count; i++) {
		InsertNthMono(r.monos, i, MonoNeg(&(p->monos[i])));
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
	/* przypadki trywialne */
	if (PolyIsZero(p)) {
		return -1;
	}
	if (PolyIsCoeff(p)) {
		return 0;
	}

	/* gdy zmienną jest x0, zwracamy po prostu najwyższy wykładnik */
	if (var_idx == 0) {
		return p->monos[p->monos_count - 1].exp;
	}

	/* dla zmiennych wyższych, trzeba wejsc do każdego jednomianu z osobna */

	poly_exp_t max_deg = 0;
	poly_exp_t curr_deg = 0;

	for (unsigned i = 0; i < p->monos_count; i++) {
		/* z perspektywy współczynnika szukamy zminnej o 1 mniejszym indeksie */
		curr_deg = PolyDegBy(&(p->monos[i].p), var_idx - 1);
		/**
		 * nie wiemy w którym jednomianie znajdziemy najwyższą potęgę,
		 * więc musimy szukać maximum "tradycyjnie"
		 */
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
	/* przypadki trywialne */
	if (PolyIsZero(p)) {
		return -1;
	}
	if (PolyIsCoeff(p)) {
		return 0;
	}

	/* tak samo jak w PolyDegBy, tyle że sumujemy potęgi po drodze */
	poly_exp_t max_deg = 0;
	poly_exp_t curr_deg = 0;
	Mono *m;

	for (unsigned i = 0; i < p->monos_count; i++) {
		m = &(p->monos[i]);
		curr_deg = PolyDeg(&(m->p)) + m->exp;
		if (curr_deg > max_deg) {
			max_deg = curr_deg;
		}
	}

	return max_deg;
}


bool PolyIsEq(const Poly *p, const Poly *q);

/**
 * Sprawdza równość dwóch jednomianów.
 * @param[in] m : jednomian
 * @param[in] o : jednomian
 * @return `m = o`
 */
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

	for (unsigned i = 0; i < p->monos_count; i++) {
		if (!MonoIsEq(&(p->monos[i]), &(q->monos[i]))) {
			return false;
		}
	}

	return true;
}

/** Podnosi l. całk. do potęgi (zapobiega overflow) 
 * @param[in] x : baza
 * @param[in] e : wykładnik
 * @return @f$x^e @f$
 */
poly_coeff_t Pow(poly_coeff_t x, poly_exp_t e) {
	poly_coeff_t r = 1;
	while (e) {
		if (e & 1) {
			r *= x;
		}
		e >>= 1;
		x *= x;
	}
	return r;
}

/** Podnosi wielomian do potęgi
 * @param[in] p : wielomian
 * @param[in] e : wykładnik
 * @return @f$x^e @f$
 */
 Poly PolyPow(const Poly *p, poly_exp_t e) {
 	Poly r = PolyFromCoeff(1);
 	Poly q = PolyClone(p);
 	Poly nr;
 	Poly nq;
 	while (e) {
 		if (e & 1) {
 			nr = PolyMul(&r, &q);
 			PolyDestroy(&r);
 			r = nr;
		}
		e >>= 1;
		nq = PolyMul(&q, &q);
		PolyDestroy(&q);
		q = nq;
	}
	PolyDestroy(&q);
	return r;
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
	/* przypadek trywialny */
	if (PolyIsCoeff(p)) {
		return *p;
	}

	/*
	 * będziemy szli po kolei po jednomianach i wyciągali ich wartość–wielomian
	 * w danym punkcie x, i sumowali wynik
	 */
	Poly r = PolyFromCoeff(p->scalar);
	Poly nr = r; // wielomian pomocniczy z wynikiem
	Poly q; // wielomian dodawany do sumy wynikowej
	poly_coeff_t val; // x^exponent

	for (unsigned i = 0; i < p->monos_count; i++) {
		val = Pow(x, p->monos[i].exp);
		q = PolyScalarMul(&(p->monos[i].p), val);
		nr = PolyAdd(&r, &q);
		PolyDestroy(&q);
		PolyDestroy(&r);
		r = nr;
	}

	return nr;
}

/**
 * Zwraca wielomian utworzony przez zamianę w jednomianie @m TODO
 * @param[in] m : jednomian
 * @param[in] count : długość tablicy x
 * @param[in] x : tablica wielomianów
 * @return wielomian
 */ 
Poly MonoCompose(const Mono *m, unsigned count, const Poly x[]) {
	Poly p = PolyPow(x, m->exp);
	Poly q;
	if (count == 0) {
		q = m->p;
	} else {
		q = PolyCompose(&(m->p), count - 1, &(x[1]));
	}
	Poly r = PolyMul(&p, &q);
	PolyDestroy(&p);
	PolyDestroy(&q);
	return r;
}

/**
 * Zwraca wielomian @p w którym pod i-tą zmienną podstawia wielomian x[i]
 * @param[in] p : wielomian "główny"
 * @param[in] count : długość tablicy x
 * @param[in] x : tablica wielomianów
 * @return p(x[0], x[1], ..., x[count - 1], 0, 0, 0, ...)
 */
Poly PolyCompose(const Poly *p, unsigned count, const Poly x[]) {
	Poly r = PolyFromCoeff(p->scalar);
	Poly tmp;
	Poly nr;
	for (unsigned i = 0; i < p->monos_count; i++) {
		tmp = MonoCompose(&(p->monos[i]), count, x);
		nr = PolyAdd(&tmp, &r);
		PolyDestroy(&tmp);
		PolyDestroy(&r);
		r = nr;
	}
	return r;
}
