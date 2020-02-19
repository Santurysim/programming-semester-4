#include "forth.c"
#include "words.c"
#include "minunit.h"

MU_TEST(forth_tests_init_free) {
    Forth forth(stdin, 100, 100);
    
    mu_check(forth.getMemory() == forth.getFreeMemory());
    mu_check(forth.getMemory() != NULL);
    mu_check(forth.getSp0() == forth.getStackPointer());
    mu_check(forth.getSp0() != NULL);
}

MU_TEST(forth_tests_align) {
    mu_check(align(8, 8) == 8);
    mu_check(align(9, 8) == 16);
    mu_check(align(7, 8) == 8);
}

MU_TEST(forth_tests_push_pop) {
    Forth forth(stdin, 100, 100);
    forth.push(123);

    mu_check(forth.getStackPointer() > forth.getSp0());
    mu_check(forth.pop() == 123);
    mu_check(forth.getSp0() == forth.getStackPointer());
}

MU_TEST(forth_tests_emit) {
    Forth forth(stdin, 100, 100);
    forth.emit(123);

    mu_check(forth.getFreeMemory() > forth.getMemory());
    mu_check(*forth.getMemory() == 123);
}

MU_TEST(forth_tests_codeword) {
    Forth forth(stdin, 100, 100);

    mu_check(forth.getLatest() == NULL);

    struct word *w1 = forth.addWord("TEST1", strlen("TEST1"));
    forth.emit(123);
    mu_check(forth.getLatest() == w1);

    struct word *w2 = forth.addWord("TEST2", strlen("TEST2"));
    mu_check((*(cell*)w1.getCode()) == 123);
    mu_check((void*)w2 > w1.getCode());
    mu_check(forth.getLatest() == w2);

    mu_check(forth.getLatest().find("TEST1", strlen("TEST1")) == w1);
    mu_check(forth.getLatest().find("TEST2", strlen("TEST2")) == w2);
    mu_check(forth.getLatest().find("TEST", strlen("TEST")) == NULL);
}

MU_TEST_SUITE(forth_tests) {
    MU_RUN_TEST(forth_tests_init_free);
    MU_RUN_TEST(forth_tests_align);
    MU_RUN_TEST(forth_tests_push_pop);
    MU_RUN_TEST(forth_tests_emit);
    MU_RUN_TEST(forth_tests_codeword);
}
