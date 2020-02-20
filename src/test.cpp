#include "minunit.h"
#include "forth.test.cpp"

int main(void) {
	MU_RUN_SUITE(forth_tests);
	MU_REPORT();
	return MU_EXIT_CODE;
}
