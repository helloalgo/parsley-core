#include <cstdarg>
#include <cstdlib>
#include <string>
#include <signal.h>
#include "main.hpp"

bool log_debug = false;
FILE* log_output = fopen("parsley_runner.log", "w");

void set_log_debug(bool mode) {
    if (!log_debug && mode) {
        log_output = fopen("parsley_runner.log", "w");
    } else if (log_debug && !mode) {
        fclose(log_output);
    }
    log_debug = mode;
}

char* file_str(FILE* file) {
    return (char*)(file == nullptr ? "default" : "given");
}

void describe(const RunResult& res) {
    fprintf(log_debug ? stdout : log_output, "[RunResult %p]\n", &res);
    fprintf(log_debug ? stdout : log_output, "PID %d, run result from runner: %s\n", res.pid, res.message);
    fprintf(log_debug ? stdout : log_output, "[Flags]\n");
    fprintf(log_debug ? stdout : log_output, "Complete%3d Signaled%3d    Exited%3d Stopped%3d Stop Sig%3d\n", res.complete, res.signaled, res.exited, res.stopped, res.stop_signal);
    fprintf(log_debug ? stdout : log_output, "Term Sig%3d    Error%3d      Exit%3d   Viol.%3d\n", res.term_signal, res.error, res.exit_code, res.violation);
    fprintf(log_debug ? stdout : log_output, "[Metrics]\n");
    fprintf(log_debug ? stdout : log_output, "CPU time %d\n", res.cpu_time);
    fprintf(log_debug ? stdout : log_output, "Real(wall) time %d\n", res.real_time);
    fprintf(log_debug ? stdout : log_output, "Memory %ld\n", res.memory);
}

void describe(const RunArgs& args) {
    fprintf(log_debug ? stdout : log_output, "[RunArgs %p]\n", &args);
    fprintf(log_debug ? stdout : log_output, "ptrace %3d seccomp %s\n", args.use_ptrace, args.seccomp_policy);
    fprintf(log_debug ? stdout : log_output, "stdin %s / stdout %s / stderr %s\n", file_str(args.input), file_str(args.output), file_str(args.error));
    fprintf(log_debug ? stdout : log_output, "cputime %dms realtime %dms\n", args.max_cpu_time, args.max_real_time);
    fprintf(log_debug ? stdout : log_output, "nproc %ld mem %ldkb stack %ldkb write %ldkb\n", args.max_process_count, args.max_memory_size, args.max_stack_size, args.max_write_size);
}

void verdict_run_result(RunResult &result, const RunArgs &args) {
    result.complete = true;
    if (result.exit_code != 0) {
        result.violation = RunViolation::RUNTIME;
        snprintf(result.message, BUF_SIZE, "Runtime Error (Exit code %d)", result.exit_code);
        return;
    }
    if (result.term_signal == SIGKILL) {
        result.violation = RunViolation::TIME;
        result.real_time = args.max_real_time;
        snprintf(result.message, BUF_SIZE, "Time Limit Exceeded");
        return;
    }
    if (result.term_signal == SIGSEGV) {
        if (args.max_memory_size > 0 && result.memory > args.max_memory_size) {
            result.violation = RunViolation::MEMORY;
            snprintf(result.message, BUF_SIZE, "Memory Limit Exceeded");
        } else {
            result.violation = RunViolation::RUNTIME;
            snprintf(result.message, BUF_SIZE, "Runtime Error (SIGSEGV)");
        }
        return;
    }
    if (result.term_signal == 31) {
        // Killed by seccomp; see man page
        result.violation = RunViolation::SECCOMP;
        snprintf(result.message, BUF_SIZE, "seccomp violation");
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
    if (args.max_real_time > 0 && result.real_time > args.max_real_time) {
        result.violation = RunViolation::TIME;
        result.real_time = args.max_real_time;
        snprintf(result.message, BUF_SIZE, "Time Limit Exceeded");
        return;
    }
    if (args.max_cpu_time > 0 && result.cpu_time > args.max_cpu_time) {
        result.violation = RunViolation::TIME;
        snprintf(result.message, BUF_SIZE, "Time Limit Exceeded");
        return;
    }
    // Passsed everything!
    result.violation = RunViolation::NONE;
    snprintf(result.message, BUF_SIZE, "Success");
}
