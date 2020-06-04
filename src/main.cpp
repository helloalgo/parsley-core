#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "cxxopts.hpp"
#include "runner.hpp"
#include "util.hpp"

int main(int argc, char** argv) {
    cxxopts::Options options("parsley-run", "Run a binary with resource limitations");
    options.add_options()
        ("c,cputime", "Maximum CPU time in milliseconds", cxxopts::value<int>()->default_value("1000"))
        ("r,realtime", "Maximum real time in milliseconds", cxxopts::value<int>()->default_value("1000"))
        ("p,nproc", "Maximum process count", cxxopts::value<int>()->default_value("1"))
        ("s,stack", "Maximum stack size in kilobytes", cxxopts::value<long>()->default_value("131072"))
        ("m,mem", "Maximum memory in kilobytes", cxxopts::value<long>()->default_value("131072"))
        ("w,write", "Maximum write size in kilobytes", cxxopts::value<long>()->default_value("0"))
        ("ptrace", "Use ptrace", cxxopts::value<bool>()->default_value("true"))
        ("seccomp", "Seccomp policy string", cxxopts::value<std::string>()->default_value("basic"))
        ("h,help", "Print usage")
        ("args", "Command arguments", cxxopts::value<std::vector<std::string>>())
        ("stdin", "Path to read stdin of child", cxxopts::value<std::string>())
        ("stdout", "Path to write stdout of child", cxxopts::value<std::string>())
        ("stderr", "Path to write stderr of child", cxxopts::value<std::string>())
    ;
    options.parse_positional({"args"});
    int original_argc = argc;
    auto result = options.parse(argc, argv);
    if (result.count("help") || original_argc == 1) {
        printf("%s\n", options.help().c_str());
        exit(0);
    }
    RunArgs args;
    if (result.count("stdin")) {
        args.input = fopen(result["stdin"].as<std::string>().c_str(), "w");
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
    args.args = new char*[command_args.size()];
    for (int i=0; i<(int)command_args.size(); i++) {
        args.args[i] = (char*)command_args[i].c_str();
    }
    args.max_cpu_time = result["cputime"].as<int>();
    args.max_real_time = result["realtime"].as<int>();
    args.max_process_count = result["nproc"].as<int>();
    args.max_stack_size = result["stack"].as<long>();
    args.max_memory_size = result["mem"].as<long>();
    args.max_write_size = result["write"].as<long>();
    args.use_ptrace = result["ptrace"].as<bool>();
    args.seccomp_policy = (char*)result["seccomp"].as<std::string>().c_str();
    RunResult res;
    run_child(args, res);
    describe_result(res);
}
