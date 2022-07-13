#!/usr/bin/env python3

import argparse
from argparse import ArgumentParser
from enum import Enum
import os
from pathlib import Path
import signal
import subprocess

# 2 min timeout
TIMEOUT = 120

# Stats for tests
TOTAL_RUNS = 0
TOTAL_FAILS = 0
TOTAL_TIMEOUTS = 0
FAILED = []


class SubprocessExit(Enum):
    Normal = 1
    Error = 2
    Timeout = 3


def setup_parser(parser: ArgumentParser()):
    parser.add_argument("-t", "--test", help="test name to run", type=str)
    parser.add_argument("--release", help="build in release mode", action="store_true")
    parser.add_argument("--log", help="build with logging", action="store_true")


def get_test_name(test: str) -> str:
    return os.path.basename(test)


def make(cmd: str, path: Path) -> tuple[bytes, SubprocessExit]:
    try:
        make_cmd = format(f"make {cmd}").strip()
        print(f"{bcolors.OKBLUE}=== {make_cmd:<10} === {bcolors.ENDC}", end='', flush=True)

        if cmd == "":
            make_cmd = ["make"]
        else:
            make_cmd = ["make", cmd]

        p = subprocess.run(
                make_cmd,
                check=True,
                env=os.environ.copy(),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=TIMEOUT,
                cwd=path
            )
        return p.stdout, SubprocessExit.Normal
    except subprocess.CalledProcessError as e:
        return e.stdout, SubprocessExit.Error
    except subprocess.TimeoutExpired as e:
        return b"Timeout", SubprocessExit.Timeout


def run_tests(path: Path):
    for file in os.listdir(path):
        file = path / file
        if os.path.isfile(file) and os.access(file, os.X_OK):
            output, exit_code = run_test(file, path)
            check_test(file, output, exit_code, path)


def run_test(test: str, path: Path) -> tuple[bytes, SubprocessExit]:
    try:
        print(f"{bcolors.OKCYAN}Running {bcolors.BOLD}{get_test_name(test)} {bcolors.ENDC}", end='', flush=True)
        p = subprocess.run(
                [test],
                check=True,
                env=os.environ.copy(),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=TIMEOUT,
                cwd=path
            )
        return p.stdout, SubprocessExit.Normal
    except subprocess.CalledProcessError as e:
        tmp = bytearray(e.stdout)
        tmp.extend(bytes(f"{signal.strsignal(-e.returncode)}", "UTF-8"))

        e.stdout = bytes(tmp)
        return e.stdout, SubprocessExit.Error
    except subprocess.TimeoutExpired as e:
        return b"Timeout", SubprocessExit.Timeout


def check_make(cmd: str, output: bytes, exit_code: SubprocessExit):
    if exit_code == SubprocessExit.Normal:
        print(f"{bcolors.OKGREEN}OK{bcolors.ENDC}", flush=True)
    elif exit_code == SubprocessExit.Error:
        make_cmd = format(f"make {cmd}").strip()
        print(f"{bcolors.FAIL}FAIL{bcolors.ENDC}", flush=True)
        raise Exception(f"{bcolors.FAIL}{make_cmd} failed: {output.decode('UTF-8')}{bcolors.ENDC}")
    else:
        make_cmd = format(f"make {cmd}").strip()
        print(f"{bcolors.WARNING}TIMEOUT{bcolors.ENDC}", flush=True)
        raise Exception(f"{bcolors.WARNING}{make_cmd} timedout: {output.decode('UTF-8')}{bcolors.ENDC}")


def check_test(test: str, output: bytes, exit_code: SubprocessExit, path: Path):
    global TOTAL_RUNS, TOTAL_FAILS, TOTAL_TIMEOUTS

    TOTAL_RUNS += 1
    if exit_code == SubprocessExit.Normal:
        output = output.decode("UTF-8")
        print(f"{bcolors.OKGREEN}OK{bcolors.ENDC}", flush=True)
    elif exit_code == SubprocessExit.Error:
        TOTAL_FAILS += 1
        print(f"{bcolors.FAIL}FAIL{bcolors.ENDC}", flush=True)
        FAILED.append({ "test": test, "output": output.decode("UTF-8"), "exit_code": exit_code })
    else:
        TOTAL_TIMEOUTS += 1
        print(f"{bcolors.WARNING}TIMEOUT{bcolors.ENDC}", flush=True)
        FAILED.append({ "test": test, "output": output.decode("UTF-8"), "exit_code": exit_code })


def main():
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    args = parser.parse_args()

    script_path = os.path.realpath(__file__)
    script_path = Path(script_path).parent.absolute()

    output, exit_code = make("clean", script_path)
    check_make("clean", output, exit_code)

    build_cmd = ""
    if args.release:
        build_cmd += "RELEASE=1 "
    if args.log:
        build_cmd += "LOG=1 "

    output, exit_code = make(build_cmd, script_path)
    check_make(build_cmd, output, exit_code)

    if args.test:
        output, exit_code = make(f"tests/{args.test}", script_path)
        check_make(f"tests/{args.test}", output, exit_code)

        test_path = script_path / "tests" / args.test
        output, exit_code = run_test(test_path, script_path)
        check_test(test_path, output, exit_code, script_path / "tests")
    else:
        output, exit_code = make("test", script_path)
        check_make("test", output, exit_code)

        run_tests(script_path / "tests")

    failed = TOTAL_FAILS
    timedout = TOTAL_TIMEOUTS
    passed = TOTAL_RUNS - failed - timedout
    print(f"{bcolors.OKBLUE}{passed}/{TOTAL_RUNS} tests {bcolors.OKGREEN}passed{bcolors.ENDC}; "     \
            f"{bcolors.OKBLUE}{failed}/{TOTAL_RUNS} tests {bcolors.FAIL}failed{bcolors.ENDC}; "      \
            f"{bcolors.OKBLUE}{timedout}/{TOTAL_RUNS} tests {bcolors.WARNING}timedout{bcolors.ENDC}")

    if FAILED:
        print(f"{bcolors.OKBLUE}list of failed tests: {bcolors.WARNING}{bcolors.BOLD}{[get_test_name(fail['test']) for fail in FAILED]}{bcolors.ENDC}")
        for fail in FAILED:
            print(f"{bcolors.WARNING}{get_test_name(fail['test'])}: {bcolors.FAIL}{fail['output']}{bcolors.ENDC}")


class bcolors:
    OKBLUE = '\033[0;34m'
    OKCYAN = '\033[0;36m'
    OKGREEN = '\033[0;32m'
    WARNING = '\033[0;33m'
    FAIL = '\033[0;31m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


if __name__ == "__main__":
    main()
