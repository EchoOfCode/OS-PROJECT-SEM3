#!/usr/bin/env bash
set -uo pipefail

TARGET="./build/campuscompute"
FAILURES=0

echo "Running integration tests: test_worker.sh"
echo "----------------------------------------"

# 1. Test worker normal
echo "  [RUN] $TARGET worker normal"
OUTPUT_NORM=$("$TARGET" worker normal 2>&1)
STATUS_NORM=$?

if [ "$STATUS_NORM" -eq 0 ]; then
    echo "  [PASS] Worker normal exited with status 0"
else
    echo "  [FAIL] Worker normal exited with status $STATUS_NORM"
    ((FAILURES++))
fi

if echo "$OUTPUT_NORM" | grep -qi "normal"; then
    echo "  [PASS] Worker normal output verified"
else
    echo "  [FAIL] Worker normal output missing confirmation"
    ((FAILURES++))
fi

# 2. Test worker fail (parent detects failure and reports it, exiting 0)
echo "  [RUN] $TARGET worker fail"
OUTPUT_FAIL=$("$TARGET" worker fail 2>&1)
STATUS_FAIL=$?

if [ "$STATUS_FAIL" -eq 0 ]; then
    echo "  [PASS] Worker fail exited with status 0 (parent cleanly caught child failure)"
else
    echo "  [FAIL] Worker fail exited with status $STATUS_FAIL"
    ((FAILURES++))
fi

if echo "$OUTPUT_FAIL" | grep -qi "fail"; then
    echo "  [PASS] Worker failure detected and reported in output"
else
    echo "  [FAIL] Worker failure was not detected in output"
    ((FAILURES++))
fi

echo ""
if [ "$FAILURES" -eq 0 ]; then
    echo "=== test_worker.sh: ALL TESTS PASSED ==="
    exit 0
else
    echo "=== test_worker.sh: $FAILURES TESTS FAILED ==="
    exit 1
fi
