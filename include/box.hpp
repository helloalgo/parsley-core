#include <cstdint>
#include <sys/resource.h>

#define EFORK 1

class ProcLimit {
  public:
    uint32_t maxRss;
    int64_t wallTime;
    int64_t fileSize;
    char* secPolicy;
    void explain();
    ProcLimit::ProcLimit(uint32_t maxRss, int64_t wallTime, int64_t fileSize, const char* secPolicy);
    ~ProcLimit();
};

class ProcResult {
  public:
    long     mem;
    int64_t  wallTime;
    bool     memViolation;
    bool     timeViolation;
    bool     seccompViolation;
    bool     fsizeViolation;
    int8_t   exitStatus;
    uint8_t  sysError;
    uint16_t pid;
    char*    message;
    void log();
};

class ProcArgs {
  public:
    uint8_t  argc;
    char**   argv;
    int      fdIn;
    int      fdOut;
    int      fdErr;
    ProcArgs(int argc, char** argv);
    ~ProcArgs();
    void log();
};

ProcResult* watchProcess(ProcLimit limit);
ProcResult* runProcess(ProcArgs args, ProcLimit limit);

// Read a row with the column `key` from /proc/(pid)/status.
// Returns -1 if not found. 
long processStatus(int pid, const char* key);
long pageFaultMem(rusage& usage);
