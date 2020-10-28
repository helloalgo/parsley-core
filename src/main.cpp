#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include "main.hpp"
#include "cxxopts.hpp"
#include "json.hpp"
using json = nlohmann::json;

ChildError* shared_error;
// char* args[] = {"g++", "/src/test_src.cpp", "-o", "output", NULL};
char* args[] = {"sleep", "10", NULL};

struct timeout_killer_args {
    pid_t pid;
    long long limit_nsec;
    bool exited = false;
};

void handle_interrupt(int signo) {
    if (signo == SIGINT) exit(11);
}

void* timeout_killer(timeout_killer_args* args) {
    struct sigaction act;
    sigset_t mask;
    act.sa_handler = handle_interrupt;
    sigaction(SIGINT, &act, NULL);
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    timespec time;
    time.tv_nsec = args->limit_nsec % 1000000000LL;
    time.tv_sec = args->limit_nsec / 1000000000LL;
    if (nanosleep(&time, NULL) != 0) {
        printf("[ERROR] Nanosleep failed: %s", strerror(errno));
        kill(args->pid, SIGKILL);
        args->exited = true;
        return NULL;
    }
    args->exited = true;
    kill(args->pid, SIGKILL);
    return NULL;
}


// Run child and parent

void exitWithError(RunError error) {
    shared_error->error = error;
    shared_error->proc_errno = errno;
    fflush(stdout);
    fflush(stderr);
    shared_error->has_error = true;
    exit(1);
}
void safe_setrlimit(int type, const rlimit &limit) {
    if (setrlimit(type, &limit) != 0) {
        exitWithError(RunError::SETRLIMIT_FAILED);
    }
}
void safe_dup2(int src, int dst) {
    if (dup2(src, dst) == -1) {
        printf("%s", strerror(errno));
        exitWithError(RunError::DUP_FAILED);
    }
}

void child_process(const RunArgs& args, ChildError* error_mem) {
    shared_error = error_mem;
    auto scmpFilter = generateFilter(args.seccomp_policy, *(args.args));
    if (args.max_stack_size > 0) {
        rlimit lim;
        lim.rlim_cur = lim.rlim_max = (rlim_t)(args.max_stack_size<<10);
        safe_setrlimit(RLIMIT_STACK, lim);
    }
    if (args.max_memory_size > 0) {
        rlimit lim;
        lim.rlim_cur = lim.rlim_max = (rlim_t)(args.max_memory_size<<11);
        safe_setrlimit(RLIMIT_AS, lim);
    }
    if (args.max_process_count > 0) {
        rlimit lim;
        lim.rlim_cur = lim.rlim_max = (rlim_t)args.max_process_count;
        safe_setrlimit(RLIMIT_NPROC, lim);
    }
    if (args.max_write_size > 0) {
        rlimit lim;
        lim.rlim_cur = lim.rlim_max = (rlim_t)args.max_write_size;
        safe_setrlimit(RLIMIT_FSIZE, lim);
    }
    if (args.max_cpu_time > 0) {
        rlimit lim;
        lim.rlim_cur = lim.rlim_max = (rlim_t)((args.max_cpu_time + 1000)/1000);
        safe_setrlimit(RLIMIT_CPU, lim);
    }
    if (args.uid != -1) {
        if (setuid(args.uid) != 0) {
            exitWithError(RunError::SETUID_FAILED);
        }
        printf("SetUID %d\n", args.uid);
    }
    if (args.gid != -1) {
        if (setgid(args.gid) != 0) {
            exitWithError(RunError::SETGID_FAILED);
        }
        printf("SetGID %d\n", args.gid);
    }
    if (args.input != nullptr) {
        safe_dup2(fileno(args.input), STDIN_FILENO);
    }
    if (args.output != nullptr) {
        safe_dup2(fileno(args.output), STDOUT_FILENO);
    }
    if (args.error != nullptr) {
        safe_dup2(fileno(args.error), STDERR_FILENO);
    }
    prctl(PR_SET_NO_NEW_PRIVS, 1);
    if (seccomp_load(scmpFilter) != 0) {
        exitWithError(RunError::SECCOMP_LOAD_FAILED);
    }
    execvp(*args.args, args.args);
    exitWithError(RunError::EXEC_FAILED);
}

