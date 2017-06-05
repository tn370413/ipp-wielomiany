/** @file
   Kalkulator działający na stosie wielomianów rzadkich wielu zmiennych.

   @author Tomasz Necio <Tomasz.Necio@fuw.edu.pl>
   @copyright Uniwersytet Warszawski
   @date 2017-05-27
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "poly.h"
#include "stack.h"

#define INITIAL_MONOS_SIZE 256 ///< początkowy rozmiar tablicy monosów dla PolyAddMonos w PolyParse
#define MAX_COMMAND_LENGTH 25 ///< maksymalna długość komendy ("AT -LONG_MAX\n")

/** Flagi oznaczające typy liczbowe dla funkcji parsujących liczby */
enum int_type_e {
    POLY_EXP_T,
    POLY_COEFF_T,
    UNSIGNED
};

/** Flagi błędów */
enum error_flag_e {
	NO_ERROR,
    EOF_FLAG,
    UNDERFLOW_ERR_FLAG,
    PARSING_ERR_FLAG,
    WRONG_COMMAND_ERR_FLAG,
    WRONG_VALUE_ERR_FLAG,
    WRONG_VARIABLE_ERR_FLAG,
    EXCEEDED_COMMAND_BUF_ERR,
    WRONG_COUNT_ERR_FLAG
};

/* * * PARSER STATE AND ERROR HANDLING * * */

/* These variables will hold the state of the whole parser */
unsigned row = 0; ///< numer wiersza na którym jest parser (licząc od 0)
unsigned column = 0; ///< numer kolumny na której jest parser
enum error_flag_e error_flag = NO_ERROR; ///< flaga ostatnio wykrytego i nieobsłużoneo błędu
bool end_of_line = false; ///< flaga wskazująca osiągnięcie końca wiersza

/* Dummy structs for quick escape from functions on error */
Poly DUMMY_POLY; ///< stała zwracana przez funkcje typu Poly w trakcie ucieczki z błędu
Mono DUMMY_MONO; ///< stała zwracana przez funkcje typu Mono w trakcie ucieczki z błędu

/**
 * Ustawia flagę błędu.
 * Ustawienie flagi błędu powinno wywołać jak najszybszą ucieczkę z kolejnych
 * kontekstów aż do osiągnięcia main(), gdzie błąd będzie obsłużony przez
 * ErrorHandle()
 * @param[in] flag : flaga błędu
 */
void ErrorSetFlag(enum error_flag_e flag) {
	error_flag = flag;
}

/**
 * Informuje o nieobsłużonych błędach.
 * Funkcje po wywołaniu innej funkcji sprawdzają Error() i w razie wystąpienia
 * błędu jak najszybciej 'propagują go' wyżej
 * @return obecnie ustawiona flaga błędu (0, jeżeli błąd jeszcze nie wystąpił)
 */
int Error() {
	return error_flag;
}

/**
 * Obsługuje błąd – akcja zależy od ustawionej flagi błędu.
 * W kontekscie zadania jedynie drukuje informację na stderr i resetuje flagę.
 */
void ErrorHandle() {
	switch (error_flag) {
	case UNDERFLOW_ERR_FLAG:
		fprintf(stderr, "ERROR %d STACK UNDERFLOW\n", row + 1);
		break;
	case PARSING_ERR_FLAG:
		fprintf(stderr, "ERROR %d %d\n", row + 1, column);
		break;
	case WRONG_COMMAND_ERR_FLAG:
		fprintf(stderr, "ERROR %d WRONG COMMAND\n", row + 1);
		break;
	case WRONG_VALUE_ERR_FLAG:
		fprintf(stderr, "ERROR %d WRONG VALUE\n", row + 1);
		break;
	case WRONG_VARIABLE_ERR_FLAG:
		fprintf(stderr, "ERROR %d WRONG VARIABLE\n", row + 1);
		break;
	case WRONG_COUNT_ERR_FLAG:
		fprintf(stderr, "ERROR %d WRONG COUNT\n", row + 1);
		break;		
	default:
		break;
	}
	error_flag = 0;
}

/* * * OUTPUT FUNCTIONS * * */

