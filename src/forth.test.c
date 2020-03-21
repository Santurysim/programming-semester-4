#include "forth.c"
#include "words.c"
#include "minunit.h"

MU_TEST(forth_tests_init_free) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 100, 100, 100);
    
    mu_check(forth.memory_free == 0);
    mu_check(forth.memory != NULL);
    mu_check(forth.sp == 0);
    mu_check(forth.sp0 != NULL);

    forth_free(&forth);
}

MU_TEST(forth_tests_align) {
    mu_check(align(8, 8) == 8);
    mu_check(align(9, 8) == 16);
    mu_check(align(7, 8) == 8);
}

MU_TEST(forth_tests_push_pop) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 100, 100, 100);
    forth_push(&forth, 123);

    mu_check(forth.sp > 0);
    mu_check(forth_pop(&forth) == 123);
    mu_check(forth.sp == 0);
}

MU_TEST(forth_tests_emit) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 100, 100, 100);
    forth_emit(&forth, 123);

    mu_check(forth.memory_free > 0);
    mu_check(*forth.memory == 123);
}

MU_TEST(forth_tests_codeword) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 100, 100, 100);

    mu_check(forth.latest == 0);

    offset w1 = word_add(&forth, strlen("TEST1"), "TEST1");
    forth_emit(&forth, 123);
    mu_check(forth.latest == w1);

    offset w2 = word_add(&forth, strlen("TEST2"), "TEST2");
    mu_check((forth.memory[word_code(&forth, w1)]) == 123);
    mu_check(w2 > word_code(&forth, w1));
    mu_check(forth.latest == w2);

    mu_check(word_find(&forth, forth.latest, strlen("TEST1"), "TEST1") == w1);
    mu_check(word_find(&forth, forth.latest, strlen("TEST2"), "TEST2") == w2);
    mu_check(word_find(&forth, forth.latest, strlen("TEST"), "TEST") == 0);
}

MU_TEST(forth_tests_compileword) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 1000, 1000, 1000);
    words_add(&forth);

    offset dup = word_find(&forth, forth.latest, strlen("dup"), "dup");
    offset mul = word_find(&forth, forth.latest, strlen("*"), "*");
    offset exit = word_find(&forth, forth.latest, strlen("exit"), "exit");
    offset square = word_find(&forth, forth.latest, strlen("square"), "square");
    mu_check(square);
    offset words = word_code(&forth, square);
    mu_check(*(offset*)(forth.memory + words) == dup);
    mu_check(*(offset*)(forth.memory + words + 1) == mul);
    mu_check(*(offset*)(forth.memory + words + 2) == exit);
    offset w1 = word_add(&forth, strlen("TEST1"), "TEST1");
    mu_check(w1 > words+2);
}

MU_TEST(forth_tests_literal) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 1000, 1000, 1000);
    words_add(&forth);

    offset literal = word_find(&forth, forth.latest, strlen("lit"), "lit");
    offset exit = word_find(&forth, forth.latest, strlen("exit"), "exit");
    offset test = word_add(&forth, strlen("TEST"), "TEST");
    WORD_PTR(&forth, test)->compiled = 1;
    forth_emit(&forth, (cell)literal);
    forth_emit(&forth, 4567);
    forth_emit(&forth, (cell)exit);

    forth_run_word(&forth, test);
    cell c = forth_pop(&forth);
    mu_check(c == 4567);
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
