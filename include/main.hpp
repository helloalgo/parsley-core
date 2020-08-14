#ifndef RUNNER_H_
#define RUNNER_H_

#include <cstdio>
#include <seccomp.h>
#include <vector>
#include <unistd.h>


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

    int uid;
    int gid;

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
    DUP_FAILED = 9,
    MMAP_FAILED = 10,
    SETGID_FAILED = 11
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

struct SharedError {
    RunError error;
    int proc_errno;
};

void allowCalls(scmp_filter_ctx ctx, std::vector<int> calls);
scmp_filter_ctx generateFilter(const char* key, const char* execPath);

void run_child(RunArgs args, RunResult &result);
void child_process(const RunArgs& args, SharedError* error_mem);
void watch_child(const RunArgs& args, pthread_t time_thread, RunResult &result, const SharedError* error_mem);

void set_log_debug(bool mode);
void describe(const RunResult& result);
void describe(const RunArgs& args);
char* file_str(FILE* file);

#endif