/**
 * Drukuje wartość logiczną ze znakiem nowej linii.
 * @param[in] val : wartość do wydrukowania
 */
void BoolPrint(bool val) {
	if (val) {
		printf("1\n");
	} else {
		printf("0\n");
	}
}

/**
 * Drukuje wielomiam.
 * @param[in] p : wielomian do wydrukowania
 */
void PolyPrint(Poly *p);

/**
 * Drukuje jednomian.
 * @param[in] m : jednomian do wydrukowania
 */
void MonoPrint(Mono *m) {
	printf("(");
	PolyPrint(&(m->p));
	printf(",%d)", m->exp);
}

/**
 * Przekształca wielomian do postaci pozwalającej na wydrukowanie wg wymagań
 * @param[in] p : wielomian do przygotowania
 * @param[in] memory_flag : informuje funkcję wywołującą czy użyto alokacji pamięci
 * @return wielomian gotowy do druku
 */
Poly PolyPrepareForPrint(Poly *p, bool *memory_flag) {
	/* Wielomian w naszej implementacji mógłby chcieć się wydrukwać jako np.
	 * "7+(((1,1),0)+(2,2),0)+(2,3)"
	 * a powinien
	 * "(((7,0)+(1,1),0)+(2,2),0)+(2,3)"
	 * Łatwiej przekształcić wielomian w tej egzotycznej sytuacji, niż wykrywać
	 * to w funkcji drukującej.
	 */
	if (p->scalar != 0 && p->monos_count > 1 && p->monos[0].exp == 0) {
		Poly r = PolyClone(p);
		r.scalar = 0;
		r.monos[0].p.scalar += p->scalar;
		*memory_flag = true;
		return r;
	}
	return *p;
}

void PolyPrint(Poly *p) {
	if (PolyIsCoeff(p)) {
		printf("%ld", p->scalar);
	} else {
		bool was_memory_allocated = false;
		Poly q = PolyPrepareForPrint(p, &was_memory_allocated);

		/* jeżeli po preparacji został jakiś skalar, to znaczy ze nie było
		 * jednomianu z exp = 0 gdzie trzeba by go było wsadzić */
		if (q.scalar != 0) {
			printf("(%ld,0)", q.scalar);
		}
		for (unsigned i = 0; i < q.monos_count; i++) {
			if (i > 0 || q.scalar != 0) {
				printf("+");
			}
			MonoPrint(&(q.monos[i]));
		}

		if (was_memory_allocated) {
			PolyDestroy(&q);
		}
	}
}


/* * * INPUT FUNCTIONS * * */

/**
 * Podgląda następny znak z stdin bez usuwania go ze strumienia.
 * @return znak na stdin
 */
int SeeChar() {
	if (feof(stdin)) {
		ErrorSetFlag(PARSING_ERR_FLAG);
		return EOF;
	}
	int r = getchar();
	ungetc(r, stdin);
	return r;
}

/**
 * Jak getchar(), plus aktualizuje stan parsera.
 * @return getchar()
 */
int GetChar() {
	int ch;
	if ((ch = getchar()) == '\n') {
		end_of_line = true;
	} else {
		end_of_line = false;
	}
	column++;
	return ch;
}

/**
 * Sprawdza czy znak jest literą
 * @param[in] ch : znak
 * @return Czy znak jest literą?
 */
bool IsLetter(int ch) {
	return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}

/**
 * Sprawdza czy znak jest cyfrą
 * @param[in] ch : znak
 * @param[in] exclude_minus : informuje czy dopuszczać znak minus?
 * @return Czy znak jest cyfrą?
 */
bool IsDigit(int ch, bool exclude_minus) {
	return ('0' <= ch && ch <= '9') || (!exclude_minus && (ch == '-'));
}

/**
 * Przewija stdin do następnej linii.
 * @param[in] eof_flag : informuje fukcję wywołującą, czy napotkano End Of File
 */
