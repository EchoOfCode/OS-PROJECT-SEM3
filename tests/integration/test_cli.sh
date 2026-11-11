#!/usr/bin/env bash
set -uo pipefail

TARGET="./build/campuscompute"
FAILURES=0

assert_exit_code() {
    local expected="$1"
    shift
    local cmd=("$@")
    echo "  [RUN] ${cmd[*]}"
    "${cmd[@]}" >/dev/null 2>&1
    local code=$?
    if [ "$code" -eq "$expected" ]; then
        echo "  [PASS] Expected exit code $expected, got $code"
    else
        echo "  [FAIL] Expected exit code $expected, got $code"
        ((FAILURES++))
    fi
}

assert_output_contains() {
    local pattern="$1"
    shift
    local cmd=("$@")
    echo "  [RUN] ${cmd[*]} (checking output contains: '$pattern')"
    local output
    output=$("${cmd[@]}" 2>&1 || true)
    if echo "$output" | grep -qi "$pattern"; then
        echo "  [PASS] Output contains '$pattern'"
    else
        echo "  [FAIL] Output does not contain '$pattern'"
        ((FAILURES++))
    fi
}

echo "Running integration tests: test_cli.sh"
echo "----------------------------------------"

# 1. Help flag and command
assert_exit_code 0 "$TARGET" help
assert_exit_code 0 "$TARGET" --help
assert_output_contains "CampusCompute" "$TARGET" help
assert_output_contains "schedule" "$TARGET" help

# 2. Unknown command (must return exit 1)
assert_exit_code 1 "$TARGET" unknown_command_foo_bar

# 3. Missing arguments
assert_exit_code 1 "$TARGET" jobs list
assert_exit_code 1 "$TARGET" schedule fcfs
assert_exit_code 1 "$TARGET" schedule rr --input data/jobs.csv
assert_exit_code 1 "$TARGET" schedule rr --quantum 0 --input data/jobs.csv
assert_exit_code 1 "$TARGET" schedule rr --quantum -5 --input data/jobs.csv
assert_exit_code 1 "$TARGET" schedule priority
assert_exit_code 1 "$TARGET" compare --quantum 2

# 4. Non-existent file
assert_exit_code 1 "$TARGET" jobs list --input data/nonexistent_jobs_file.csv

echo ""
if [ "$FAILURES" -eq 0 ]; then
    echo "=== test_cli.sh: ALL TESTS PASSED ==="
    exit 0
else
    echo "=== test_cli.sh: $FAILURES TESTS FAILED ==="
    exit 1
fi
