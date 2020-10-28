import json
import os
import unittest

is_ci = ('CI' in os.environ) and (os.environ['CI'] == 'true')

os.chdir('..')

if not is_ci:
    print("Building..")
    if os.system("docker build . -t realcore:dev") != 0:
        exit(1)

my_path = os.path.abspath(os.path.dirname(__file__))


def build_binary(source: str):
    if is_ci:
        if source.endswith(".cpp"):
            os.system("g++ %s -o %s" % (source, source[:-4]))
        else:
            os.system("gcc %s -o %s" % (source, source[:-2]))
    else:
        if source.endswith(".cpp"):
            os.system("docker run -v $(pwd):/tt -w /tt/test --entrypoint /src/bin/realcore realcore:dev -S none -- g++ /tt/test/%s -o /tt/test/%s" % (source, source[:-4]))
        else:
            os.system("docker run -v $(pwd):/tt -w /tt/test --entrypoint /src/bin/realcore realcore:dev -S none -- gcc /tt/test/%s -o /tt/test/%s" % (source, source[:-2]))

def run_binary(name: str, options: dict):
    if is_ci:
        return run_command('test/%s' % name, options)
    else:
        return run_command('/tt/test/%s' % name, options)

def run_command(cmd: str, options: dict):
    if options is None:
        options = {}
    options["stdout"] = "stdout.txt"
    options["stderr"] = "stderr.txt"
    opts_string = ' '.join([
        ("--%s %s" % (key, str(options[key]))) for key in options.keys()
    ])
    if is_ci:
        os.system("bin/realcore %s -- %s" % (opts_string, cmd))
    else:
        os.system("docker run --cap-add SYS_PTRACE -v $(pwd):/tt -w /tt/test -it --entrypoint /src/bin/realcore realcore:dev %s -- %s" % (opts_string, cmd))
    result = {}
    with open("test/result.json", "r") as f:
        result = json.load(f)
    stderr, stdout = None, None
    with open("test/stdout.txt", "r") as f:
        stdout = '\n'.join(f.readlines())
    with open("test/stderr.txt", "r") as f:
        stderr = '\n'.join(f.readlines())
    return result, stdout, stderr


class HelloWorldTestCase(unittest.TestCase):
    def setUp(self):
        build_binary("sample_hello.c")
        self.result, self.stdout, self.stderr = run_binary("sample_hello", None)
    def test_success(self):
        self.assertEqual(self.result["result"]["error"], 0)
        self.assertEqual(self.result["result"]["flags"]["complete"], True)
        self.assertEqual(self.result["result"]["flags"]["exited"], True)
        self.assertEqual(self.result["result"]["flags"]["signaled"], False)
        self.assertEqual(self.result["result"]["flags"]["stopped"], False)
        self.assertEqual(self.result["result"]["flags"]["stop_signal"], 0)
        self.assertEqual(self.result["result"]["flags"]["term_signal"], 0)
        self.assertEqual(self.stdout, "Hello 5")
        self.assertEqual(self.stderr, "")

class SeccompTestCase(unittest.TestCase):
    def setUp(self):
        build_binary("ls.c")
    def test_default(self):
        self.result, _, _ = run_binary("ls", None)
        self.assertEqual(self.result["result"]["error"], 0)
        self.assertEqual(self.result["result"]["flags"]["complete"], True)
        self.assertEqual(self.result["result"]["flags"]["signaled"], True)
        self.assertEqual(self.result["result"]["violation"], 3)
    def test_unlimited(self):
        self.result, _, _ = run_binary("ls", {"seccomp": "none"})
        self.assertEqual(self.result["result"]["error"], 0)
        self.assertEqual(self.result["result"]["flags"]["complete"], True)
        self.assertEqual(self.result["result"]["violation"], 0)

class TimeTestCase(unittest.TestCase):
    def setUp(self):
        build_binary("sleep.c")
    def test(self):
        self.result, _, _ = run_binary("sleep", None)
        self.assertEqual(self.result["result"]["error"], 0)


class BOJ2493Case(unittest.TestCase):
    def setUp(self):
        build_binary("boj2493.cpp")
        f = open("test/stdin.txt", "w")
        f.write("5\n6 9 5 7 4")
        f.close()
    def test(self):
        self.result, self.stdout, self.stderr = run_binary("boj2493", {
            "stdin": "/tt/test/stdin.txt",
        })
        self.assertEqual(self.result["result"]["error"], 0)
        self.assertEqual(self.result["result"]["flags"]["complete"], True)
        self.assertEqual(self.result["result"]["violation"], 0)
        self.assertEqual(self.stdout.strip(), "0 0 2 2 4")

class MemoryTestCase(unittest.TestCase):
    def setUp(self):
        build_binary("memory.c")
    def test(self):
        self.result, _, _ = run_binary("memory", {
            "mem": 32*1024
        })


class CompileTestCase(unittest.TestCase):
    def test(self):
        file = "test/sample_hello.c" if is_ci else "/tt/test/sample_hello.c"
        self.result, self.out, self.err = run_command("gcc %s -o /tt/test/sample_build" % file, {
            "mem": 32*1024,
            "write": 100000,
            "seccomp": "none"
        })
        self.assertEqual(self.result["result"]["error"], 0)
        self.assertEqual(self.result["result"]["flags"]["complete"], True)
        self.assertEqual(self.result["result"]["violation"], 0)
        self.exec_result, self.out, self.err = run_binary("sample_build", {
            "mem": 32*1024
        })
        self.assertEqual(self.out, "Hello 5")


class RunInfTestCase(unittest.TestCase):
    def setUp(self):
        build_binary("run-inf.c")
    def test(self):
        self.result, self.stdout, self.stderr = run_binary("run-inf", {
            "realtime": "2000"
        })
        self.assertEqual(self.result["result"]["wall_time"], 2000)
        self.assertEqual(self.result["result"]["violation"], 2)
        self.assertEqual(self.result["result"]["flags"]["complete"], True)
        self.assertEqual(self.result["result"]["flags"]["exited"], False)
        self.assertEqual(self.result["result"]["flags"]["signaled"], True)


class PythonTestCase(unittest.TestCase):
    def setUp(self):
        pass
    def test(self):
        self.result, self.stdout, self.stderr = run_command("python3 /tt/test/simple.py", {
            "nproc": "100",
            "seccomp": "none"
        })
        self.assertEqual(self.result["result"]["error"], 0)
        self.assertEqual(self.result["result"]["flags"]["complete"], True)
        self.assertEqual(self.result["result"]["violation"], 0)
        self.assertEqual(self.stdout, 'hi\n')
