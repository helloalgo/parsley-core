#include <cassert>
#include <cstdio>
#include <cstring>
#include "box.hpp"

ProcLimit::ProcLimit(uint32_t maxRss, int64_t wallTime, const char* secPolicy) {
  this->maxRss = maxRss;
  this->wallTime = wallTime;
  this->secPolicy = new char[strlen(secPolicy)+1];
  strcpy(this->secPolicy, secPolicy);
}

ProcLimit::~ProcLimit() {
  delete[] this->secPolicy;
}

void ProcLimit::explain() {
  printf("ProcLimit wallTime %ld(us), maxRss %d(b), secPolicy %s",
    this->wallTime, this->maxRss, this->secPolicy);
}

ProcArgs::ProcArgs(int argc, char** argv) {
  assert(argc > 0);
  this->argc = argc - 1;
  this->command = new char[strlen(*argv)+1];
  this->argv = new char*[this->argc];
  for (int i=0; i<this->argc; i++) {
    this->argv[i] = new char[strlen(argv[i+1])+1];
    strcpy(this->argv[i], argv[i+1]);
  }
  // Connect to std 
  this->fdIn = 0;
  this->fdOut = 1;
  this->fdErr = 2;
}

ProcArgs::~ProcArgs() {
  delete[] this->command;
  for (int i=0; i<this->argc; i++) {
    delete[] this->argv[i];
  }
  delete[] this->argv;
}
