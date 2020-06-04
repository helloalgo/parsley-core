# Parsley
```
Run a binary with resource limitations
Usage:
  parsley-run [OPTION...] positional parameters

  -c, --cputime arg   Maximum CPU time in milliseconds (default: 1000)
  -r, --realtime arg  Maximum real time in milliseconds (default: 1000)
  -p, --nproc arg     Maximum process count (default: 1)
  -s, --stack arg     Maximum stack size in kilobytes (default: 131072)
  -m, --mem arg       Maximum memory in kilobytes (default: 131072)
  -w, --write arg     Maximum write size in kilobytes (default: 0)
      --ptrace        Use ptrace (default: true)
      --seccomp arg   Seccomp policy string (default: basic)
  -h, --help          Print usage
      --stdin arg     Path to read stdin of child
      --stdout arg    Path to write stdout of child
      --stderr arg    Path to write stderr of child
```

# Getting Started
The preferred method for building and running is via Docker.
Extract header files from the actual environment, and set your editor or IDE to use them.
```bash
cd script && ./extract-includes
# Use the headers in script/includes
```
Configuration files for vscode is included.

# Build and run
```bash
cd script && ./build
./run-dev
```