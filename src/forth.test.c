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
    forth_init(&forth, stdin, stdout, 100, 100, 100);
    
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
    forth_init(&forth, stdin, stdout, 100, 100, 100);
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
    forth_init(&forth, stdin, stdout, 100, 100, 100);
    forth_emit(&forth, 123);

    mu_check(forth.memory_free > forth.memory);
    mu_check(*forth.memory == 123);

    forth_free(&forth);
}

MU_TEST(forth_tests_codeword) {
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 100, 100, 100);

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
    forth_init(&forth, stdin, stdout, 200, 200, 200);
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
    forth_init(&forth, stdin, stdout, 200, 200, 200);
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

MU_TEST(forth_tests_io){
    struct forth forth = {0};
    FILE *zero = fopen("/dev/zero", "r");
    FILE *devnull = fopen("/dev/null", "w");
    forth_init(&forth, stdin, stdout, 100, 100, 100);
    mu_check(!strcmp(forth.prompt, "cforth> "));
    forth_set_input(&forth, zero);
    mu_check(!*forth.prompt); // forth->prompt == ""
    forth_set_output(&forth, devnull);
    mu_check(!*forth.prompt);
}

MU_TEST(forth_tests_cell_print){
    char *str = (char*)malloc(32 * sizeof(char));
    FILE *out = fmemopen(str, 31 * sizeof(char), "w");
    cell_print(out, 123);
    fflush(out);
    mu_check(!strncmp(str, "123 ", 3));
    fclose(out);
    free(str);
}

MU_TEST(forth_tests_read_word){
    char buffer[32];
    enum forth_result retval;
    size_t length;
    char *str1 = strdup("foo");
    char *str2 = strdup("  bar");
    char *str3 = strdup("foo\n\t bar");
    char *str4 = strdup("   ");

    FILE *test1 = fmemopen(str1, strlen(str1) * sizeof(char), "r");
    FILE *test2 = fmemopen(str2, strlen(str2) * sizeof(char), "r");
    FILE *test3 = fmemopen(str3, strlen(str3) * sizeof(char), "r");
    FILE *test4 = fmemopen(str4, strlen(str4) * sizeof(char), "r");

    retval = read_word(test1, "", 31, buffer, &length);
    mu_check(!strncmp(buffer, "foo", 3));
    mu_check(retval == FORTH_OK);
    mu_check(length == 3);

    retval = read_word(test2, "", 31, buffer, &length);
    mu_check(!strncmp(buffer, "bar", 3));
    mu_check(retval == FORTH_OK);
    mu_check(length == 3);

    retval = read_word(test3, "", 31, buffer, &length);
    mu_check(!strncmp(buffer, "foo", 3));
    mu_check(retval == FORTH_OK);
    mu_check(length == 3);

    retval = read_word(test4, "", 31, buffer, &length);
    mu_check(retval == FORTH_EOF);
    
    retval = read_word(test3, "", 2, buffer, &length);
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
    forth_init(&forth, stdin, stdout, 200, 200, 200);
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
    FILE *stream = fmemopen(program, strlen(program) * sizeof(char), "r");
    struct forth forth = {0};
    forth_init(&forth, stream, stdout, 200, 200, 200);
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
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    forth_push(&forth, 123);
    drop(&forth);
    mu_check(forth.sp0 == forth.sp);
    forth_free(&forth);
}

MU_TEST(words_tests_dup){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    forth_push(&forth, 123);
    _dup(&forth);
    mu_check(forth_pop(&forth) == 123);
    mu_check(forth_pop(&forth) == 123);
    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_add){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    
    forth_push(&forth, 1);
    forth_push(&forth, 2);
    add(&forth);
    mu_check(forth_pop(&forth) == 3);

    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_sub){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    
    forth_push(&forth, 3);
    forth_push(&forth, 2);
    sub(&forth);
    mu_check(forth_pop(&forth) == 1);

    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_mul){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    
    forth_push(&forth, 3);
    forth_push(&forth, 4);
    mul(&forth);
    mu_check(forth_pop(&forth) == 12);

    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_div){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    
    forth_push(&forth, 24);
    forth_push(&forth, 7);
    _div(&forth);
    mu_check(forth_pop(&forth) == 3);

    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_mod){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    
    forth_push(&forth, 24);
    forth_push(&forth, 11);
    mod(&forth);
    mu_check(forth_pop(&forth) == 2);

    forth_free(&forth);
}