RunResult* watch_child(pid_t pid, pthread_t time_thread, const RunArgs& args, const ChildError* error_mem, const timeout_killer_args* time_mem) {
    auto startTime = std::chrono::steady_clock::now();
    RunResult* result = new RunResult();
    result->message = new char[BUF_SIZE];
    result->pid = pid;
    result->complete = false;
    rusage usage;
    int status;
    pid_t waitResult = wait4(pid, &status, 0, &usage);
    if (waitResult == -1) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        result->error = RunError::WAIT_FAILED;
        snprintf(result->message, BUF_SIZE, "Error: WAIT_FAILED: %s", strerror(errno));
        return result;
    }
    auto endTime = std::chrono::steady_clock::now();
    if (!time_mem->exited) {
        pthread_kill(time_thread, SIGINT);
    }
    if (WIFSTOPPED(status) != 0) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
    result->complete = true;
    result->real_time = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    result->cpu_time = usage.ru_utime.tv_sec*1000 + usage.ru_utime.tv_usec/1000;
    result->memory = usage.ru_maxrss;
    result->stopped = WIFSTOPPED(status) != 0;
    result->signaled = WIFSIGNALED(status) != 0;
    result->exited = WIFEXITED(status) != 0;
    result->stop_signal = (WIFSTOPPED(status) != 0) ? WSTOPSIG(status) : 0;
    result->term_signal = (WIFSIGNALED(status) != 0) ? WTERMSIG(status) : 0;
    result->exit_code = (WIFEXITED(status) != 0) ? WEXITSTATUS(status) : 0;
    if (error_mem->has_error) {
        result->error = (RunError)result->exit_code;
        snprintf(result->message, BUF_SIZE, "Error from child process: RunError code %d, errno: %s", error_mem->error, strerror(error_mem->proc_errno));
        kill(pid, SIGKILL);
    }
    verdict_run_result(*result, args);
    return result;
}

// 종료 시그널 핸들러에서 사용하기 위한 child process pid
// 반드시 (1) fork 직후 pid를 설정하고 (2) 이후에 수정하지 말아야 합니다!!!
pid_t child_pid = -1;

void sigterm_handler(int signo) {
    if (child_pid != -1) {
        kill(child_pid, SIGKILL);
        waitpid(child_pid, NULL, 0);
    }
    exit(1);
}

RunResult* run(RunArgs args) {
  // Create shared memory
    auto error_mem = (ChildError*)mmap(0, sizeof(ChildError), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if ((long long)error_mem == -1) {
        RunResult* result = new RunResult();
        result->error = RunError::MMAP_FAILED;
        snprintf(result->message, BUF_SIZE, "Error: MMAP_FAILED: %s", strerror(errno));
        return result;
    }
    pid_t pid = fork();
    fflush(stdout);
    if (pid == 0) {
        child_process(args, error_mem);
        return NULL;
    }
    if (pid < 0) {
        RunResult* result = new RunResult();
        result->error = RunError::FORK_FAILED;
        snprintf(result->message, BUF_SIZE, "Error: FORK_FAILED: %s", strerror(errno));
        return result;
    }
    // Set signal handler
    child_pid = pid;
    if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
        RunResult* result = new RunResult();
        result->error = RunError::SIGNAL_FAILED;
        snprintf(result->message, BUF_SIZE, "Error: SIGNAL_FAILED");
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return result;
    }
    write_log("Forked: %d\n", pid);
    pthread_t tid = 0;
    timeout_killer_args killer_args;
    killer_args.pid = pid;
    killer_args.limit_nsec = args.max_real_time*1000000LL;
    if (pthread_create(&tid, NULL, (void*(*)(void*))timeout_killer, (void*)(&killer_args)) != 0) {
        RunResult* result = new RunResult();
        result->error = RunError::FORK_FAILED;
        snprintf(result->message, BUF_SIZE, "Error: PTHREAD_FORK_FAILED: %s", strerror(errno));
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        munmap(error_mem, sizeof(ChildError));
        return result;
    }
    RunResult* result = watch_child(pid, tid, args, error_mem, &killer_args);
    munmap(error_mem, sizeof(ChildError));
    // Ensure child process has exited
    waitpid(pid, NULL, 0);
    return result;
}

