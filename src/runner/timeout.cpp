#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "timeout.hpp"

void timeout_killer(timeout_killer_args args) {
    timespec time;
    time.tv_nsec = args.limit_nsec % 1000000000;
    time.tv_sec = args.limit_nsec / 1000000000;
    if (nanosleep(&time, NULL) != 0) {
        kill(args.pid, SIGKILL);
        return;
    }
    kill(args.pid, SIGKILL);
}
