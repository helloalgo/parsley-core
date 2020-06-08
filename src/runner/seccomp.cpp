#include <cstdio>
#include <cstring>
#include <vector>
#include "runner.hpp"

void allowCalls(scmp_filter_ctx ctx, int calls[]) {
    int length = sizeof(calls) / sizeof(int);
    for (int i=0;i<length;i++) {
        seccomp_rule_add(ctx, SCMP_ACT_ALLOW, calls[i], 0);
    }
}

int BASIC_CALLS[] = {SCMP_SYS(read), SCMP_SYS(fstat),
                    SCMP_SYS(mmap), SCMP_SYS(mprotect),
                    SCMP_SYS(munmap), SCMP_SYS(uname),
                    SCMP_SYS(arch_prctl), SCMP_SYS(brk),
                    SCMP_SYS(access), SCMP_SYS(exit_group),
                    SCMP_SYS(close), SCMP_SYS(readlink),
                    SCMP_SYS(sysinfo), SCMP_SYS(write),
                    SCMP_SYS(writev), SCMP_SYS(lseek),
                    SCMP_SYS(open), SCMP_SYS(openat),
                    SCMP_SYS(clock_gettime)};


void basicFilter(scmp_filter_ctx filter) {
    allowCalls(filter, BASIC_CALLS);
}

scmp_filter_ctx generateFilter(const char* key) {
    if (strncmp(key, "none", 5) == 0) {
        return seccomp_init(SCMP_ACT_ALLOW);
    }
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_KILL);

    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sigreturn), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(execve), 0);

    if (strncmp(key, "basic", 6)==0) {
        basicFilter(ctx);
        return ctx;
    }
    return ctx;
}
