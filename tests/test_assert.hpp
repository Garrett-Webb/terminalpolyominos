#pragma once

#include <cstdio>

namespace tp::test {

inline int g_failures = 0;
inline int g_checks = 0;

inline void check(bool cond, const char* expr, const char* file, int line) {
  ++g_checks;
  if (!cond) {
    ++g_failures;
    std::fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expr);
  }
}

inline int summary(const char* suite) {
  if (g_failures == 0) {
    std::printf("%s: %d checks OK\n", suite, g_checks);
    return 0;
  }
  std::printf("%s: %d/%d checks failed\n", suite, g_failures, g_checks);
  return 1;
}

}  // namespace tp::test

#define TP_CHECK(expr) ::tp::test::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
