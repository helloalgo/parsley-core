#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <stdio.h>
#include <seccomp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include "runner.hpp"

void exitWithError(RunError error) {
    fflush(stdout);
    fflush(stderr);
    raise(SIGUSR1);
    exit(error);
}
void safe_setrlimit(int type, const rlimit &limit) {
    if (setrlimit(type, &limit) != 0) {
        exitWithError(RunError::SETRLIMIT_FAILED);
    }
}
void safe_dup2(int src, int dst) {
    if (dup2(src, dst) == -1) {
        printf("%s", strerror(errno));
        exitWithError(RunError::DUP_FAILED);
    }
}

void child_process(const RunArgs& args) {
    auto scmpFilter = generateFilter(args.seccomp_policy);
    if (args.max_stack_size > 0) {
        rlimit lim;
        lim.rlim_cur = lim.rlim_max = (rlim_t)(args.max_stack_size<<10);
        safe_setrlimit(RLIMIT_STACK, lim);
    }
    if (args.max_memory_size > 0) {
        rlimit lim;
        lim.rlim_cur = lim.rlim_max = (rlim_t)(args.max_memory_size<<11);
        safe_setrlimit(RLIMIT_AS, lim);
    }
    if (args.max_process_count > 0) {
        rlimit lim;
        lim.rlim_cur = lim.rlim_max = (rlim_t)args.max_process_count;
        safe_setrlimit(RLIMIT_NPROC, lim);
    }
    if (args.max_write_size > 0) {
        rlimit lim;
        lim.rlim_cur = lim.rlim_max = (rlim_t)args.max_write_size;
        safe_setrlimit(RLIMIT_FSIZE, lim);
    }
    if (args.max_cpu_time > 0) {
        rlimit lim;
        lim.rlim_cur = lim.rlim_max = (rlim_t)((args.max_cpu_time + 1000)/1000);
        safe_setrlimit(RLIMIT_CPU, lim);
    }
    if (args.input != nullptr) {
        safe_dup2(fileno(args.input), STDIN_FILENO);
    }
    if (args.output != nullptr) {
        safe_dup2(fileno(args.output), STDOUT_FILENO);
    }
    if (args.error != nullptr) {
        safe_dup2(fileno(args.error), STDERR_FILENO);
    }
    if (args.use_ptrace) {
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) != 0) {
            exitWithError(RunError::PTRACE_FAILED);
        }
    }
    prctl(PR_SET_NO_NEW_PRIVS, 1);
    if (seccomp_load(scmpFilter) != 0) {
        exitWithError(RunError::SECCOMP_LOAD_FAILED);
    }
    execv(*args.args, args.args);
    exitWithError(RunError::EXEC_FAILED);
}
