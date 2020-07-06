import json
import os
import unittest

print("Building..")
os.system("./build")
os.chdir("..")
os.system("pwd")
my_path = os.path.abspath(os.path.dirname(__file__))

def build_binary(source: str):
    os.system("docker run -v $(pwd):/src -w /src realcore:dev gcc /src/tests/%s -o /src/tests/%s" % (source, source[:-2]))

def build_everything():
    sources = filter(lambda x: x.endswith(".c"), os.listdir("./tests"))
    for source in sources:
        os.system("docker run -v $(pwd):/src -w /src realcore:dev gcc /src/tests/%s -o /src/tests/%s" % (source, source[:-2]))

def run_binary(name: str, options: dict):
    opts_string = ""
    if options is not None:
        for key in options.keys():
            opts_string += "--%s %s" % (key, str(options[key]))
    os.system("docker run --cap-add SYS_PTRACE -v $(pwd):/src -w /src -it realcore:dev /src/bin/realcore /src/tests/%s" % name)
    result = {}
    with open("result.json", "r") as f:
        result = json.load(f)
    return result

class HelloWorldTestCase(unittest.TestCase):
    def setUp(self):
        build_binary("sample_hello.c")
        self.result = run_binary("sample_hello", None)
    def test_success(self):
        self.assertEqual(self.result["result"]["flags"]["complete"], True)
        self.assertEqual(self.result["result"]["flags"]["error"], 1)
