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
    result->maxRss = 0;
    result->memViolation = false;
    result->seccompViolation = false;
    result->timeViolation = false;
    
    do {
        pid2 = waitpid(pid, &status, WNOHANG);
        // Check memory
        getrusage(RUSAGE_CHILDREN, &usage);
        result->maxRss = std::max(result->maxRss, usage.ru_maxrss);
        if (result->maxRss > limit.maxRss) {
            kill(pid, SIGKILL);
            result->memViolation = true;
            break;
        }
        // Check time
        clock_gettime(CLOCK_REALTIME, &timeSpec);
        result->wallTime = timeSpec.tv_nsec - startTime;
        if (result->wallTime > limit.wallTime) {
            kill(pid, SIGKILL);
            result->timeViolation = true;
            break;
        }
    } while (pid2 == 0);
    waitpid(pid, &status, WNOHANG);
    if (!WIFEXITED(status)) {
        if (!(result->memViolation || result->timeViolation)) {
            result->seccompViolation = true;
        }
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
    close(args.fdIn);
    close(args.fdOut);
    close(args.fdErr);
    execvp(*args.argv, args.argv);
}
