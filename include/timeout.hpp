#include <unistd.h>

struct timeout_killer_args {
    pid_t pid;
    long limit_nsec;
};

void timeout_killer(timeout_killer_args args);
