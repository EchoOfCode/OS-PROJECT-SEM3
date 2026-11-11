#!/usr/bin/env bash
set -uo pipefail

TARGET="./build/campuscompute"
TEST_LOG="logs/test_logger_run.log"
FAILURES=0

echo "Running integration tests: test_logger.sh"
echo "----------------------------------------"

mkdir -p logs
rm -f "$TEST_LOG"

echo "  [RUN] $TARGET logger --producers 2 --events 5 --capacity 5 --output $TEST_LOG"
"$TARGET" logger --producers 2 --events 5 --capacity 5 --output "$TEST_LOG"
STATUS=$?

if [ "$STATUS" -ne 0 ]; then
    echo "  [FAIL] Logger exited with non-zero code $STATUS"
    ((FAILURES++))
else
    echo "  [PASS] Logger exited successfully (code 0)"
fi

if [ -f "$TEST_LOG" ]; then
    echo "  [PASS] Output log file $TEST_LOG was created"
    
    # Check total events: 2 producers * 5 events = 10 events
    LINE_COUNT=$(grep -c "Producer" "$TEST_LOG" || true)
    echo "  [INFO] Total log events found: $LINE_COUNT"
    if [ "$LINE_COUNT" -eq 10 ]; then
        echo "  [PASS] Expected 10 events, found exactly 10"
    else
        echo "  [FAIL] Expected 10 events, found $LINE_COUNT"
        ((FAILURES++))
    fi
else
    echo "  [FAIL] Output log file $TEST_LOG does not exist"
    ((FAILURES++))
fi

rm -f "$TEST_LOG"

echo ""
if [ "$FAILURES" -eq 0 ]; then
    echo "=== test_logger.sh: ALL TESTS PASSED ==="
    exit 0
else
    echo "=== test_logger.sh: $FAILURES TESTS FAILED ==="
    exit 1
fi
