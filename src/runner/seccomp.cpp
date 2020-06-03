#include <cstdio>
#include <cstring>
#include <vector>
#include "runner.hpp"

void allowCalls(scmp_filter_ctx ctx, std::vector<int> calls) {
  for (auto it=calls.begin(); it!=calls.end(); it++) {
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, *it, 0);
  }
}

const auto BASIC_CALLS = std::vector<int>({
    SCMP_SYS(read),
    SCMP_SYS(write),
    SCMP_SYS(open),
    SCMP_SYS(close),
    SCMP_SYS(stat),
    SCMP_SYS(fstat),
    SCMP_SYS(arch_prctl),
    SCMP_SYS(lseek),
    SCMP_SYS(mmap),
    SCMP_SYS(mprotect),
    SCMP_SYS(munmap),
    SCMP_SYS(brk),
    SCMP_SYS(pread64),
    SCMP_SYS(writev),
    SCMP_SYS(access),
    SCMP_SYS(execve),
    SCMP_SYS(uname),
    SCMP_SYS(readlink),
    SCMP_SYS(arch_prctl),
    SCMP_SYS(time),
    SCMP_SYS(futex),
    SCMP_SYS(set_thread_area),
    SCMP_SYS(exit_group),
    SCMP_SYS(mq_open),
    SCMP_SYS(unshare),
    SCMP_SYS(set_robust_list),
    SCMP_SYS(splice),
    SCMP_SYS(getrandom),
    SCMP_SYS(openat),
    SCMP_SYS(times),
    -1
});

void basicFilter(scmp_filter_ctx filter) {
    printf("Generating seccomp filter basic\n");
    allowCalls(filter, BASIC_CALLS);
}

scmp_filter_ctx generateFilter(const char* key) {
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_KILL);

    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sigreturn), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(execve), 0);

    if (strncmp(key, "basic", 5)==0) {
        basicFilter(ctx);
    }
    return ctx;
}
