#include <cstdio>
#include <seccomp.h>
#include <vector>
#include <unistd.h>

#ifndef RUNNER_H_
#define RUNNER_H_

struct RunArgs {
    int max_cpu_time; // ms
    int max_real_time; // ms
    long max_process_count;
    long max_write_size; // 
    long max_memory_size; // KB
    long max_stack_size;  // KB

    char* seccomp_policy;
    bool use_ptrace;
    char** args;

    FILE* input;
    FILE* output;
    FILE* error;
    FILE* logPath;
};

enum RunViolation {
    NONE = 0,
    MEMORY = 1,
    TIME = 2,
    SECCOMP = 3,
    WRITE_SIZE = 4,
    RUNTIME = 5
};

enum RunError {
    SUCCESS = 0,
    SECCOMP_INVALID = 1,
    SECCOMP_LOAD_FAILED = 2,
    FORK_FAILED = 3,
    WAIT_FAILED = 4,
    PTRACE_FAILED = 5,
    SETRLIMIT_FAILED = 6,
    SETUID_FAILED = 7,
    EXEC_FAILED = 8,
    DUP_FAILED = 9
};

struct RunResult {
    bool complete;

    RunError error;
    RunViolation violation;
    char* message;
    int cpu_time;
    int real_time;
    long memory;
    int exit_code;
    pid_t pid;
    // Process results
    bool stopped;
    bool signaled;
    bool exited;
    int stop_signal;
    int term_signal;
};

void allowCalls(scmp_filter_ctx ctx, std::vector<int> calls);
scmp_filter_ctx generateFilter(const char* key);

void run_child(RunArgs args, RunResult &result);
void child_process(const RunArgs& args);
void watch_child(const RunArgs& args, RunResult &result);

#endif
