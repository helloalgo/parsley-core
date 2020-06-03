#include "util.hpp"

void describe_result(const RunResult& res) {
    printf("[RunResult %p]\n", &res);
    printf("PID %d, run result from runner: %s\n", res.pid, res.message);
    printf("[Flags]\n");
    printf("Complete%3d Signaled%3d    Exited%3d Stopped%3d Stop Sig%3d\n", res.complete, res.signaled, res.exited, res.stopped, res.stop_signal);
    printf("Term Sig%3d    Error%3d      Exit%3d   Viol.%3d\n", res.term_signal, res.error, res.exit_code, res.violation);
    printf("[Metrics]\n");
    printf("CPU time %d\n", res.cpu_time);
    printf("Real(wall) time %d\n", res.real_time);
    printf("Memory %ld\n", res.memory);
}