void GoToNextLine(bool *eof_flag) {
	/* check if we haven't already reached the end of the line */
	if (end_of_line) { return; }

	int ch = SeeChar();

	/* W momencie wywołania tej funkcji, nie powinno być już znaków na stdin */
	if (!Error() && ch != '\n' && ch != EOF) {
		column++;
		ErrorSetFlag(PARSING_ERR_FLAG);
	}

	while (Error() && ch != '\n' && ch != EOF) {
		getchar();
		ch = SeeChar();
		if (ch == EOF) {
			*eof_flag = EOF_FLAG;
			return;
		}
	}

	ch = getchar(); // must be '\n' or End Of File
	if (ch == EOF) {
		*eof_flag = true;
	}
}

/**
 * Wczytuje wielomian z stdin.
 * @return wczytany wielomian
 */
Poly PolyParse();

/**
 * Sprawdza, czy do liczby r można dokleić cyfrę bez przekraczania limitu
 * @param[in] r : liczba
 * @param[in] digit : doklejana cyfra
 * @param[in] max_val : limit
 */
void ValidateNextDigit(long r, int digit, bool minus, long max_val) {
	if (r > max_val / 10 ||
			(r == max_val / 10 && digit > (max_val % 10) + minus)) {
		ErrorSetFlag(PARSING_ERR_FLAG);
	}
}

/**
 * Wczytuje liczbę z tablicy znaków lub stdin, jeżeli nie podano tablicy
 * @param[in] c : tablica znaków
 * @param[in] type : flaga typu liczby, słuzy do obsługi niepoprawnych wartości
 * @return wczytana liczba
 */
long NumberRead(char *c, enum int_type_e type) {
	if ((c && !IsDigit(c[0], false)) ||
	   (!c && !IsDigit(SeeChar(), false))) {
		if (!c) { GetChar(); }
		ErrorSetFlag(PARSING_ERR_FLAG);
		return 0;
	}

	/* obsługa minusa */
	bool minus = false;
	if ((!c && SeeChar() == '-') ||
		(c && c[0] == '-')) {
		if (type == POLY_EXP_T || type == UNSIGNED) {
			ErrorSetFlag(PARSING_ERR_FLAG);
			return 0;
		}

		minus = true;
		if (c) {
			c++;
		} else {
			GetChar(); // '-'
		}
	}

	int ch;
	if (c) {
		ch = c[0];
	} else {
		ch = SeeChar();
	}
	int digit;
	long r = 0;
	while (IsDigit(ch, true)) {
		if (c) {
			c++;
		} else {
			ch = GetChar();
		}
		digit = ch - 48; // '0' = 48

		switch (type) {
		case POLY_EXP_T:
			ValidateNextDigit(r, digit, minus, POLY_EXP_MAX);
			break;
		case POLY_COEFF_T:
			ValidateNextDigit(r, digit, minus,  POLY_COEFF_MAX);
			break;
		case UNSIGNED:
			ValidateNextDigit(r, digit, minus, UINT32_MAX);
			break;
		default:
			assert(false);
			break;
		}
		if (Error()) { return 0; }

		r = 10 * r + digit;
		if (c) {
			ch = c[0];
		} else {
			ch = SeeChar();
		}
	}

	if (c && c[0] != '\0') {
		ErrorSetFlag(PARSING_ERR_FLAG);
		return 0;
	}

	if (minus) {
		return -r;
	}
	return r;
}

/**
 * Wczytuje liczbę z stdin
 * @param[in] type : flaga typu liczby, używana do obsługi błędnych wartości
 * @return wczytana liczba
 */
long NumberParse(int type) {
	return NumberRead(NULL, type);
}

/**
 * Wczytuje jednomian ze stdin.
 * @return wczytany jednomian
 */
Mono MonoParse() {
	if (GetChar() != '(') {
		ErrorSetFlag(PARSING_ERR_FLAG);
		return DUMMY_MONO;
	}

	Poly p = PolyParse();
	if (Error()) { return DUMMY_MONO; }

	if (GetChar() != ',') {
		ErrorSetFlag(PARSING_ERR_FLAG);
		return DUMMY_MONO;
	}

	int ch = SeeChar();
	if (!IsDigit(ch, false)) {
		GetChar();
		ErrorSetFlag(PARSING_ERR_FLAG);
		return DUMMY_MONO;
	}

	poly_exp_t e = NumberParse(POLY_EXP_T);
	if (Error()) { return DUMMY_MONO; }

	if (GetChar() != ')') {
		ErrorSetFlag(PARSING_ERR_FLAG);
		return DUMMY_MONO;
	}

	return MonoFromPoly(&p, e);
}