MU_TEST_SUITE(words_tests_swap){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    
    forth_push(&forth, 1);
    forth_push(&forth, 2);
    swap(&forth);
    mu_check(forth_pop(&forth) == 1);
    mu_check(forth_pop(&forth) == 2);

    forth_free(&forth);
}

MU_TEST(words_tests_rot_back){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

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
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    forth_push(&forth, 1);
    forth_push(&forth, 2);
    forth_push(&forth, 3);
    rot(&forth);
    mu_check(forth_pop(&forth) == 1);
    mu_check(forth_pop(&forth) == 3);
    mu_check(forth_pop(&forth) == 2);
    forth_free(&forth);
}

MU_TEST(words_tests_show){
    char *str;
    FILE *out;
    struct forth forth = {0};
    str = (char*)malloc(32 * sizeof(char));
    out = fmemopen(str, 31*sizeof(char), "w");
    forth_init(&forth, stdin, out, 200, 200, 200);
    
    forth_push(&forth, 1);
    forth_push(&forth, 2);
    show(&forth);
    fflush(out);
    mu_check(!strncmp(str, "1 2 (top)", strlen("1 2 (top)")));
    forth_free(&forth);
    fclose(out);
    free(str);
}

MU_TEST(words_tests_over){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

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
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    _true(&forth);
    mu_check(forth_pop(&forth) == -1);

    forth_free(&forth);
}

MU_TEST(words_tests_false){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    _false(&forth);
    mu_check(forth_pop(&forth) == 0);

    forth_free(&forth);
}

MU_TEST(words_tests_xor){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    forth_push(&forth, 3); // 0011
    forth_push(&forth, 5); // 0101
    _xor(&forth);
    mu_check(forth_pop(&forth) == 6); // 0110

    forth_free(&forth);
}

MU_TEST(words_tests_or){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    forth_push(&forth, 3); // 0011
    forth_push(&forth, 5); // 0101
    _or(&forth);
    mu_check(forth_pop(&forth) == 7); // 0111

    forth_free(&forth);
}

MU_TEST(words_tests_and){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    forth_push(&forth, 3); // 0011
    forth_push(&forth, 5); // 0101
    _and(&forth);
    mu_check(forth_pop(&forth) == 1); // 0001

    forth_free(&forth);
}

MU_TEST(words_tests_not){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    forth_push(&forth, 3); // 0011
    _not(&forth);
    mu_check(forth_pop(&forth) == ~3);

    forth_free(&forth);
}

