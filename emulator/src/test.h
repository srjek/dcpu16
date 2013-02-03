#ifndef emulator_test_h
#define emulator_test_h

#define TERM_RED "\x1b[31m"
#define TERM_GREEN "\x1b[32m"
#define TERM_YELLOW "\x1b[33m"

#define TERM_END "\x1b[0m"

#define TEST_FAIL TERM_RED "FAILED" TERM_END
#define TEST_SUCCESS TERM_GREEN "SUCCESS" TERM_END
#define TEST_NOT_TESTED TERM_YELLOW "NOT TESTED" TERM_END

#endif