Poly PolyParse() {
	if (SeeChar() == '(') { /* array of monos */
		/* we create a dynamic array of monos and will be growing it when
		 * there's a need for more space */
		size_t monos_size = INITIAL_MONOS_SIZE;
		Mono *monos = calloc(monos_size, sizeof(Mono));
		assert(monos != NULL);
		unsigned monos_count = 0;

		int c;
		while (true) {
			if (monos_count == monos_size) {
				monos_size *= 2;
				monos = realloc(monos, monos_size * sizeof(Mono));
				assert(monos != NULL);
			}

			monos[monos_count] = MonoParse();
			if (Error()) {
				free(monos);
				return DUMMY_POLY;
			}

			monos_count++;
			if ((c = SeeChar()) != '+') { // probably ',' or '\n' or 'EOF'
				break;
			} else {
				GetChar(); // '+'
			}
		}

		Poly p = PolyAddMonos(monos_count, monos);
		free(monos);
		return p;

	} else /* a scalar */ {
		poly_coeff_t r = NumberParse(POLY_COEFF_T);
		if (Error()) { return DUMMY_POLY; }

		return PolyFromCoeff(r);
	}
}


/* * * POLY OPERATIONS * * */

/**
 * jak GetTop(), ale sprawdza, czy na stosie jest wielomian
 * @param[in] s : stos
 * @return GetTop(s)
 */
Poly GetTopSafely(Stack *s) {
	if (IsEmpty(s)) {
		ErrorSetFlag(UNDERFLOW_ERR_FLAG);
		return DUMMY_POLY;
	}
	return GetTop(s);
}

/**
 * Jak Pop(), ale sprawdza czy na stosie jest wielomian do zrzucenia
 * @param[in] s : stos
 * @return Pop(s)
 */
Poly PopSafely(Stack *s) {
	if (IsEmpty(s)) {
		ErrorSetFlag(UNDERFLOW_ERR_FLAG);
		return DUMMY_POLY;
	}
	return Pop(s);
}

/**
 * Działa operacją na dwóch wielomianach na wierzchu stosu i wynik wstawia na
 * ich miejsce.
 * @param[in] s : stos
 * @param[in] op : działanie wielomian x wielomian → wielomian
 */
void ActOnTwoPolysOnStack(Stack *s,
						  Poly (*op)(const Poly *, const Poly *)) {
	Poly p = PopSafely(s);
	if (Error()) { return; }

	Poly q = PopSafely(s);
	if (Error()) {
		/* zwracamy p, żeby stos był w takim stanie jak przed errorem */
		Push(s, &p);
		return;
	}

	Poly r = op(&p, &q);
	PolyDestroy(&p);
	PolyDestroy(&q);

	Push(s, &r);
}

/**
 * Zastępuje dwa wielomiany na wierzchu stosu ich sumą
 * @param[in] s : stos
 */
void AddTwoPolysFromStack(Stack *s) {
	return ActOnTwoPolysOnStack(s, PolyAdd);
}

/**
 * Zastępuje dwa wielomiany na wierzchu stosu ich iloczynem
 * @param[in] s : stos
 */
void MultiplyTwoPolysFromStack(Stack *s) {
	return ActOnTwoPolysOnStack(s, PolyMul);
}

/**
 * Zastępuje dwa wielomiany na wierzchu stosu ich różnicą
 * @param[in] s : stos
 */
void SubtractTwoPolysFromStack(Stack *s) {
	return ActOnTwoPolysOnStack(s, PolySub);
}

/**
 * Drukuje, czy dwa wielomiany na wierzchu stosu są równe
 * @param[in] s : stos
 */
void PrintAreEqualTwoPolysFromStack(Stack *s) {
	Poly p = PopSafely(s);
	if (Error()) { return; }

	Poly q = GetTopSafely(s);
	if (Error()) {
		Push(s, &p);
		return;
	}

	bool r = PolyIsEq(&p, &q);
	Push(s, &p);
	BoolPrint(r);
}

