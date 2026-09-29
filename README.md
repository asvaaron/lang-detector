# lang-detector

A small C command-line tool that scans your system for installed programming
language interpreters/compilers and reports their versions, alongside basic
OS information.

It checks 33 languages/toolchains (GCC, Clang, Python 2/3, Node.js, Java, Go,
Rust, Ruby, PHP, Perl, Lua, R, Julia, .NET, GHC, Erlang, Elixir, Swift,
Kotlin, Scala, TypeScript, Deno, Bun, Bash, Zig, Nim, Crystal, Dart,
gfortran, GnuCOBOL, NASM, and more) by searching `$PATH` for each executable
and, if found, running its version command.

## Build

Requires CMake (>= 3.10) and a C11 compiler.

```sh
mkdir -p build
cd build
cmake ..
cmake --build .
```

This produces the `lang-detector` binary inside `build/`.

## Usage

```sh
./lang-detector [-f|--format table|json|csv] [--allow-root] [-h|--help]
```

| Flag | Description |
| --- | --- |
| `-f`, `--format <fmt>` | Output format: `table` (default), `json`, or `csv`. Also accepts `--format=<fmt>`. |
| `--allow-root` | Allow running as `root` (unsafe — see note below). |
| `-h`, `--help` | Show the usage message and exit. |

**Note:** by default the tool refuses to run as `root`, since it executes binaries
found on `$PATH` (e.g. `gcc --version`) to determine their versions. Prefer running it
as a regular user; if you're in a container/VM where you're always root, pass
`--allow-root` to bypass the check.

## Examples

### Table (default)

```sh
$ ./lang-detector
Operating System: Debian GNU/Linux 13 (trixie) (Kernel 6.17.0-PRoot-Distro, aarch64)
Scanning system for installed programming languages...

LANGUAGE               STATUS     VER#           VERSION
------------------------------------------------------------------------------------
C (GCC)                found      14.2.0         gcc (Debian 14.2.0-19) 14.2.0
C (Clang)              found      21.1.8         clang version 21.1.8
C++ (G++)              found      14.2.0         g++ (Debian 14.2.0-19) 14.2.0
Python 3                found      3.13.5         Python 3.13.5
Python 2               missing    -              -
Node.js                found      20.19.2        v20.19.2
...
------------------------------------------------------------------------------------
9 of 33 checked languages appear to be installed.
```

### JSON

```sh
$ ./lang-detector --format json
{
  "installed_count": 9,
  "total_count": 33,
  "languages": [
    {"name": "C (GCC)", "exec": "gcc", "found": true, "version": "gcc (Debian 14.2.0-19) 14.2.0", "version_number": "14.2.0"},
    {"name": "Python 2", "exec": "python2", "found": false, "version": "", "version_number": ""},
    ...
  ]
}
```

### CSV

```sh
$ ./lang-detector --format=csv
language,exec,status,version,version_number
C (GCC),gcc,found,gcc (Debian 14.2.0-19) 14.2.0,14.2.0
C (Clang),clang,found,clang version 21.1.8,21.1.8
Python 3,python3,found,Python 3.13.5,3.13.5
...
```

## Project layout

```
.
├── CMakeLists.txt          # CMake build configuration
├── include/                # Public headers
│   ├── lang_scan.h
│   └── os_info.h
└── src/                    # Implementation
    ├── main.c
    ├── lang_scan.c
    └── os_info.c
```

| File | Purpose |
| --- | --- |
| [src/main.c](src/main.c) | CLI argument parsing and entry point. |
| [include/lang_scan.h](include/lang_scan.h) / [src/lang_scan.c](src/lang_scan.c) | Language table, `$PATH` scanning, version extraction, and table/JSON/CSV printers. |
| [include/os_info.h](include/os_info.h) / [src/os_info.c](src/os_info.c) | Reads `/etc/os-release` and `uname()` to print a one-line OS summary. |
| [CMakeLists.txt](CMakeLists.txt) | CMake build configuration. |

## How it works

For each known language, the tool searches every directory in `$PATH` for
the executable (e.g. `python3`). If found, it runs the language's version
command (e.g. `python3 --version`) through the shell, captures the first
non-empty line of combined stdout/stderr output, and extracts a dotted
version number (e.g. `3.13.5`) from it via a regular expression.
