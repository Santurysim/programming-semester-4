#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L
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

MU_TEST(forth_tests_data_stack) {
    Forth forth(stdin, 100, 100, 100);
    forth.push(123);

    mu_check(forth.getStackPointer() > forth.getStackBottom());
    mu_check(forth.pop() == 123);
    mu_check(forth.getStackBottom() == forth.getStackPointer());

    forth.push(456);
    mu_check(*forth.top() == 456);
    *forth.top() = 789;
    mu_check(forth.pop() == 789);
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
	Forth forth(stdin, 200, 200, 200);
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

    const char* dummy[] = {"foo", "bar", "exit"};
    int retval = forth.addCompiledWord("test", dummy);
    mu_check(retval == 1);
    mu_check(forth.getLatest()->isHidden());
}

MU_TEST(forth_tests_literal){
	Forth forth(stdin, 200, 200, 200);
	forth.addMachineWords();

	const Word *literal = forth.getLatest()->find("lit", strlen("lit"));
	const Word *exit = forth.getLatest()->find("exit", strlen("exit"));
	Word *test = forth.addWord("TEST", strlen("TEST"), true);
	forth.emit((cell)literal);
	forth.emit(4567);
	forth.emit((cell)exit);

	forth.runWord(test);
	cell c = forth.pop();
	mu_check(c == 4567);
}

MU_TEST(forth_tests_read_word){
    char buffer[32];
    enum ForthResult retval;
    size_t length;
    char *str1 = strdup("foo");
    char *str2 = strdup("  bar");
    char *str3 = strdup("foo\n\t bar");
    char *str4 = strdup("   ");

    FILE *test1 = fmemopen(str1, strlen(str1), "r");
    FILE *test2 = fmemopen(str2, strlen(str2), "r");
    FILE *test3 = fmemopen(str3, strlen(str3), "r");
    FILE *test4 = fmemopen(str4, strlen(str4), "r");

    retval = readWord(test1, buffer, 31, &length);
    mu_check(!strncmp(buffer, "foo", 3));
    mu_check(retval == FORTH_OK);
    mu_check(length == 3);

    retval = readWord(test2, buffer, 31, &length);
    mu_check(!strncmp(buffer, "bar", 3));
    mu_check(retval == FORTH_OK);
    mu_check(length == 3);

    retval = readWord(test3, buffer, 31, &length);
    mu_check(!strncmp(buffer, "foo", 3));
    mu_check(retval == FORTH_OK);
    mu_check(length == 3);

    retval = readWord(test4, buffer, 31, &length);
    mu_check(retval == FORTH_EOF);
    
    retval = readWord(test3, buffer, 2, &length);
    mu_check(retval == FORTH_BUFFER_OVERFLOW);

    fclose(test1); fclose(test2); fclose(test3); fclose(test4);
    free(str1); free(str2); free(str3); free(str4);
}

MU_TEST(forth_tests_run_number){
	const cell *test;
    const Word **code_ptr;
    Word *word;
    const Word *lit;
    Forth forth(stdin, 200, 200, 200);
    const char *str1 = "1";
    const char *str2 = "foo";
    forth.addMachineWords();

    forth.runNumber(str1, strlen(str1));
    mu_check(forth.pop() == 1);

    test = forth.getStackPointer();
    forth.runNumber(str2, strlen(str2));
    mu_check(forth.getStackPointer() == test);

    word = forth.addWord("bar", strlen("bar"), true);
    word->setCompiled(true);
    lit = forth.getLatest()->find("lit", strlen("lit"));
    forth.setCompiling(true);
    forth.runNumber(str1, strlen(str1));
    code_ptr = (const Word**)word->getCode();
    mu_check(*code_ptr == lit);
    code_ptr += 1;
    mu_check((cell)(*code_ptr) == 1);
}

MU_TEST(forth_tests_run){
    char *program = strdup(": init_fib 1 1 ; : next_fib swap over + ; init_fib next_fib next_fib");
    FILE *stream = fmemopen(program, strlen(program), "r");
    Forth forth(stdin, 200, 200, 200);
    forth.setInput(stream);
    forth.addMachineWords();
    forth.run();

    mu_check(forth.pop() == 3);
    mu_check(forth.pop() == 2);
    
    fclose(stream);
    free(program);
}

MU_TEST_SUITE(forth_tests) {
    MU_RUN_TEST(forth_tests_init_free);
    MU_RUN_TEST(forth_tests_align);
    MU_RUN_TEST(forth_tests_data_stack);
    MU_RUN_TEST(forth_tests_emit);
    MU_RUN_TEST(forth_tests_codeword);
    MU_RUN_TEST(forth_tests_compileword);
    MU_RUN_TEST(forth_tests_literal);
    MU_RUN_TEST(forth_tests_literal);
    MU_RUN_TEST(forth_tests_read_word);
    MU_RUN_TEST(forth_tests_run_number);
    MU_RUN_TEST(forth_tests_run);
}
