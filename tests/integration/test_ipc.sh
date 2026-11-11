#!/usr/bin/env bash
set -uo pipefail

TARGET="./build/campuscompute"
FAILURES=0

echo "Running integration tests: test_ipc.sh"
echo "----------------------------------------"

echo "  [RUN] $TARGET ipc demo"
OUTPUT=$("$TARGET" ipc demo 2>&1)
STATUS=$?

if [ "$STATUS" -ne 0 ]; then
    echo "  [FAIL] IPC demo exited with non-zero status $STATUS"
    ((FAILURES++))
else
    echo "  [PASS] IPC demo exited with status 0"
fi

# Verify communication occurred
if echo "$OUTPUT" | grep -qi "shared memory"; then
    echo "  [PASS] Output confirmed shared memory usage"
else
    echo "  [FAIL] Output missing shared memory indication"
    ((FAILURES++))
fi

if echo "$OUTPUT" | grep -qi "IPC Demo Completed Successfully"; then
    echo "  [PASS] Output confirmed successful completion"
else
    echo "  [FAIL] Output did not report successful completion"
    ((FAILURES++))
fi

echo ""
if [ "$FAILURES" -eq 0 ]; then
    echo "=== test_ipc.sh: ALL TESTS PASSED ==="
    exit 0
else
    echo "=== test_ipc.sh: $FAILURES TESTS FAILED ==="
    exit 1
fi
