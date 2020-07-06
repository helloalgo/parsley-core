import json
import os
import unittest

print("Building..")
os.system("./build")
os.chdir("..")
my_path = os.path.abspath(os.path.dirname(__file__))

def build_binary(source: str):
    if source.endswith(".cpp"):
        os.system("docker run -v $(pwd):/src -w /src realcore:dev g++ /src/tests/%s -o /src/tests/%s" % (source, source[:-4]))
    else:
        os.system("docker run -v $(pwd):/src -w /src realcore:dev gcc /src/tests/%s -o /src/tests/%s" % (source, source[:-2]))

def build_everything():
    sources = filter(lambda x: x.endswith(".c"), os.listdir("./tests"))
    for source in sources:
        os.system("docker run -v $(pwd):/src -w /src realcore:dev gcc /src/tests/%s -o /src/tests/%s" % (source, source[:-2]))

def run_binary(name: str, options: dict):
    if options is None:
        options = {}
    options["stdout"] = "stdout.txt"
    options["stderr"] = "stderr.txt"
    opts_string = ' '.join([
        ("--%s %s" % (key, str(options[key]))) for key in options.keys()
    ])
    os.system("docker run --cap-add SYS_PTRACE -v $(pwd):/src -w /src -it realcore:dev /src/bin/realcore /src/tests/%s %s" % (name, opts_string))
    result = {}
    with open("result.json", "r") as f:
        result = json.load(f)
    stderr, stdout = None, None
    with open("stdout.txt", "r") as f:
        stdout = '\n'.join(f.readlines())
    with open("stderr.txt", "r") as f:
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
        f = open("stdin.txt", "w")
        f.write("5\n6 9 5 7 4")
        f.close()
    def test(self):
        self.result, self.stdout, self.stderr = run_binary("boj2493", {
            "stdin": "stdin.txt"
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
        print(self.result)