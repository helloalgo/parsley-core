#include <cstdint>

class ProcLimit {
  public:
    uint32_t maxRss;
    int64_t wallTime;
    char* secPolicy;
    void explain();
    ProcLimit(uint32_t maxRss, int64_t wallTime, const char* secPolicy);
    ~ProcLimit();
};

class ProcResult {
  public:
    long maxRss;
    int64_t wallTime;
    bool     memViolation;
    bool     timeViolation;
    bool     seccompViolation;
    int8_t   exitStatus;
    uint8_t  sysError;
};

class ProcArgs {
  public:
    char*    command;
    uint8_t  argc;
    char**   argv;
    int      fdIn;
    int      fdOut;
    int      fdErr;
    ProcArgs(int argc, char** argv);
    ~ProcArgs();
};