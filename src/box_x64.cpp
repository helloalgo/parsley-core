#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <seccomp.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <linux/ptrace.h>
#include <utility>
#include "box.hpp"
#include "security.hpp"

ProcResult* watchProcess(pid_t pid, const ProcLimit limit) {
    ProcResult* result = new ProcResult();
    rusage usage;
    timespec timeSpec;
    int status, pid2;
    long startTime;
    clock_gettime(CLOCK_REALTIME, &timeSpec);
    startTime = timeSpec.tv_nsec;

    result->pid = pid;
    result->mem = 0;
    result->memViolation = false;
    result->seccompViolation = false;
    result->timeViolation = false;
    
    // ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACESYSGOOD);

    do {
        pid2 = waitpid(pid, &status, WNOHANG);
        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
            ptrace(PTRACE_CONT, pid, NULL, SIGCONT);
            pid2 = 0;
            continue;
        }
        long rssNow = processStatus(pid, "VmHWM:");
        result->mem = std::max((result->mem), rssNow<<10);
        if (result->mem > limit.maxRss) {
            kill(pid, SIGKILL);
            result->memViolation = true;
        }
        // Check time
        clock_gettime(CLOCK_REALTIME, &timeSpec);
        result->wallTime = timeSpec.tv_nsec - startTime;
        if (result->wallTime > limit.wallTime) {
            kill(pid, SIGKILL);
            result->timeViolation = true;
        }
    } while (pid2 == 0);

    #ifdef DEBUG
    printf("STOPPED %d SIGNALED %d EXITED %d STOPSIG %d TERMSIG %d\n", WIFSTOPPED(status), WIFSIGNALED(status), WIFEXITED(status), WSTOPSIG(status), WTERMSIG(status));
    #endif

    result->message = new char[512];
    if (WIFSIGNALED(status)) {
        if (WTERMSIG(status) == SIGKILL) {
            // Killed by watchProcess; should be memory or time
            if (result->memViolation) {
                sprintf(result->message, "memory limit exceeded");
            } else if (result->timeViolation) {
                sprintf(result->message, "time limit exceeded");
            } else {
                fprintf(stderr, "PID %d: SIGKILL but not caught on result\n", pid);
                sprintf(result->message, "system error: unexpected SIGKILL");
            }
        } else if(WTERMSIG(status) == 31) {
            // Killed by seccomp; see man page
            result->seccompViolation = true;
            user_regs_struct reg;
            ptrace(PTRACE_GETREGS, pid, NULL, &reg);
            fprintf(stderr, "PID %d: seccomp violation: syscall %ld\n", pid, reg.orig_rax);
            sprintf(result->message, "seccomp violation: syscall %ld", reg.orig_rax);
        }
    } else {
        sprintf(result->message, "success");
    }

    result->exitStatus = WEXITSTATUS(status);
    return result;
}

ProcResult* runProcess(ProcArgs args, const ProcLimit limit) {
    scmp_filter_ctx scmpFilter = generateFilter(limit.secPolicy);
    fflush(stdout);
    fflush(stderr);
    
    int pid = fork();
    if (pid == -1) {
        ProcResult* result = new ProcResult();
        result->sysError = EFORK;
        return result;
    }
    if (pid != 0) {
        return watchProcess(pid, limit);
    }
    // Child process
    dup2(args.fdIn, 0);
    dup2(args.fdOut, 1);
    dup2(args.fdErr, 2);
    int traceResult = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    if (traceResult != 0) {
        printf("fatal: failed to init ptrace - error %d", traceResult);
        fflush(stdout);
        exit(-1);
    }
    prctl(PR_SET_NO_NEW_PRIVS, 1);
    int scmpResult = seccomp_load(scmpFilter);
    if (scmpResult != 0) {
        printf("fatal: failed to load seccomp filter - error %d", scmpResult);
        fflush(stdout);
        exit(-1);
    }
    execv(*args.argv, args.argv);
}
