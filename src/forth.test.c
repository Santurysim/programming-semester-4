#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include "forth.c"
#include "words.c"
#include "minunit.h"

MU_TEST(forth_tests_init_free) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 100, 100, 100);
    
    mu_check(forth.memory == forth.memory_free);
    mu_check(forth.memory != NULL);
    mu_check(forth.sp0 == forth.sp);
    mu_check(forth.sp0 != NULL);

    forth_free(&forth);
}

MU_TEST(forth_tests_align) {
    mu_check(align(8, 8) == 8);
    mu_check(align(9, 8) == 16);
    mu_check(align(7, 8) == 8);
}

MU_TEST(forth_tests_data_stack) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 100, 100, 100);
    forth_push(&forth, 123);

    mu_check(forth.sp > forth.sp0);
    mu_check(forth_pop(&forth) == 123);
    mu_check(forth.sp0 == forth.sp);

    forth_push(&forth, 456);
    mu_check(*forth_top(&forth) == 456);
    *forth_top(&forth) = 789;
    mu_check(forth_pop(&forth) == 789);

    forth_free(&forth);
}

MU_TEST(forth_tests_emit) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 100, 100, 100);
    forth_emit(&forth, 123);

    mu_check(forth.memory_free > forth.memory);
    mu_check(*forth.memory == 123);

    forth_free(&forth);
}

MU_TEST(forth_tests_codeword) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 100, 100, 100);

    mu_check(forth.latest == NULL);

    struct word *w1 = word_add(&forth, strlen("TEST1"), "TEST1");
    forth_emit(&forth, 123);
    mu_check(forth.latest == w1);

    struct word *w2 = word_add(&forth, strlen("TEST2"), "TEST2");
    mu_check((*(cell*)word_code(w1)) == 123);
    mu_check((void*)w2 > word_code(w1));
    mu_check(forth.latest == w2);

    mu_check(word_find(forth.latest, strlen("TEST1"), "TEST1") == w1);
    mu_check(word_find(forth.latest, strlen("TEST2"), "TEST2") == w2);
    mu_check(word_find(forth.latest, strlen("TEST"), "TEST") == NULL);

    forth_free(&forth);
}

MU_TEST(forth_tests_compileword) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);
    words_add(&forth);

    const struct word *dup = word_find(forth.latest, strlen("dup"), "dup");
    const struct word *mul = word_find(forth.latest, strlen("*"), "*");
    const struct word *exit = word_find(forth.latest, strlen("exit"), "exit");
    const struct word *square = word_find(forth.latest, strlen("square"), "square");
    mu_check(square);
    struct word **words = (struct word**)word_code(square);
    mu_check(words[0] == dup);
    mu_check(words[1] == mul);
    mu_check(words[2] == exit);
    struct word *w1 = word_add(&forth, strlen("TEST1"), "TEST1");
    mu_check((void*)w1 > (void*)(words+2));


    const char* dummy[] = {"foo", "bar", "exit"};
    int retval = forth_add_compileword(&forth, "test", dummy);
    mu_check(retval == 1);
    mu_check(forth.latest->hidden == true);

    forth_free(&forth);
}

MU_TEST(forth_tests_literal) {
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);
    words_add(&forth);

    const struct word *literal = word_find(forth.latest, strlen("lit"), "lit");
    const struct word *exit = word_find(forth.latest, strlen("exit"), "exit");
    struct word *test = word_add(&forth, strlen("TEST"), "TEST");
    test->compiled = true;
    forth_emit(&forth, (cell)literal);
    forth_emit(&forth, 4567);
    forth_emit(&forth, (cell)exit);

    forth_run_word(&forth, test);
    cell c = forth_pop(&forth);
    mu_check(c == 4567);

    forth_free(&forth);
}

MU_TEST(forth_tests_read_word){
    char buffer[32];
    enum forth_result retval;
    size_t length;
    char *str1 = strdup("foo");
    char *str2 = strdup("  bar");
    char *str3 = strdup("foo\n\t bar");
    char *str4 = strdup("   ");

    FILE *test1 = fmemopen(str1, strlen(str1), "r");
    FILE *test2 = fmemopen(str2, strlen(str2), "r");
    FILE *test3 = fmemopen(str3, strlen(str3), "r");
    FILE *test4 = fmemopen(str4, strlen(str4), "r");

    retval = read_word(test1, 31, buffer, &length);
    mu_check(!strncmp(buffer, "foo", 3));
    mu_check(retval == FORTH_OK);
    mu_check(length == 3);

    retval = read_word(test2, 31, buffer, &length);
    mu_check(!strncmp(buffer, "bar", 3));
    mu_check(retval == FORTH_OK);
    mu_check(length == 3);

    retval = read_word(test3, 31, buffer, &length);
    mu_check(!strncmp(buffer, "foo", 3));
    mu_check(retval == FORTH_OK);
    mu_check(length == 3);

    retval = read_word(test4, 31, buffer, &length);
    mu_check(retval == FORTH_EOF);
    
    retval = read_word(test3, 2, buffer, &length);
    mu_check(retval == FORTH_BUFFER_OVERFLOW);

    fclose(test1); fclose(test2); fclose(test3); fclose(test4);
    free(str1); free(str2); free(str3); free(str4);
}