/**
 * Drukuje czy wielomian na wierzchu stosu jest skalarem
 * @param[in] s : stos
 */
void IsCoeffPolyOnStack(Stack *s) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	BoolPrint(PolyIsCoeff(&top));
}

/**
 * Drukuje czy wielomian na wierzchu stosu jest zerem
 * @param[in] s : stos
 */
void IsZeroPolyOnStack(Stack *s) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	BoolPrint(PolyIsZero(&top));
}

/**
 * Wrzuca na stos kopię wielomianu na wierzchu tego stosu
 * @param[in] s : stos
 */
void ClonePolyOnStack(Stack *s) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	Poly p = PolyClone(&top);
	Push(s, &p);
}

/**
 * Zastępuje wielomian na wierzchu stosu jego przeciwieństwem
 * @param[in] s : stos
 */
void NegatePolyOnStack(Stack *s) {
	Poly p = PopSafely(s);
	if (Error()) { return; }

	Poly q = PolyNeg(&p);
	PolyDestroy(&p);
	Push(s, &q);
}

/**
 * Drukuje stopień wielomianu znajdujacego się na wierzchu stosu
 * @param[in] s : stos
 */
void PrintDegreePolyOnStack(Stack *s) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	printf("%d\n", PolyDeg(&top));
}

/**
 * Drukuje wielomian znajdujący się na wierzchu stosu.
 * @param[in] s : stos
 */
void PrintPolyOnStack(Stack *s) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	PolyPrint(&top);
	printf("\n");
}

/** Polecenie to zdejmuje z wierzchołka stosu najpierw wielomian p, a potem 
 * kolejno wielomiany x[0], x[1], …, x[count - 1] i umieszcza na stosie wynik
 * funkcji PolyCompose
 * @param[in] s : stos
 * @param[in] count : liczba wielomianów do zmielenia
*/
void ComposePolysOnStack(Stack *s, unsigned count) {
	if (!HasElements(s, count + 1)) {
		ErrorSetFlag(UNDERFLOW_ERR_FLAG);
		return;
	}
	
	Poly p = Pop(s);
	Poly x[count];
	for (unsigned i = 0; i < count; i++) {
		x[i] = Pop(s);
	}
	Push(s, PolyCompose(&p, count, x));
	
	PolyDestroy(&p);
	for (unsigned i = 0; i < count; i++) {
		PolyDestroy(&(x[i]));
	}
}

/**
 * Drukuje stopień wielomianu na wierzchu stosu ze względu na n-tą zmienną
 * @param[in] s : stos
 * @param[in] n : n
 */
void PrintDegBy(Stack *s, unsigned n) {
	Poly top = GetTopSafely(s);
	if (Error()) { return; }

	int r = PolyDegBy(&top, n);
	printf("%d\n", r);
}

/**
 * Zastępuje wielomian na wierzchu stosu jego "wartością w punkcie x"
 * (obcięciem do zbioru `{(x_1, x_2, ...) : x_1 = x}`)
 * @param[in] s : stos
 * @param[in] x : x
 */
void CalculatePolyAt(Stack *s, poly_coeff_t x) {


	Poly top = PopSafely(s);
	if (Error()) { return; }

	Poly p = PolyAt(&top, x);
	PolyDestroy(&top);
	Push(s, &p);
}

/* * * THE PROGRAM * * */

/**
 * Wykonuje komendę zawartą w tablicy znaków.
 * @param[in] s : stos wielomianów na którym operujemy
 * @param[in] command : tablica znaków zawierająca komendę do wykonania
 */
