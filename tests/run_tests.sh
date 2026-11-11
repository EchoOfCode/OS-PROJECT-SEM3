#!/usr/bin/env bash
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${PROJECT_ROOT}"

TOTAL_PASSED=0
TOTAL_FAILED=0

run_unit_test() {
    local bin="$1"
    local name="$2"
    echo "--------------------------------------------------"
    echo "Executing Unit Test: $name"
    echo "--------------------------------------------------"
    if [ -x "$bin" ]; then
        if "$bin"; then
            echo "--> Result: $name PASSED"
            ((TOTAL_PASSED++))
        else
            echo "--> Result: $name FAILED"
            ((TOTAL_FAILED++))
        fi
    else
        echo "--> ERROR: Executable $bin not found or not executable"
        ((TOTAL_FAILED++))
    fi
}

run_integration_test() {
    local script="$1"
    local name="$2"
    echo "--------------------------------------------------"
    echo "Executing Integration Test: $name"
    echo "--------------------------------------------------"
    if [ -f "$script" ]; then
        if bash "$script"; then
            echo "--> Result: $name PASSED"
            ((TOTAL_PASSED++))
        else
            echo "--> Result: $name FAILED"
            ((TOTAL_FAILED++))
        fi
    else
        echo "--> ERROR: Script $script not found"
        ((TOTAL_FAILED++))
    fi
}

echo "=================================================="
echo "CampusCompute — Master Test Runner"
echo "=================================================="

echo ""
echo "=== STEP 1: Building Unit Test Executables ==="
make test_units

echo ""
echo "=== STEP 2: Running Unit Tests ==="
run_unit_test "./build/test_jobs" "Job Management Unit Tests"
run_unit_test "./build/test_scheduler" "CPU Schedulers Unit Tests"
run_unit_test "./build/test_banker" "Banker Safety Algorithm Unit Tests"

echo ""
echo "=== STEP 3: Running Integration Tests ==="
run_integration_test "tests/integration/test_cli.sh" "CLI Interaction & Validation"
run_integration_test "tests/integration/test_logger.sh" "Producer-Consumer Logger"
run_integration_test "tests/integration/test_ipc.sh" "Shared Memory & Semaphore IPC"
run_integration_test "tests/integration/test_worker.sh" "Worker Lifecycle & Failure Detection"
run_integration_test "tests/integration/test_cleanup.sh" "Resource Cleanup Verification"

echo ""
echo "=================================================="
echo "CampusCompute Test Suite Summary:"
echo "  Total Passed: $TOTAL_PASSED"
echo "  Total Failed: $TOTAL_FAILED"
echo "=================================================="

if [ "$TOTAL_FAILED" -eq 0 ]; then
    echo "ALL TESTS PASSED SUCCESSFULLY!"
    exit 0
else
    echo "SOME TESTS FAILED!"
    exit 1
fi