MU_TEST(forth_tests_run_number){
	const cell *test;
    const struct word **code_ptr;
    struct word *word;
    const struct word *lit;
    struct forth forth = {0};
    const char *str1 = "1";
    const char *str2 = "foo";
    forth_init(&forth, stdin, 200, 200, 200);
    words_add(&forth);

    forth_run_number(&forth, strlen(str1), str1);
    mu_check(forth_pop(&forth) == 1);

    test = forth.sp;
    forth_run_number(&forth, strlen(str2), str2);
    mu_check(forth.sp == test);

    word = word_add(&forth, strlen("bar"), "bar");
    lit = word_find(forth.latest, strlen("lit"), "lit");
    forth.is_compiling = true;
    word->compiled = true;
    forth_run_number(&forth, strlen(str1), str1);
    code_ptr = (const struct word**)word_code(word);
    mu_check(*code_ptr == lit);
    code_ptr += 1;
    mu_check((cell)(*code_ptr) == 1);

    forth_free(&forth);
}

MU_TEST(forth_tests_run){
    char *program = strdup(": init_fib 1 1 ; : next_fib swap over + ; init_fib next_fib next_fib");
    FILE *stream = fmemopen(program, strlen(program), "r");
    struct forth forth = {0};
    forth_init(&forth, stream, 200, 200, 200);
    words_add(&forth);
    forth_run(&forth);

    mu_check(forth_pop(&forth) == 3);
    mu_check(forth_pop(&forth) == 2);
    
    forth_free(&forth);
    fclose(stream);
    free(program);
}

MU_TEST(words_tests_drop){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 123);
    drop(&forth);
    mu_check(forth.sp0 == forth.sp);
    forth_free(&forth);
}

MU_TEST(words_tests_dup){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 123);
    _dup(&forth);
    mu_check(forth_pop(&forth) == 123);
    mu_check(forth_pop(&forth) == 123);
    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_add){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);
    
    forth_push(&forth, 1);
    forth_push(&forth, 2);
    add(&forth);
    mu_check(forth_pop(&forth) == 3);

    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_sub){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);
    
    forth_push(&forth, 3);
    forth_push(&forth, 2);
    sub(&forth);
    mu_check(forth_pop(&forth) == 1);

    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_mul){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);
    
    forth_push(&forth, 3);
    forth_push(&forth, 4);
    mul(&forth);
    mu_check(forth_pop(&forth) == 12);

    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_div){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);
    
    forth_push(&forth, 24);
    forth_push(&forth, 7);
    _div(&forth);
    mu_check(forth_pop(&forth) == 3);

    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_mod){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);
    
    forth_push(&forth, 24);
    forth_push(&forth, 11);
    mod(&forth);
    mu_check(forth_pop(&forth) == 2);

    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_swap){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);
    
    forth_push(&forth, 1);
    forth_push(&forth, 2);
    swap(&forth);
    mu_check(forth_pop(&forth) == 1);
    mu_check(forth_pop(&forth) == 2);

    forth_free(&forth);
}

MU_TEST(words_tests_rot_back){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 1);
    forth_push(&forth, 2);
    forth_push(&forth, 3);
    rot_back(&forth);
    mu_check(forth_pop(&forth) == 2);
    mu_check(forth_pop(&forth) == 1);
    mu_check(forth_pop(&forth) == 3);
    forth_free(&forth);
}

MU_TEST(words_tests_rot){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 1);
    forth_push(&forth, 2);
    forth_push(&forth, 3);
    rot(&forth);
    mu_check(forth_pop(&forth) == 1);
    mu_check(forth_pop(&forth) == 3);
    mu_check(forth_pop(&forth) == 2);
    forth_free(&forth);
}

MU_TEST(words_tests_over){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 1);
    forth_push(&forth, 2);
    over(&forth);
    mu_check(forth_pop(&forth) == 1);
    mu_check(forth_pop(&forth) == 2);
    mu_check(forth_pop(&forth) == 1);
    forth_free(&forth);
}

