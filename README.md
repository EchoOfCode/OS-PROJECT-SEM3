# CampusCompute — Intelligent Job Scheduling and Resource Management

An academic and professional Operating Systems mini-project implemented in standard C11 for Linux / Ubuntu (WSL). CampusCompute models a high-performance university shared computing facility where multiple compute jobs compete for CPU time and system resources.

The application demonstrates core operating-system mechanisms through real algorithms and POSIX system calls: CPU scheduling, multithreaded producer-consumer synchronization, inter-process communication via shared memory, deadlock avoidance via Dijkstra's Banker's algorithm, child process lifecycle and failure tracking, POSIX resource limits, and resource leak cleanup.

---

## Table of Contents

1. [Architecture & Directory Structure](#1-architecture--directory-structure)
2. [Functional Requirements (FR1–FR8)](#2-functional-requirements-fr1fr8)
3. [Prerequisites & Environment Setup](#3-prerequisites--environment-setup)
4. [Build & Compilation Guide](#4-build--compilation-guide)
5. [CLI Command Reference](#5-cli-command-reference)
6. [Test Suite & Verification](#6-test-suite--verification)
7. [Screenshots & Submission Evidence](#7-screenshots--submission-evidence)
8. [Submission Packaging](#8-submission-packaging)

---

## 1. Architecture & Directory Structure

```text
CampusCompute/
├── Makefile                        # POSIX C11 strict build system (-Wall -Wextra -Werror)
├── README.md                       # Complete technical documentation
├── requirements-evidence.txt       # Python dependencies for evidence assembly
├── .gitignore                      # Exclusions for build artifacts and temporary files
│
├── include/                        # Public C header files with include guards
│   ├── campuscompute.h             # Master header
│   ├── common.h                    # Error macros, safe memory allocators, string helpers
│   ├── cli.h                       # CLI parser and dispatch declarations
│   ├── jobs.h                      # Job struct, JobList container, CSV loader
│   ├── scheduler.h                 # Timeline segment, ScheduleResult, FCFS/RR/Priority
│   ├── logger.h                    # POSIX thread producer-consumer bounded queue
│   ├── ipc.h                       # POSIX shared memory & semaphore payload definitions
│   ├── banker.h                    # Banker's safety algorithm and state matrix structures
│   ├── worker.h                    # Process fork, waitpid, and exit status tracking
│   ├── resource_limits.h           # POSIX getrlimit/setrlimit RLIMIT_NOFILE enforcement
│   └── cleanup.h                   # Resource reclamation and verification
│
├── src/                            # Module implementations
│   ├── main.c                      # Application entry point
│   ├── cli.c                       # Argument parsing, error checking, integrated demo
│   ├── common.c                    # Safe memory allocators and string conversions
│   ├── jobs.c                      # Job management and CSV parser
│   ├── scheduler_common.c          # Timeline management, Gantt printing, comparison table
│   ├── scheduler_fcfs.c            # First-Come First-Served scheduling algorithm
│   ├── scheduler_rr.c              # Round Robin scheduling algorithm (preemptive time slice)
│   ├── scheduler_priority.c        # Non-preemptive priority scheduling (lower = higher prio)
│   ├── logger.c                    # Multi-threaded producer-consumer activity logger
│   ├── ipc.c                       # Shared memory (shm_open, mmap) & named semaphores
│   ├── banker.c                    # Dijkstra's Banker's algorithm (safe/unsafe analysis)
│   ├── worker.c                    # Worker lifecycle (fork, waitpid, failure detection)
│   ├── resource_limits.c           # File descriptor limit exhaustion handling
│   └── cleanup.c                   # Resource release validation
│
├── data/                           # Standard test fixtures
│   ├── jobs.csv                    # Sample 4-job scheduling input
│   ├── banker_safe.txt             # 5-process, 3-resource safe state dataset
│   └── banker_unsafe.txt           # 5-process, 3-resource unsafe/deadlock state dataset
│
├── tests/                          # Automated test suites
│   ├── test_framework.h            # Lightweight C unit test framework
│   ├── run_tests.sh                # Master runner for unit and integration suites
│   ├── unit/                       # Unit tests compiled to isolated executables
│   │   ├── test_jobs.c             # Job validation, CSV loading, deep-copy tests
│   │   ├── test_scheduler.c        # Metric calculations (CT/TAT/WT) for FCFS/RR/Priority
│   │   └── test_banker.c           # Safety check, unsafe detection, matrix validation
│   └── integration/                # Bash integration test scripts
│       ├── test_cli.sh             # Command-line interface and argument errors
│       ├── test_logger.sh          # Multi-producer concurrency and log file output
│       ├── test_ipc.sh             # Inter-process shared memory exchange
│       ├── test_worker.sh          # Worker normal and failure exit code detection
│       └── test_cleanup.sh         # Resource cleanup verification
│
├── scripts/                        # Automation & submission utilities
│   ├── setup_environment.sh        # Toolchain installer (gcc, make, python3)
│   ├── run_all_tests.sh            # One-step clean build and test execution
│   ├── prepare_evidence.sh         # Terminal SRN configuration and screenshot commands
│   ├── build_evidence_pdf.py       # ReportLab PDF compiler for 24 screenshots
│   ├── build_submission.sh         # Clean submission ZIP packaging script
│   └── verify_submission.sh        # Pre-flight verification checklist
│
├── screenshots/                    # Screenshot management
│   ├── README.md                   # Evidence capture guidelines
│   ├── manifest.csv                # 24-entry screenshot checklist and status
│   └── captures/                   # Directory where genuine terminal captures are saved
│       └── .gitkeep
│
├── logs/                           # Runtime log destination
│   └── .gitkeep
└── submission/                     # Generated PDF and ZIP destination
```

---

## 2. Functional Requirements (FR1–FR8)

### FR1: Job Management and Lifecycle (`jobs.c`, `jobs.h`)
- Defines `Job` with ID, arrival time, CPU burst time, remaining time, priority (1–10), completion time, turnaround time, waiting time, and status (`JOB_NEW`, `JOB_READY`, `JOB_RUNNING`, `JOB_COMPLETED`).
- Safely parses CSV files (`job_list_load_csv`), ignoring comments and blank lines, validating integer ranges.
- Enforces strict validation (`job_validate`): burst > 0, arrival >= 0, priority in [1, 10], and rejects duplicate job IDs.
- Provides deep-copy capability (`job_list_copy`) to ensure independent scheduler execution without cross-contamination.

### FR2: CPU Scheduling Algorithms (`scheduler_*.c`, `scheduler.h`)
- **First Come First Serve (FCFS)**: Non-preemptive scheduling based on arrival time. Handles CPU idle gaps when the CPU arrives before jobs.
- **Round Robin (RR)**: Preemptive scheduling using a fixed time quantum. Jobs arriving during a slice enter the ready queue before the preempted job is requeued.
- **Priority Scheduling**: Non-preemptive scheduling prioritizing the lowest numerical priority (1 = highest priority). Tie-breaking falls back to arrival time, then input order.
- Generates ASCII execution timelines (Gantt charts), calculates Completion Time (CT), Turnaround Time (TAT = CT - Arrival), Waiting Time (WT = TAT - Burst), and average TAT/WT.
- Schedulers can be evaluated simultaneously using the `compare` command.

### FR3: Concurrent Activity Logger (`logger.c`, `logger.h`)
- Bounded circular queue (`LogQueue`) protected by a POSIX mutex (`pthread_mutex_t`) and two condition variables (`pthread_cond_t` for `not_full` and `not_empty`).
- Configurable multiple producer threads generate structured events; a dedicated consumer thread drains the queue and writes formatted timestamped logs to file.
- Clean shutdown: atomic decrement of active producers, queue draining before thread termination, and proper joining of all threads.

### FR4: Inter-Process Communication (`ipc.c`, `ipc.h`)
- Cooperating parent and child processes communicating through POSIX shared memory (`shm_open`, `ftruncate`, `mmap`).
- Coordinated using named POSIX semaphores (`sem_open`, `sem_wait`, `sem_post`).
- Exchanging structured payloads (`SharedPayload`) with request/response verification.
- Guarantees cleanup via `munmap`, `shm_unlink`, `sem_close`, and `sem_unlink`.

### FR5: Resource-Safety Analysis (`banker.c`, `banker.h`)
- Implements Dijkstra's Banker's Algorithm for deadlock avoidance.
- Calculates the `Need` matrix: $\text{Need}[i][j] = \text{Maximum}[i][j] - \text{Allocation}[i][j]$.
- Validates inputs: checks non-negative resources, validates $\text{Allocation} \le \text{Maximum}$.
- Finds and prints the safe execution sequence for safe states; detects unsafe states and explains blocked processes.

### FR6: Worker Process Lifecycle & Failure Detection (`worker.c`, `worker.h`)
- Spawns worker processes using `fork()`.
- Parent monitors child execution using `waitpid()`.
- Uses POSIX inspection macros (`WIFEXITED`, `WEXITSTATUS`, `WIFSIGNALED`, `WTERMSIG`) to detect normal termination (status 0) and controlled failure (status 42 or signal).

### FR7: Resource Limits Enforcement & Cleanup (`resource_limits.c`, `cleanup.c`)
- Enforces POSIX file descriptor limits (`RLIMIT_NOFILE`) using `getrlimit` and `setrlimit`.
- Demonstrates process boundary behavior when resources are exhausted (`EMFILE`).
- Demonstrates thorough cleanup of allocated memory, mutexes, condition variables, child processes, temporary files, shared memory segments, and semaphores.

### FR8: Command-Line Interface (`cli.c`, `main.c`)
- Robust argument parser handling subcommands and flags (`--input`, `--quantum`, `--producers`, `--events`, `--capacity`, `--output`).
- Meaningful diagnostic error reporting and non-zero exit codes (exit 1) on errors.
- Provides an integrated end-to-end demo (`campuscompute demo integrated`) executing all modules sequentially.

---

## 3. Prerequisites & Environment Setup

The application is built for Linux / Ubuntu (including WSL Ubuntu on Windows).

### Toolchain Requirements
- **GCC**: C11 compliant compiler (`gcc >= 9.0`)
- **GNU Make**: Build automation tool
- **POSIX Libraries**: `pthread` (threading), `rt` (real-time POSIX IPC)
- **Python 3 & pip**: For assembling the evidence screenshot PDF (`reportlab`, `pillow`)
- **unzip**: For archive verification

### Setup on Ubuntu / WSL
Run the automated environment setup script:
```bash
bash scripts/setup_environment.sh
```

Or install packages manually:
```bash
sudo apt-get update
sudo apt-get install -y build-essential gcc make python3 python3-pip unzip
pip3 install -r requirements-evidence.txt
```

---

## 4. Build & Compilation Guide

The build system adheres to strict C11 compilation flags:
`-std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wformat=2 -Werror`

### Common Makefile Targets
```bash
# Compile the main campuscompute binary
make all

# Clean all build artifacts
make clean

# Compile and run the complete test suite (unit + integration)
make test

# Compile isolated unit test binaries
make test_units

# Build with AddressSanitizer and UndefinedBehaviorSanitizer
make sanitize

# Run the integrated demonstration
make demo

# Display available Makefile targets
make help
```

---

## 5. CLI Command Reference

```bash
# Display help and command usage
./build/campuscompute help

# Job Management
./build/campuscompute jobs list --input data/jobs.csv

# CPU Scheduling
./build/campuscompute schedule fcfs --input data/jobs.csv
./build/campuscompute schedule rr --quantum 2 --input data/jobs.csv
./build/campuscompute schedule priority --input data/jobs.csv
./build/campuscompute compare --quantum 2 --input data/jobs.csv

# Concurrency & Activity Logger
./build/campuscompute logger --producers 2 --events 10 --capacity 5 [--output logs/app.log]

# Inter-Process Communication
./build/campuscompute ipc demo

# Deadlock Avoidance (Banker's Algorithm)
./build/campuscompute banker safe
./build/campuscompute banker unsafe

# Process Lifecycle & Failure Detection
./build/campuscompute worker normal
./build/campuscompute worker fail

# Resource Limits & Cleanup
./build/campuscompute limits nofile
./build/campuscompute cleanup demo

# Integrated End-to-End Demonstration
./build/campuscompute demo integrated --input data/jobs.csv --quantum 2
```

---

## 6. Test Suite & Verification

The project includes both unit tests (compiled C executables) and integration tests (bash test suites).

### Running All Tests
```bash
bash scripts/run_all_tests.sh
```
Or:
```bash
make test
```

### Unit Tests (`tests/unit/`)
- `test_jobs`: CSV parsing, field validation, duplicate rejection, deep-copy isolation.
- `test_scheduler`: Completion/turnaround/waiting time calculations against known fixtures, single-job execution, empty input handling, idle periods, quantum validation.
- `test_banker`: Safe sequence calculation, deadlock/unsafe detection, allocation > max validation, file loading error handling.

### Integration Tests (`tests/integration/`)
- `test_cli.sh`: Option parsing, unknown commands, missing arguments, non-existent files.
- `test_logger.sh`: Multithreaded execution, log creation, event count verification.
- `test_ipc.sh`: Shared memory creation, payload exchange, semaphore synchronization.
- `test_worker.sh`: Normal child process exit, controlled failure detection.
- `test_cleanup.sh`: Verification of resource unlinking and teardown.

---

## 7. Screenshots & Submission Evidence

The project requires 24 genuine execution screenshots captured in your Ubuntu / WSL terminal, with your Student Registration Number (SRN) visible in the prompt.

### Quick Screenshot Capture Workflow
1. Read the instructions in `screenshots/README.md`.
2. Configure your SRN terminal prompt:
   ```bash
   export SRN="PES1UG20CS000"     # Replace with your actual SRN
   export PS1="[${SRN}] \w\$ "
   ```
3. Run `bash scripts/prepare_evidence.sh` to print the command for each screenshot.
4. Execute each command in your terminal and save the capture to `screenshots/captures/S01_environment.png` through `S24_submission_zip.png`.
5. Update `screenshots/manifest.csv` marking captured items as `CAPTURED`.
6. Compile the evidence PDF:
   ```bash
   python3 scripts/build_evidence_pdf.py --srn PES1UG20CS000
   ```
   The PDF will be generated at `submission/PES1UG20CS000.pdf`.

---

## 8. Submission Packaging

To build the final submission archive:
```bash
bash scripts/build_submission.sh PES1UG20CS000
```
This generates `submission/PES1UG20CS000.zip` containing all source code, headers, Makefile, tests, scripts, fixtures, and documentation, while excluding object files and temporary build artifacts.

To verify readiness prior to submission:
```bash
bash scripts/verify_submission.sh
```
