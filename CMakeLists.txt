cmake_minimum_required(VERSION 3.0)
project(wielomiany C)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

# Nie chcemy oglądać komunikatów o procentowym postępie kompilacji.
set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)

# Chcemy zobaczyć polecenia wywołane przez make.
set(CMAKE_VERBOSE_MAKEFILE ON)

# Ustawiamy wspólne opcje kompilowania dla wszystkich wariantów projektu.
set(CMAKE_C_FLAGS "-std=c11 -lm -Wall -Wextra")
# Domyślne opcje dla wariantów Release i Debug są sensowne.
# Jeśli to konieczne, ustawiamy tu inne.
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-g")

find_library(CMOCKA cmocka)

if (NOT CMOCKA)
	message(FATAL_ERROR "Could not find cmocka.")
endif ()

# Wskazujemy pliki źródłowe.
set(SOURCE_FILES
    src/poly.c
    src/poly.h
	src/stack.h
	src/main.c
)

enable_testing()

# Wskazujemy plik wykonywalny.
add_executable(calc_poly ${SOURCE_FILES})

add_executable(unit_tests_poly ${SOURCE_FILES} src/unit_tests_poly_utils.h src/unit_tests_poly.c)

set_target_properties(
	unit_tests_poly
	PROPERTIES
	COMPILE_DEFINITIONS UNIT_TESTING=1)

target_link_libraries(unit_tests_poly ${CMOCKA})

add_test(unit_tests_poly ${CMAKE_CURRENT_BINARY_DIR}/unit_tests_poly)

# Dodajemy obsługę Doxygena: sprawdzamy, czy jest zainstalowany i jeśli tak to:
find_package(Doxygen)
if (DOXYGEN_FOUND)
    # Wskazujemy lokalizacją pliku konfiguracyjnego i podajemy jego docelową lokalizację w folderze, gdzie następuje kompilacja.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    # Dodajemy cel doc: użycie make doc spowoduje wywołanie doxygena z odpowiednim plikiem konfiguracyjnym w folderze kompilacji.
    # Na wyjście zostanie wypisany odpowiedni komentarz.
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )
endif (DOXYGEN_FOUND)
