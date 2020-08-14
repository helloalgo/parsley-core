#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include "main.hpp"

const int timeLimitMillis = 5000;
// char* args[] = {"g++", "/src/test_src.cpp", "-o", "output", NULL};
char* args[] = {"sleep", "10", NULL};

// Time watcher

struct timeout_killer_args {
    pid_t pid;
    long long limit_nsec;
};

void* timeout_killer(timeout_killer_args* args) {
    timespec time;
    time.tv_nsec = args->limit_nsec % 1000000000LL;
    time.tv_sec = args->limit_nsec / 1000000000LL;
    if (nanosleep(&time, NULL) != 0) {
        printf("[ERROR] Nanosleep failed: %d", errno);
        kill(args->pid, SIGKILL);
        return NULL;
    }
    kill(args->pid, SIGKILL);
    return NULL;
}

// Run child and parent

void child_process() {
  execvp(*args, args);
}

int main() {
  pid_t pid = fork();
  if (pid == 0) {
    child_process();
    return 0;
  }
  printf("I'm a parent with child pid %d\n", pid);

  pthread_t tid = 0;
  timeout_killer_args killer_args;
  killer_args.pid = pid;
  killer_args.limit_nsec = timeLimitMillis*1000000LL;
  pthread_create(&tid, NULL, (void*(*)(void*))timeout_killer, (void*)(&killer_args));

  int status;
  rusage usage;
  wait4(pid, &status, 0, &usage);
  if (WIFEXITED(status)) {
    printf("Child exited, returning %d", WEXITSTATUS(status));
  }
  else if (WIFSIGNALED(status)) {
    printf("Child signaled, with signal %d", WTERMSIG(status));
  }
  pthread_kill(tid, SIGKILL);
}