int main(int argc, char** argv) {
    cxxopts::Options options("parsley-run", "Run a binary with resource limitations");
    options.add_options()
        ("c,cputime", "Maximum CPU time in milliseconds", cxxopts::value<int>()->default_value("1000"))
        ("r,realtime", "Maximum real time in milliseconds", cxxopts::value<int>()->default_value("1000"))
        ("p,nproc", "Maximum process count", cxxopts::value<int>()->default_value("1"))
        ("s,stack", "Maximum stack size in kilobytes", cxxopts::value<long>()->default_value("131072"))
        ("m,mem", "Maximum memory in kilobytes", cxxopts::value<long>()->default_value("131072"))
        ("w,write", "Maximum write size in kilobytes", cxxopts::value<long>()->default_value("0"))
        ("P,ptrace", "Use ptrace", cxxopts::value<bool>()->default_value("true"))
        ("S,seccomp", "Seccomp policy string", cxxopts::value<std::string>()->default_value("basic"))
        ("I,stdin", "Path to read stdin of child", cxxopts::value<std::string>())
        ("O,stdout", "Path to write stdout of child", cxxopts::value<std::string>())
        ("E,stderr", "Path to write stderr of child", cxxopts::value<std::string>())
        ("D,debug", "Output debug information to stdout", cxxopts::value<bool>()->default_value("false"))
        ("uid", "UID for running the binary", cxxopts::value<int>())
        ("gid", "GID for running the binary", cxxopts::value<int>())
        ("h,help", "Print usage")
        ("args", "Command arguments", cxxopts::value<std::vector<std::string>>())
    ;
    options.parse_positional({"args"});
    int original_argc = argc;
    auto result = options.parse(argc, argv);
    if (result.count("help") || original_argc == 1) {
        printf("%s\n", options.help().c_str());
        exit(0);
    }
    set_log_debug(result["debug"].as<bool>());

    RunArgs args;
    if (result.count("stdin")) {
        args.input = fopen(result["stdin"].as<std::string>().c_str(), "r");
    } else {
        args.input = nullptr;
    }
    if (result.count("stdout")) {
        args.output = fopen(result["stdout"].as<std::string>().c_str(), "w");
    } else {
        args.output = nullptr;
    }
    if (result.count("stderr")) {
        args.error = fopen(result["stderr"].as<std::string>().c_str(), "w");
    } else {
        args.error = nullptr;
    }
    auto command_args = result["args"].as<std::vector<std::string>>();
    args.args = new char*[command_args.size()+1];
    for (int i=0; i<(int)command_args.size(); i++) {
        args.args[i] = (char*)command_args[i].c_str();
    }
    // Needed by exec (see man page execvp(3))
    args.args[command_args.size()] = NULL;
    args.max_cpu_time = result["cputime"].as<int>();
    args.max_real_time = result["realtime"].as<int>();
    args.max_process_count = result["nproc"].as<int>();
    args.max_stack_size = result["stack"].as<long>();
    args.max_memory_size = result["mem"].as<long>();
    args.max_write_size = result["write"].as<long>();
    args.use_ptrace = result["ptrace"].as<bool>();
    args.seccomp_policy = (char*)result["seccomp"].as<std::string>().c_str();
    if (result.count("uid") > 0) {
        args.uid = result["uid"].as<int>();
    } else {
        args.uid = -1;
    }
    if (result.count("gid") > 0) {
        args.gid = result["gid"].as<int>();
    } else {
        args.gid = -1;
    }
    describe(args);
    RunResult* res = run(args);
    describe(*res);

    try {
        char cwd[1025];
        getcwd(cwd, sizeof(cwd));
        json resultJson = {
            {"result", {
                {"pid", res->pid},
                {"message", res->message},
                {"flags", {
                    {"complete", res->complete},
                    {"signaled", res->signaled},
                    {"exited", res->exited},
                    {"stopped", res->stopped},
                    {"stop_signal", res->stop_signal},
                    {"term_signal", res->term_signal},
                }},
                {"error", res->error},
                {"exit_code", res->exit_code},
                {"violation", res->violation},
                {"cpu_time", res->cpu_time},
                {"wall_time", res->real_time},
                {"memory", res->memory}
            }},
            {"args", {
                {"command", command_args},
                {"seccomp_policy", args.seccomp_policy},
                {"files", {
                    {"input", file_str(args.input)},
                    {"output", file_str(args.output)},
                    {"error", file_str(args.error)}
                }},
                {"max_cpu_time", args.max_cpu_time},
                {"max_real_time", args.max_real_time},
                {"max_process_count", args.max_process_count},
                {"max_memory", args.max_memory_size},
                {"max_stack", args.max_stack_size},
                {"max_write", args.max_write_size}
            }},
            {"env", {
                {"workdir", cwd}
            }}
        };
        std::ofstream o("result.json");
        o << std::setw(4) << resultJson << std::endl;
        o.close();
    } catch (json::exception& e) {
        write_log(e.what());
    }
}
