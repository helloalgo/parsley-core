#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include "cxxopts.hpp"
#include "runner.hpp"
#include "json.hpp"
#include "util.hpp"
using json = nlohmann::json;

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
    describe(args);
    RunResult res;
    run_child(args, res);
    describe(res);
    json resultJson = {
        {"result", {
            {"pid", res.pid},
            {"message", res.message},
            {"flags", {
                {"complete", res.complete},
                {"signaled", res.signaled},
                {"exited", res.exited},
                {"stopped", res.stopped},
                {"stop_signal", res.stop_signal},
                {"term_signal", res.term_signal},
            }},
            {"error", res.error},
            {"exit_code", res.exit_code},
            {"violation", res.violation},
            {"cpu_time", res.cpu_time},
            {"wall_time", res.real_time},
            {"memory", res.memory}
        }},
        {"args", {
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
        }}
    };
    std::ofstream o("result.json");
    o << std::setw(4) << resultJson << std::endl;
    o.close();
}
