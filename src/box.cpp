#include <cassert>
#include <cstdio>
#include <cstring>
#include <sys/resource.h>
#include <unistd.h>
#include "box.hpp"

#define BUFFER_SIZE 4096

ProcLimit::ProcLimit(uint32_t maxRss, int64_t wallTime, int64_t fileSize, const char* secPolicy) {
  this->maxRss = maxRss;
  this->wallTime = wallTime;
  this->fileSize = fileSize;
  this->secPolicy = new char[strlen(secPolicy)+1];
  strcpy(this->secPolicy, secPolicy);
}

ProcLimit::~ProcLimit() {
  delete[] this->secPolicy;
}

void ProcLimit::explain() {
  printf("ProcLimit wallTime %ld(us), mem %u(B), fileWrite %ld(B), secPolicy %s",
    this->wallTime, this->maxRss, this->fileSize, this->secPolicy);
}

ProcArgs::ProcArgs(int argc, char** argv) {
  assert(argc > 0);
  this->argc = argc;
  this->argv = new char*[this->argc+1];
  this->argv[this->argc] = NULL; // This makes execvp happy
  for (int i=0; i<this->argc; i++) {
    this->argv[i] = new char[strlen(argv[i])+1];
    strcpy(this->argv[i], argv[i]);
  }
  // Connect to std 
  this->fdIn = 0;
  this->fdOut = 1;
  this->fdErr = 2;
}

ProcArgs::~ProcArgs() {
  for (int i=0; i<this->argc; i++) {
    delete[] this->argv[i];
  }
  delete[] this->argv;
}

long processStatus(int pid, const char* key) {
  FILE *fp;
  char command[BUFFER_SIZE], buf[BUFFER_SIZE];
  long ret=-1;
  sprintf(command, "/proc/%d/status", pid);
  fp = fopen(command, "re");
  int keyLen = strlen(key);
  while (fp && fgets(buf, BUFFER_SIZE-1, fp)) {
    if (strncmp(buf, key, keyLen) == 0) {
      sscanf(buf+keyLen+1, "%ld", &ret);
    }
  }
  if (fp) {
    fclose(fp);
  }
  return ret;
}

long pageFaultMem(rusage& usage) {
  return usage.ru_minflt * getpagesize();
}

void ProcResult::log() {
  printf(
    "[ProcResult pid %d, code %d]\n  mem %ldKB, walltime %ldns\n  Violations: mem %d, time %d, seccomp %d, write %d\n  message: %s\n",
    this->pid, this->exitStatus, (this->mem)>>10, this->wallTime,
    this->memViolation, this->timeViolation, this->seccompViolation, this->fsizeViolation,
    this->message
  );
}

void ProcArgs::log() {
  printf(
    "[ProcArgs command %s, argc %d]\n", *(this->argv), this->argc
  );
}