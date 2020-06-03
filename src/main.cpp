#include <cstdio>
#include <cstring>
#include "cxxopts.hpp"
#include "runner.hpp"
#include "util.hpp"

int main() {
    printf("Starting parsley\n");
    char command[30] = "/src/tests/ls";
    RunArgs args;
    args.max_cpu_time = 2000;
    args.max_real_time = 2000;
    args.input = nullptr;
    args.output = fopen("test-out", "w");
    args.error = fopen("test-err", "w");
    args.args = new char*[2]{command, NULL};
    args.max_process_count = 1;
    args.max_stack_size = (1L)<<30;   // (in bytes)
    args.max_memory_size = (1L)<<30;  // (in bytes)
    args.max_write_size = 0;
    args.use_ptrace = true;
    args.seccomp_policy = "basic";
    RunResult res;
    run_child(args, res);
    describe_result(res);
}