void ExecuteCommand(Stack *s, char *command) {
	if (strcmp(command, "ZERO") == 0) {
		Poly p = PolyZero();
		Push(s, &p);

	} else if (strcmp(command, "IS_COEFF") == 0) {
		IsCoeffPolyOnStack(s);

	} else if (strcmp(command, "IS_ZERO") == 0) {
		IsZeroPolyOnStack(s);

	} else if (strcmp(command, "CLONE") == 0) {
		ClonePolyOnStack(s);

	} else if (strcmp(command, "ADD") == 0) {
		AddTwoPolysFromStack(s);

	} else if (strcmp(command, "MUL") == 0) {
		MultiplyTwoPolysFromStack(s);

	} else if (strcmp(command, "NEG") == 0) {
		NegatePolyOnStack(s);

	} else if (strcmp(command, "SUB") == 0) {
		SubtractTwoPolysFromStack(s);

	} else if (strcmp(command, "IS_EQ") == 0) {
		PrintAreEqualTwoPolysFromStack(s);

	} else if (strcmp(command, "DEG") == 0) {
		PrintDegreePolyOnStack(s);

	} else if (strcmp(command, "PRINT") == 0) {
		PrintPolyOnStack(s);

	} else if (strcmp(command, "POP") == 0) {
		Poly p = PopSafely(s);
		if (Error()) { return; }
		PolyDestroy(&p);
		
	
	} else if (strncmp(command, "COMPOSE ", 8) == 0) {
		if (Error() == EXCEEDED_COMMAND_BUF_ERR) {
			ErrorSetFlag(WRONG_VARIABLE_ERR_FLAG);
			return;
		}

		unsigned arg = NumberRead(command + 8, UNSIGNED);
		if (Error()) {
			ErrorSetFlag(WRONG_VARIABLE_ERR_FLAG);
			return;
		}
		ComposePolysOnStack(s, arg);

	} else if (strncmp(command, "DEG_BY ", 7) == 0) {
		if (Error() == EXCEEDED_COMMAND_BUF_ERR) {
			ErrorSetFlag(WRONG_VARIABLE_ERR_FLAG);
			return;
		}

		unsigned arg = NumberRead(command + 7, UNSIGNED);
		if (Error()) {
			ErrorSetFlag(WRONG_COUNT_ERR_FLAG);
			return;
		}
		PrintDegBy(s, arg);

	} else if (strncmp(command, "AT ", 3) == 0) {
		if (Error() == EXCEEDED_COMMAND_BUF_ERR) {
			ErrorSetFlag(WRONG_VALUE_ERR_FLAG);
			return;
		}

		poly_coeff_t arg = NumberRead(command + 3, POLY_COEFF_T);
		if (Error()) {
			ErrorSetFlag(WRONG_VALUE_ERR_FLAG);
			return;
		}
		CalculatePolyAt(s, arg);
		
	} else {
		ErrorSetFlag(WRONG_COMMAND_ERR_FLAG);
	}
}

/**
 * Główna funkcja programu.
 * @return flaga ostatniego nieobsłużonego błędu
 */
int main() {
	Stack s = StackEmpty();

	char command_buf[MAX_COMMAND_LENGTH] = {'\0'};
	size_t new_line_pos;
	int ch;
	bool eof_flag = false;

	while (!eof_flag) {
		end_of_line = false;
		column = 0;

		ch = SeeChar();
		if (ch == EOF) {
			break;
		} else if (IsLetter(ch)) { /* komenda */
			/* komendy możemy trzymać w buforze o ograniczonej pojemności */
			fgets(command_buf, sizeof command_buf, stdin);

			new_line_pos = strcspn(command_buf, "\r\n");
			if (!(new_line_pos == MAX_COMMAND_LENGTH - 1)) {
				/* Gdzieś w buforze jest '\n'. Po przeczytaniu go więc
				   skończy się już linia i musimy to pamiętać.
				*/
				end_of_line = true;
				command_buf[new_line_pos] = 0;
			} else {
				/* Ktoś wprowadził zbyt długą komendę lub argument.
				   Nie możemy jeszcze obsłużyć błędu, bo zależnie od komendy
				   będziemy podawać różne komunikaty. */
				ErrorSetFlag(EXCEEDED_COMMAND_BUF_ERR);
			}

			ExecuteCommand(&s, command_buf);

			if (Error()) {
				ErrorHandle();
				GoToNextLine(&eof_flag);
			}

		} else { /* wielomian */
			Poly p = PolyParse();
			GoToNextLine(&eof_flag);
			if (!(Error())) {
				Push(&s, &p);
			} else {
				ErrorHandle();
			}
		}
		row++;
	}

	StackDestroy(&s);
	return error_flag;
}
