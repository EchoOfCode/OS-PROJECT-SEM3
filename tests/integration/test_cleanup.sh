#!/usr/bin/env bash
set -uo pipefail

TARGET="./build/campuscompute"
FAILURES=0

echo "Running integration tests: test_cleanup.sh"
echo "----------------------------------------"

echo "  [RUN] $TARGET cleanup demo"
OUTPUT=$("$TARGET" cleanup demo 2>&1)
STATUS=$?

if [ "$STATUS" -eq 0 ]; then
    echo "  [PASS] Cleanup demo exited with status 0"
else
    echo "  [FAIL] Cleanup demo exited with status $STATUS"
    ((FAILURES++))
fi

if echo "$OUTPUT" | grep -qi "cleanup"; then
    echo "  [PASS] Cleanup output verified"
else
    echo "  [FAIL] Cleanup output missing expected headers"
    ((FAILURES++))
fi

echo ""
if [ "$FAILURES" -eq 0 ]; then
    echo "=== test_cleanup.sh: ALL TESTS PASSED ==="
    exit 0
else
    echo "=== test_cleanup.sh: $FAILURES TESTS FAILED ==="
    exit 1
fi