MU_TEST(words_tests_true){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    _true(&forth);
    mu_check(forth_pop(&forth) == -1);

    forth_free(&forth);
}

MU_TEST(words_tests_false){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    _false(&forth);
    mu_check(forth_pop(&forth) == 0);

    forth_free(&forth);
}

MU_TEST(words_tests_xor){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 3); // 0011
    forth_push(&forth, 5); // 0101
    _xor(&forth);
    mu_check(forth_pop(&forth) == 6); // 0110

    forth_free(&forth);
}

MU_TEST(words_tests_or){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 3); // 0011
    forth_push(&forth, 5); // 0101
    _or(&forth);
    mu_check(forth_pop(&forth) == 7); // 0111

    forth_free(&forth);
}

MU_TEST(words_tests_and){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 3); // 0011
    forth_push(&forth, 5); // 0101
    _and(&forth);
    mu_check(forth_pop(&forth) == 1); // 0001

    forth_free(&forth);
}

MU_TEST(words_tests_not){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 3); // 0011
    _not(&forth);
    mu_check(forth_pop(&forth) == ~3);

    forth_free(&forth);
}

MU_TEST(words_tests_eq){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 1);
    forth_push(&forth, 1);
    _eq(&forth);
    mu_check(forth_pop(&forth) == -1);

    forth_push(&forth, 1);
    forth_push(&forth, 2);
    _eq(&forth);
    mu_check(forth_pop(&forth) == 0);

    forth_free(&forth);
}

MU_TEST(words_tests_lt){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 1);
    forth_push(&forth, 1);
    lt(&forth);
    mu_check(forth_pop(&forth) == 0);

    forth_push(&forth, 2);
    forth_push(&forth, 1);
    lt(&forth);
    mu_check(forth_pop(&forth) == 0);

    forth_push(&forth, 1);
    forth_push(&forth, 2);
    lt(&forth);
    mu_check(forth_pop(&forth) == -1);

    forth_free(&forth);
}

MU_TEST(words_tests_within){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_push(&forth, 123);
    forth_push(&forth, 100);
    forth_push(&forth, 200);
    within(&forth);
    mu_check(forth_pop(&forth) == -1);

    forth_push(&forth, 400);
    forth_push(&forth, 222);
    forth_push(&forth, 333);
    within(&forth);
    mu_check(forth_pop(&forth) == 0);

    forth_push(&forth, 123);
    forth_push(&forth, 1000);
    forth_push(&forth, 30);
    within(&forth);
    mu_check(forth_pop(&forth) == 0);

    forth_free(&forth);
}



MU_TEST_SUITE(forth_tests) {
    MU_RUN_TEST(forth_tests_init_free);
    MU_RUN_TEST(forth_tests_align);
    MU_RUN_TEST(forth_tests_data_stack);
    MU_RUN_TEST(forth_tests_emit);
    MU_RUN_TEST(forth_tests_codeword);
    MU_RUN_TEST(forth_tests_compileword);
    MU_RUN_TEST(forth_tests_literal);
    MU_RUN_TEST(forth_tests_read_word);
    MU_RUN_TEST(forth_tests_run_number);
    MU_RUN_TEST(forth_tests_run);
}

MU_TEST_SUITE(words_tests){
    MU_RUN_TEST(words_tests_drop);
    MU_RUN_TEST(words_tests_dup);
    MU_RUN_TEST(words_tests_add);
    MU_RUN_TEST(words_tests_sub);
    MU_RUN_TEST(words_tests_mul);
    MU_RUN_TEST(words_tests_div);
    MU_RUN_TEST(words_tests_mod);
    MU_RUN_TEST(words_tests_swap);
    MU_RUN_TEST(words_tests_rot_back);
    MU_RUN_TEST(words_tests_rot);
    MU_RUN_TEST(words_tests_over);
    MU_RUN_TEST(words_tests_true);
    MU_RUN_TEST(words_tests_false);
    MU_RUN_TEST(words_tests_xor);
    MU_RUN_TEST(words_tests_or);
    MU_RUN_TEST(words_tests_and);
    MU_RUN_TEST(words_tests_not);
    MU_RUN_TEST(words_tests_eq);
    MU_RUN_TEST(words_tests_lt);
    MU_RUN_TEST(words_tests_within);
}
// vim: ts=4 sw=4 expandtab
/*
MU_TEST(words_tests_template){
    struct forth forth = {0};
    forth_init(&forth, stdin, 200, 200, 200);

    forth_free(&forth);
}
*/