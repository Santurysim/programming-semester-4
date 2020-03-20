#include "forth.cpp"
#include "words.cpp"
#include "minunit.h"

MU_TEST(forth_tests_init_free) {
    Forth forth(stdin, 100, 100, 100);
    
    mu_check(forth.getMemory() == forth.getFreeMemory());
    mu_check(forth.getMemory() != NULL);
    mu_check(forth.getStackBottom() == forth.getStackPointer());
    mu_check(forth.getStackBottom() != NULL);
}

MU_TEST(forth_tests_align) {
    mu_check(align(8, 8) == 8);
    mu_check(align(9, 8) == 16);
    mu_check(align(7, 8) == 8);
}

MU_TEST(forth_tests_push_pop) {
    Forth forth(stdin, 100, 100, 100);
    forth.push(123);

    mu_check(forth.getStackPointer() > forth.getStackBottom());
    mu_check(forth.pop() == 123);
    mu_check(forth.getStackBottom() == forth.getStackPointer());
}

MU_TEST(forth_tests_emit) {
    Forth forth(stdin, 100, 100, 100);
    forth.emit(123);

    mu_check(forth.getFreeMemory() > forth.getMemory());
    mu_check(*forth.getMemory() == 123);
}

MU_TEST(forth_tests_codeword) {
    Forth forth(stdin, 100, 100, 100);

    mu_check(forth.getLatest() == NULL);

    Word *w1 = forth.addWord("TEST1", strlen("TEST1"), false);
    forth.emit(123);
    mu_check(forth.getLatest() == w1);

    Word *w2 = forth.addWord("TEST2", strlen("TEST2"), false);
    mu_check((*(cell*)w1->getCode()) == 123);
    mu_check((void*)w2 > w1->getCode());
    mu_check(forth.getLatest() == w2);

    mu_check(forth.getLatest()->find("TEST1", strlen("TEST1")) == w1);
    mu_check(forth.getLatest()->find("TEST2", strlen("TEST2")) == w2);
    mu_check(forth.getLatest()->find("TEST", strlen("TEST")) == NULL);
}

MU_TEST(forth_tests_compileword){
	Forth forth(stdin, 100, 100, 100);
	forth.addMachineWords();

	const Word *dup = forth.getLatest()->find("dup", strlen("dup"));
	const Word *mul = forth.getLatest()->find("*", strlen("*"));
	const Word *exit = forth.getLatest()->find("exit", strlen("exit"));
	const Word *square = forth.getLatest()->find("square", strlen("square"));

	mu_check(square);
	const Word *const*words = (const Word*const*)square->getConstCode();
	mu_check(words[0] == dup);
	mu_check(words[1] == mul);
	mu_check(words[2] == exit);
	Word *w1 = forth.addWord("TEST1", strlen("TEST1"), false);
	mu_check((const void*)w1 > (const void*)(words + 2));
}

MU_TEST(forth_tests_literal){
	Forth forth(stdin, 100, 100, 100);
	forth.addMachineWords();

	const Word *literal = forth.getLatest()->find("lit", strlen("lit"));
	const Word *exit = forth.getLatest()->find("exit", strlen("exit"));
	Word *test = forth.addWord("TEST", strlen("TEST"), true);
	forth.emit((cell)literal);
	forth.emit(4567);
	forth.emit((cell)exit);

	forth.runWord(test);
	cell c = forth.pop();
	mu_check(c = 4567);
}

MU_TEST_SUITE(forth_tests) {
    MU_RUN_TEST(forth_tests_init_free);
    MU_RUN_TEST(forth_tests_align);
    MU_RUN_TEST(forth_tests_push_pop);
    MU_RUN_TEST(forth_tests_emit);
    MU_RUN_TEST(forth_tests_codeword);
    MU_RUN_TEST(forth_tests_compileword);
    MU_RUN_TEST(forth_tests_literal);
}
