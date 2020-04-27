#include <seccomp.h>
#include <vector>

void allowCalls(scmp_filter_ctx* ctx, std::vector<int> calls);