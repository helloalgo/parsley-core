#include <cstdio>
#include <cstring>
#include <chrono>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/user.h>
#include <unistd.h>
#include "runner.hpp"
#include "timeout.hpp"

#define BUF_SIZE 512

void watch_child(pid_t pid, const RunArgs& args, RunResult &result) {
    auto startTime = std::chrono::steady_clock::now();
    if (args.use_ptrace) {
        ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXIT);
    }
    rusage usage;
    int status;
    while (true) {
        pid_t waitResult = wait4(pid, &status, WSTOPPED, &usage);
        if (waitResult == -1) {
            kill(pid, SIGKILL);
            result.error = RunError::WAIT_FAILED;
            snprintf(result.message, BUF_SIZE, "Error: WAIT_FAILED: %s", strerror(errno));
            return;
        }
        if (args.use_ptrace && WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
            ptrace(PTRACE_CONT, pid, NULL, SIGCONT);
            continue;
        }
        break;
    }
    auto endTime = std::chrono::steady_clock::now();
    result.real_time = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    result.cpu_time = usage.ru_utime.tv_sec*1000 + usage.ru_utime.tv_usec/1000;
    result.memory = usage.ru_maxrss;
    result.stopped = WIFSTOPPED(status) != 0;
    result.signaled = WIFSIGNALED(status) != 0;
    result.exited = WIFEXITED(status) != 0;
    result.stop_signal = (WIFSTOPPED(status) != 0) ? WSTOPSIG(status) : 0;
    result.term_signal = (WIFSIGNALED(status) != 0) ? WTERMSIG(status) : 0;
    result.exit_code = (WIFEXITED(status) != 0) ? WEXITSTATUS(status) : 0;
    if (result.term_signal == SIGUSR1) {
        result.error = (RunError)result.exit_code;
        snprintf(result.message, BUF_SIZE, "Error: RunError %d", result.exit_code);
        return;
    }
    // Not our fault now
    result.complete = true;
    if (result.exit_code != 0) {
        result.violation = RunViolation::RUNTIME;
        snprintf(result.message, BUF_SIZE, "Runtime Error (Exit code %d)", result.exit_code);
        return;
    }
    if (result.term_signal == SIGSEGV) {
        if (args.max_memory_size > 0 && result.memory > args.max_memory_size) {
            result.violation = RunViolation::MEMORY;
            snprintf("result.message", BUF_SIZE, "Memory Limit Exceeded");
        } else {
            result.violation = RunViolation::RUNTIME;
            snprintf(result.message, BUF_SIZE, "Runtime Error (SIGSEGV)");
        }
        return;
    }
    if (result.term_signal == 31) {
        // Killed by seccomp; see man page
        result.violation = RunViolation::SECCOMP;
        if (args.use_ptrace) {
            user_regs_struct reg;
            ptrace(PTRACE_GETREGS, pid, NULL, &reg);
            snprintf(result.message, BUF_SIZE, "seccomp violation: syscall %ld", reg.orig_rax);
        } else {
            snprintf(result.message, BUF_SIZE, "seccomp violation");
        }
        return;
    }
    if (result.term_signal == SIGXFSZ && args.max_write_size > 0) {
        result.violation = RunViolation::WRITE_SIZE;
        snprintf(result.message, BUF_SIZE, "Output Limit Exceeded");
        return;
    }
    if (args.max_memory_size > 0 && result.memory > args.max_memory_size) {
        result.violation = RunViolation::MEMORY;
        snprintf(result.message, BUF_SIZE, "Memory Limit Exceeded");
        return;
    }
    if (args.max_cpu_time > 0 && result.cpu_time > args.max_cpu_time) {
        result.violation = RunViolation::TIME;
        snprintf(result.message, BUF_SIZE, "Time Limit Exceeded");
        return;
    }
    if (args.max_real_time > 0 && result.real_time > args.max_real_time) {
        result.violation = RunViolation::TIME;
        snprintf(result.message, BUF_SIZE, "Time Limit Exceeded");
        return;
    }
    // Passsed everything!
    result.violation = RunViolation::NONE;
    snprintf(result.message, BUF_SIZE, "Success");
}

void run_child(RunArgs args, RunResult &result) {
    pid_t pid = fork();
    result.complete = false;
    result.violation = RunViolation::NONE;
    result.error = RunError::SUCCESS;
    result.message = new char[BUF_SIZE];
    if (pid < 0) {
        result.error = RunError::FORK_FAILED;
        snprintf(result.message, BUF_SIZE, "Error: FORK_FAILED: %s", strerror(errno));
        return;
    }
    result.pid = pid;
    if (pid == 0) {
        child_process(args);
        return;
    }
    pthread_t tid = 0;
    timeout_killer_args killer_args;
    killer_args.pid = pid;
    killer_args.limit_nsec = args.max_real_time*1000000;
    if (pthread_create(&tid, NULL, (void*(*)(void*))timeout_killer, (void*)(&killer_args)) != 0) {
        result.error = RunError::FORK_FAILED;
        snprintf(result.message, BUF_SIZE, "Error: PTHREAD_FORK_FAILED: %s", strerror(errno));
        return;
    }
    watch_child(pid, args, result);
}
