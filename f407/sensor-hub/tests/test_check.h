#pragma once

#include <cstdlib>
#include <iostream>

#define TEST_CHECK(expression)                                                        \
    do {                                                                              \
        if (!(expression)) {                                                          \
            std::cerr << "CHECK failed: " #expression << " at " << __FILE__ << ':'   \
                      << __LINE__ << '\n';                                            \
            std::exit(EXIT_FAILURE);                                                  \
        }                                                                             \
    } while (false)

// Keep existing tests readable while making every check active in Debug and Release.
// Unlike the standard assert macro, this alias always evaluates the expression.
#ifdef assert
#undef assert
#endif
#define assert(expression) TEST_CHECK(expression)
