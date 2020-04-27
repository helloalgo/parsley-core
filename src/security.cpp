#include "security.hpp"

void allowCalls(scmp_filter_ctx* ctx, std::vector<int> calls) {
  for (auto it=calls.begin(); it!=calls.end(); it++) {
    seccomp_rule_add(*ctx, SCMP_ACT_ALLOW, *it, 0);
  }
}