MU_TEST(words_tests_eq){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

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
    forth_init(&forth, stdin, stdout, 200, 200, 200);

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
    forth_init(&forth, stdin, stdout, 200, 200, 200);

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

MU_TEST(words_tests_rstack){
    struct forth forth = {0};
    const struct word *square;
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    words_add(&forth);
    square = word_find(forth.latest, strlen("square"), "square");
    forth_push(&forth, (cell)forth.stopword);
    rpush(&forth);
    mu_check(*forth.rp0 == (cell)forth.stopword);
    forth_push_return(&forth, (cell)square);
    forth_push_return(&forth, (cell)square);
    rpop(&forth);
    mu_check(forth_pop(&forth) == (cell)square);
    rtop(&forth);
    mu_check(forth_pop(&forth) == (cell)forth.stopword);

    forth_free(&forth);
}

MU_TEST(words_tests_rshow){
    char *str;
    FILE *out;
    struct forth forth = {0};
    str = (char*)malloc(32 * sizeof(char));
    out = fmemopen(str, 31*sizeof(char), "w");
    forth_init(&forth, stdin, out, 200, 200, 200);
    
    forth_push_return(&forth, 1);
    forth_push_return(&forth, 2);
    rshow(&forth);
    fflush(out);
    mu_check(!strncmp(str, "1 2 (r-top)", strlen("1 2 (r-top)")));
    forth_free(&forth);
    fclose(out);
    free(str);
}

MU_TEST(words_tests_memory){
    cell *foo;
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    foo = (intptr_t*)malloc(sizeof(intptr_t));
    *foo = 123;

    forth_push(&forth, (cell)foo);
    memory_read(&forth);
    mu_check(forth_pop(&forth) == *foo);
    forth_push(&forth, 567);
    forth_push(&forth, (cell)foo);
    memory_write(&forth);
    mu_check(*foo == 567);
    forth_free(&forth);
    free(foo);
}

MU_TEST(words_tests_here){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    here(&forth);
    mu_check(forth_pop(&forth) == (cell)&forth.memory_free);

    forth_free(&forth);
}

MU_TEST(words_tests_branch){
    struct forth forth = {0};
    const struct word *dup, *xor_word;
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    words_add(&forth);
    word_add(&forth, strlen("foo"), "foo");
    dup = word_find(forth.latest, strlen("dup"), "dup");
    xor_word = word_find(forth.latest, strlen("xor"), "xor");
    forth.executing = (struct word**)forth.memory_free;
    forth_emit(&forth, 2*sizeof(cell));
    forth_emit(&forth, (cell)xor_word);
    forth_emit(&forth, (cell)dup);
    branch(&forth);
    mu_check(*forth.executing == dup);

    forth_free(&forth);
}

MU_TEST(words_tests_branch0){
    struct forth forth = {0};
    const struct word *dup, *xor_word;
    cell *code_ptr;
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    words_add(&forth);
    word_add(&forth, strlen("foo"), "foo");
    dup = word_find(forth.latest, strlen("dup"), "dup");
    xor_word = word_find(forth.latest, strlen("xor"), "xor");
    code_ptr = forth.memory_free;
    forth.executing = (struct word**)code_ptr;

    forth_push(&forth, 0);
    forth_emit(&forth, 2*sizeof(cell));
    forth_emit(&forth, (cell)xor_word);
    forth_emit(&forth, (cell)dup);
    branch0(&forth);
    mu_check(*forth.executing == dup);

    forth.executing = (struct word**)code_ptr;
    forth_push(&forth, -1);
    branch0(&forth);
    mu_check(*forth.executing == xor_word);
    
    forth_free(&forth);
}

MU_TEST(words_tests_immediate) {
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    words_add(&forth);
    
    immediate(&forth);
    mu_check(forth.latest->immediate);

    forth_free(&forth);
}

MU_TEST(words_tests_next_word){
    char *test;
    char *word = strdup("bar");
    FILE *input = fmemopen(word, strlen("bar") * sizeof(char), "r");
    struct forth forth = {0};
    forth_init(&forth, input, stdout, 200, 200, 200);
    next_word(&forth);

    mu_check(forth_pop(&forth) == (cell)strlen(word));
    test = (char*)forth_pop(&forth);
    mu_check(!strncmp(word, test, 3));

    forth_free(&forth);
    fclose(input);
    free(word);
}

MU_TEST(words_tests_find){
    struct forth forth = {0};
    char *word = strdup("_xor");
    const struct word *xor_word;
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    words_add(&forth);
    xor_word = word_find(forth.latest, strlen("_xor"), "_xor");

    forth_push(&forth, (cell)word);
    forth_push(&forth, strlen("_xor"));
    find(&forth);

    mu_check((struct word*)forth_pop(&forth) == xor_word);

    forth_free(&forth);
    free(word);
}

MU_TEST(words_tests_word_code){
    struct forth forth = {0};
    const struct word *word;
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    words_add(&forth);
    word = word_find(forth.latest, strlen("immediate"), "immediate");

    forth_push(&forth, (cell)word);
    _word_code(&forth);
    mu_check(forth_pop(&forth) == (cell)word_code(word));

    forth_free(&forth);
}

MU_TEST(words_tests_comma){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);
    
    forth_push(&forth, 1);
    comma(&forth);
    mu_check(*forth.memory == 1);

    forth_free(&forth);
}

MU_TEST(words_tests_next){
    struct forth forth = {0};
    forth_init(&forth, stdin, stdout, 200, 200, 200);

    forth.executing = (struct word**)forth.memory;
    forth_emit(&forth, 123);
    forth_emit(&forth, 456);
    next(&forth);
    mu_check(*(cell*)forth.executing == 456);

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
    MU_RUN_TEST(forth_tests_io);
    MU_RUN_TEST(forth_tests_cell_print);
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
    MU_RUN_TEST(words_tests_show);
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
    MU_RUN_TEST(words_tests_rstack);
    MU_RUN_TEST(words_tests_rshow);
    MU_RUN_TEST(words_tests_memory);
    MU_RUN_TEST(words_tests_here);
    MU_RUN_TEST(words_tests_branch);
    MU_RUN_TEST(words_tests_branch0);
    MU_RUN_TEST(words_tests_immediate);
    MU_RUN_TEST(words_tests_next_word);
    MU_RUN_TEST(words_tests_find);
    MU_RUN_TEST(words_tests_word_code);
    MU_RUN_TEST(words_tests_comma);
    MU_RUN_TEST(words_tests_next);
}
// vim: ts=4 sw=4 expandtab
