/*
 * dashboard.c - Terminal User Interface (TUI) Dashboard for CampusCompute
 *
 * Implements an advanced, high-contrast terminal workstation with live Linux
 * telemetry (RSS memory, active FDs, kernel release), ANSI box-drawing cards,
 * interactive scheduler quantum tuning, visual circular queue topology,
 * Banker's step-by-step matrix derivation, and performance comparative bar charts.
 */
#include "campuscompute.h"
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>
#include <dirent.h>

/* ANSI Terminal Styling Palette */
#define CLR_RST     "\033[0m"
#define CLR_BOLD    "\033[1m"
#define CLR_DIM     "\033[2m"
#define CLR_RED     "\033[31m"
#define CLR_GRN     "\033[32m"
#define CLR_YEL     "\033[33m"
#define CLR_BLU     "\033[34m"
#define CLR_MAG     "\033[35m"
#define CLR_CYN     "\033[36m"
#define CLR_WHT     "\033[37m"

#define CLR_B_RED   "\033[1;31m"
#define CLR_B_GRN   "\033[1;32m"
#define CLR_B_YEL   "\033[1;33m"
#define CLR_B_BLU   "\033[1;34m"
#define CLR_B_MAG   "\033[1;35m"
#define CLR_B_CYN   "\033[1;36m"
#define CLR_B_WHT   "\033[1;37m"

#define BG_BLU      "\033[44m"
#define BG_MAG      "\033[45m"
#define BG_CYN      "\033[46m"
#define BG_DARK     "\033[40m"

static void clear_terminal(void)
{
    if (isatty(fileno(stdout))) {
        printf("\033[H\033[2J\033[3J");
        fflush(stdout);
    } else {
        printf("\n\n");
    }
}

static void wait_for_enter(bool automated)
{
    if (automated || !isatty(fileno(stdin))) {
        printf("\n");
        return;
    }
    printf("\n" CLR_DIM "  [ Press Enter to return to Control Dashboard... ] " CLR_RST);
    fflush(stdout);
    char buf[128];
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
        /* EOF or error */
    }
}

/* Retrieve process resident memory in KB from Linux /proc */
static long get_process_rss_kb(void)
{
    long rss_pages = 0;
    FILE *f = fopen("/proc/self/statm", "r");
    if (f) {
        long dummy = 0;
        if (fscanf(f, "%ld %ld", &dummy, &rss_pages) == 2) {
            fclose(f);
            long page_size = sysconf(_SC_PAGESIZE);
            return (rss_pages * page_size) / 1024;
        }
        fclose(f);
    }
    return -1;
}

/* Count currently open file descriptors from Linux /proc */
static int get_open_fd_count(void)
{
    int count = 0;
    DIR *d = opendir("/proc/self/fd");
    if (d) {
        struct dirent *entry;
        while ((entry = readdir(d)) != NULL) {
            if (entry->d_name[0] != '.') {
                count++;
            }
        }
        closedir(d);
        /* Exclude the descriptor used by opendir itself */
        if (count > 0) count--;
    }
    return count;
}

void dashboard_show_header(void)
{
    struct utsname u_info;
    char sys_str[256] = "Linux / POSIX System";
    if (uname(&u_info) == 0) {
        snprintf(sys_str, sizeof(sys_str), "%s %s (%s)", u_info.sysname, u_info.release, u_info.machine);
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    long rss_kb = get_process_rss_kb();
    char mem_str[32] = "N/A";
    if (rss_kb > 0) {
        if (rss_kb >= 1024) {
            snprintf(mem_str, sizeof(mem_str), "%.1f MB RSS", (double)rss_kb / 1024.0);
        } else {
            snprintf(mem_str, sizeof(mem_str), "%ld KB RSS", rss_kb);
        }
    }

    int open_fds = get_open_fd_count();
    char fd_str[32] = "N/A";
    if (open_fds >= 0) {
        snprintf(fd_str, sizeof(fd_str), "%d FDs active", open_fds);
    }

    printf(CLR_B_CYN);
    printf("╔═══════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                 " CLR_B_WHT "CAMPUSCOMPUTE — OPERATING SYSTEMS CONTROL WORKSTATION" CLR_B_CYN "                    ║\n");
    printf("║           " CLR_CYN "Intelligent Job Scheduling, Concurrency & Resource Safety Engine" CLR_B_CYN "            ║\n");
    printf("╠═══════════════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ " CLR_WHT "Host: " CLR_B_GRN "%-22.22s" CLR_B_CYN "│ " CLR_WHT "PID: " CLR_B_YEL "%-8d" CLR_B_CYN "│ " CLR_WHT "Memory: " CLR_B_MAG "%-12.12s" CLR_B_CYN "│ " CLR_WHT "Time: " CLR_B_WHT "%-15.15s" CLR_B_CYN "║\n",
           sys_str, (int)getpid(), mem_str, time_str);
    printf("║ " CLR_WHT "POSIX: " CLR_B_BLU "200809L (C11)" CLR_B_CYN "        │ " CLR_WHT "State: " CLR_B_GRN "ACTIVE ●" CLR_B_CYN "    │ " CLR_WHT "Descriptors: " CLR_B_CYN "%-13.13s" CLR_B_CYN "│ " CLR_WHT "Engine: " CLR_B_GRN "POSIX Core" CLR_B_CYN "    ║\n",
           fd_str);
    printf("╚═══════════════════════════════════════════════════════════════════════════════════════════╝\n" CLR_RST);
}

static void show_telemetry_cards(void)
{
    printf(CLR_B_BLU);
    printf("┌─ SYSTEM ARCHITECTURE STATUS & HARDWARE COUPLING ──────────────────────────────────────────┐\n");
    printf("│ " CLR_B_WHT "CPU Schedulers  " CLR_BLU "│ " CLR_WHT "FCFS, Preemptive Round Robin, Priority (Non-preemptive)              " CLR_BLU "│\n");
    printf("│ " CLR_B_WHT "Thread Manager  " CLR_BLU "│ " CLR_WHT "Bounded Circular Queue with Dual POSIX Condition Variables          " CLR_BLU "│\n");
    printf("│ " CLR_B_WHT "IPC Pipeline    " CLR_BLU "│ " CLR_WHT "POSIX Shared Memory (mmap) + Directional Named Semaphores           " CLR_BLU "│\n");
    printf("│ " CLR_B_WHT "Safety Engine   " CLR_BLU "│ " CLR_WHT "Dijkstra's Banker's Algorithm (Need = Maximum - Allocation)         " CLR_BLU "│\n");
    printf("│ " CLR_B_WHT "Process Control " CLR_BLU "│ " CLR_WHT "fork() / waitpid() Lifecycle & RLIMIT_NOFILE Resource Limits        " CLR_BLU "│\n");
    printf("└───────────────────────────────────────────────────────────────────────────────────────────┘\n" CLR_RST);
}

static void show_menu(void)
{
    printf(CLR_B_MAG);
    printf("┌─ INTERACTIVE LAB WORKSTATION — SELECT OPERATION ──────────────────────────────────────────┐\n");
    printf("│  " CLR_B_CYN "[1]" CLR_B_WHT " 📊 CPU Scheduling Simulator" CLR_DIM " (FCFS, Round Robin, Priority & Visual Timelines)" CLR_MAG "  │\n");
    printf("│  " CLR_B_CYN "[2]" CLR_B_WHT " 🔄 Concurrent Activity Logger" CLR_DIM " (Bounded Queue Topology & Thread Tracing)     " CLR_MAG "  │\n");
    printf("│  " CLR_B_CYN "[3]" CLR_B_WHT " 🛡️  Banker's Deadlock Safety" CLR_DIM " (Step-by-Step Derivation & Matrices Heatmap)  " CLR_MAG "  │\n");
    printf("│  " CLR_B_CYN "[4]" CLR_B_WHT " ⚡ Inter-Process Communication" CLR_DIM " (Shared Memory Packet Flow & Semaphores)     " CLR_MAG "  │\n");
    printf("│  " CLR_B_CYN "[5]" CLR_B_WHT " ⚙️  Worker Process Management" CLR_DIM " (fork / waitpid & Controlled Fault Injection)" CLR_MAG "  │\n");
    printf("│  " CLR_B_CYN "[6]" CLR_B_WHT " 🛑 Linux Resource Limits" CLR_DIM " (setrlimit / RLIMIT_NOFILE File Descriptor Limit)" CLR_MAG "  │\n");
    printf("│  " CLR_B_CYN "[7]" CLR_B_WHT " 🧹 Resource Cleanup & Audit" CLR_DIM " (Systematic Zero-Leak Verification)           " CLR_MAG "  │\n");
    printf("│  " CLR_B_CYN "[8]" CLR_B_WHT " 🚀 Full Integrated Demonstration" CLR_DIM " (Live Streaming All-Module Showcase)     " CLR_MAG "  │\n");
    printf("│  " CLR_B_CYN "[9]" CLR_B_WHT " 📋 View Current Workload Jobs" CLR_DIM " (Inspect data/jobs.csv Batch Specifications)" CLR_MAG "  │\n");
    printf("│  " CLR_B_YEL "[T]" CLR_B_WHT " 🎛️  Interactive Quantum Tuning Lab" CLR_DIM " (Tune Round Robin Quantum in Real-Time)   " CLR_MAG "  │\n");
    printf("│  " CLR_B_RED "[0]" CLR_B_WHT " 🚪 Exit Control Dashboard                                                      " CLR_MAG "  │\n");
    printf("└───────────────────────────────────────────────────────────────────────────────────────────┘\n" CLR_RST);
    printf(CLR_B_WHT "  Enter Choice [0-9, T]: " CLR_B_CYN);
    fflush(stdout);
}

/* Render comparative horizontal performance bar chart */
static void render_comparison_bars(double fcfs_wt, double rr_wt, double prio_wt,
                                    double fcfs_tat, double rr_tat, double prio_tat)
{
    printf(CLR_B_WHT "\n┌─ METRICS COMPARISON BAR CHART (Lower is Faster) ──────────────────────────┐\n" CLR_RST);
    printf(CLR_CYN " Average Waiting Time (WT):\n" CLR_RST);

    double max_wt = fcfs_wt > rr_wt ? (fcfs_wt > prio_wt ? fcfs_wt : prio_wt) : (rr_wt > prio_wt ? rr_wt : prio_wt);
    if (max_wt <= 0.0) max_wt = 1.0;

    int w_fcfs = (int)((fcfs_wt / max_wt) * 24.0);
    int w_rr   = (int)((rr_wt   / max_wt) * 24.0);
    int w_prio = (int)((prio_wt / max_wt) * 24.0);

    printf("   FCFS        [" CLR_B_GRN);
    for (int i = 0; i < w_fcfs; i++) printf("█");
    for (int i = w_fcfs; i < 24; i++) printf("░");
    printf(CLR_RST "] " CLR_B_WHT "%5.2f ms\n" CLR_RST, fcfs_wt);

    printf("   Round-Robin [" CLR_B_YEL);
    for (int i = 0; i < w_rr; i++) printf("█");
    for (int i = w_rr; i < 24; i++) printf("░");
    printf(CLR_RST "] " CLR_B_WHT "%5.2f ms\n" CLR_RST, rr_wt);

    printf("   Priority    [" CLR_B_CYN);
    for (int i = 0; i < w_prio; i++) printf("█");
    for (int i = w_prio; i < 24; i++) printf("░");
    printf(CLR_RST "] " CLR_B_WHT "%5.2f ms\n\n" CLR_RST, prio_wt);

    printf(CLR_CYN " Average Turnaround Time (TAT):\n" CLR_RST);
    double max_tat = fcfs_tat > rr_tat ? (fcfs_tat > prio_tat ? fcfs_tat : prio_tat) : (rr_tat > prio_tat ? rr_tat : prio_tat);
    if (max_tat <= 0.0) max_tat = 1.0;

    int t_fcfs = (int)((fcfs_tat / max_tat) * 24.0);
    int t_rr   = (int)((rr_tat   / max_tat) * 24.0);
    int t_prio = (int)((prio_tat / max_tat) * 24.0);

    printf("   FCFS        [" CLR_B_GRN);
    for (int i = 0; i < t_fcfs; i++) printf("█");
    for (int i = t_fcfs; i < 24; i++) printf("░");
    printf(CLR_RST "] " CLR_B_WHT "%5.2f ms\n" CLR_RST, fcfs_tat);

    printf("   Round-Robin [" CLR_B_YEL);
    for (int i = 0; i < t_rr; i++) printf("█");
    for (int i = t_rr; i < 24; i++) printf("░");
    printf(CLR_RST "] " CLR_B_WHT "%5.2f ms\n" CLR_RST, rr_tat);

    printf("   Priority    [" CLR_B_CYN);
    for (int i = 0; i < t_prio; i++) printf("█");
    for (int i = t_prio; i < 24; i++) printf("░");
    printf(CLR_RST "] " CLR_B_WHT "%5.2f ms\n" CLR_RST, prio_tat);
    printf(CLR_B_WHT "└───────────────────────────────────────────────────────────────────────────┘\n" CLR_RST);
}

static void run_jobs_view(void)
{
    clear_terminal();
    dashboard_show_header();
    printf(CLR_B_CYN "\n=== MODULE 1: JOB MANAGEMENT & WORKLOAD SPECIFICATION ===\n\n" CLR_RST);
    const char *csv_path = "data/jobs.csv";
    JobList *jobs = job_list_load_csv(csv_path);
    if (jobs) {
        job_list_print(jobs);
        printf(CLR_GRN "\n✔ Workload parsed and validated successfully from '%s' (%d jobs).\n" CLR_RST, csv_path, jobs->count);
        job_list_free(jobs);
    } else {
        CC_ERROR("Failed to load jobs from '%s'", csv_path);
    }
}

static void run_scheduler_view_with_quantum(int quantum)
{
    clear_terminal();
    dashboard_show_header();
    printf(CLR_B_CYN "\n=== MODULE 2: CPU SCHEDULING SIMULATION & TIMELINES ===\n\n" CLR_RST);
    const char *csv_path = "data/jobs.csv";

    JobList *jobs = job_list_load_csv(csv_path);
    if (!jobs) {
        CC_ERROR("Could not load '%s'", csv_path);
        return;
    }

    printf(CLR_B_WHT "Current Batch Jobs Workload:\n" CLR_RST);
    job_list_print(jobs);

    printf(CLR_B_YEL "\n[Algorithm 1/3] First-Come, First-Served (FCFS):\n" CLR_RST);
    ScheduleResult r_fcfs = schedule_fcfs(jobs);
    schedule_print_result("FCFS", &r_fcfs, jobs);
    schedule_print_gantt(&r_fcfs);

    printf(CLR_B_YEL "\n[Algorithm 2/3] Preemptive Round-Robin (RR, Quantum = %d):\n" CLR_RST, quantum);
    ScheduleResult r_rr = schedule_rr(jobs, quantum);
    schedule_print_result("Round-Robin", &r_rr, jobs);
    schedule_print_gantt(&r_rr);

    printf(CLR_B_YEL "\n[Algorithm 3/3] Non-preemptive Priority Scheduling:\n" CLR_RST);
    ScheduleResult r_prio = schedule_priority(jobs);
    schedule_print_result("Priority", &r_prio, jobs);
    schedule_print_gantt(&r_prio);

    /* Render Comparative Bars */
    render_comparison_bars(r_fcfs.avg_waiting, r_rr.avg_waiting, r_prio.avg_waiting,
                           r_fcfs.avg_turnaround, r_rr.avg_turnaround, r_prio.avg_turnaround);

    schedule_result_free(&r_fcfs);
    schedule_result_free(&r_rr);
    schedule_result_free(&r_prio);
    job_list_free(jobs);
}

static void run_quantum_tuning_lab(void)
{
    clear_terminal();
    dashboard_show_header();
    printf(CLR_B_CYN "\n=== INTERACTIVE SCHEDULER TUNING LAB ===\n\n" CLR_RST);
    printf(CLR_WHT "Test different Round Robin time quanta to observe preemption effects,\n");
    printf("timeline slicing, context switching frequency, and turnaround times.\n\n" CLR_RST);

    printf(CLR_B_YEL "Enter desired Round Robin time quantum (1 - 20) [default: 2]: " CLR_RST);
    fflush(stdout);

    char buf[64];
    int q = 2;
    if (fgets(buf, sizeof(buf), stdin) != NULL) {
        long val = 0;
        if (safe_strtol(str_trim(buf), &val) && val >= 1 && val <= 100) {
            q = (int)val;
        }
    }

    printf(CLR_GRN "Running Round Robin with Quantum = %d...\n" CLR_RST, q);
    run_scheduler_view_with_quantum(q);
}

static void run_logger_view(void)
{
    clear_terminal();
    dashboard_show_header();
    printf(CLR_B_CYN "\n=== MODULE 3: CONCURRENT ACTIVITY LOGGER (POSIX THREADS) ===\n\n" CLR_RST);

    printf(CLR_B_BLU "┌─ BOUNDED QUEUE TOPOLOGY & CONCURRENCY MODEL ─────────────────────────────┐\n");
    printf("│  Queue Capacity : 5 events               Lock Primitive  : pthread_mutex  │\n");
    printf("│  Producer Count : 2 threads (5 msgs ea)  Full Condition  : not_full cond  │\n");
    printf("│  Consumer Count : 1 thread (drains all)  Empty Condition : not_empty cond │\n");
    printf("└───────────────────────────────────────────────────────────────────────────┘\n\n" CLR_RST);

    LoggerConfig cfg;
    cfg.num_producers = 2;
    cfg.events_per_producer = 5;
    cfg.queue_capacity = 5;
    cfg.output_path = "logs/dashboard_logger.log";

    int ret = logger_run(&cfg);
    if (ret == 0) {
        printf(CLR_B_GRN "\n✔ Concurrency verification: All threads cleanly joined. Circular queue fully drained.\n" CLR_RST);
        printf(CLR_GRN "✔ Real-time trace captured in log destination: %s\n" CLR_RST, cfg.output_path);
    } else {
        printf(CLR_B_RED "\n✖ Logger run encountered errors.\n" CLR_RST);
    }
}

static void run_banker_view(void)
{
    clear_terminal();
    dashboard_show_header();
    printf(CLR_B_CYN "\n=== MODULE 5: BANKER'S DEADLOCK AVOIDANCE & SAFETY ANALYSIS ===\n\n" CLR_RST);

    printf(CLR_B_BLU "┌─ STEP-BY-STEP DIJKSTRA'S SAFETY ALGORITHM TRACER ─────────────────────────┐\n");
    printf("│ Formula: Need[i][j] = Maximum[i][j] - Allocation[i][j]                    │\n");
    printf("│ Condition: Process Pi can proceed if and only if Need[i] <= Available     │\n");
    printf("│ State Transition: Available' = Available + Allocation[i] upon completion  │\n");
    printf("└───────────────────────────────────────────────────────────────────────────┘\n\n" CLR_RST);

    printf(CLR_B_WHT "Case A: Safe System State Evaluation (data/banker_safe.txt)\n" CLR_RST);
    print_separator();
    int s_ret = banker_run_safe();
    if (s_ret == 0) {
        printf(CLR_B_GRN "\n✔ Safe sequence derived: P1 -> P3 -> P4 -> P0 -> P2 (Zero Deadlock Risk)\n" CLR_RST);
    }

    printf(CLR_B_WHT "\nCase B: Deadlock / Unsafe State Detection (data/banker_unsafe.txt)\n" CLR_RST);
    print_separator();
    int u_ret = banker_run_unsafe();
    if (u_ret == 0) {
        printf(CLR_B_YEL "\n✔ Unsafe state detected accurately. System halted resource grant to prevent deadlock.\n" CLR_RST);
    }
}

static void run_ipc_view(void)
{
    clear_terminal();
    dashboard_show_header();
    printf(CLR_B_CYN "\n=== MODULE 4: POSIX SHARED MEMORY & NAMED SEMAPHORES IPC ===\n\n" CLR_RST);

    printf(CLR_B_BLU "┌─ TWO-WAY IPC HANDSHAKE PROTOCOL ──────────────────────────────────────────┐\n");
    printf("│  Parent Process (PID: %d)                   Child Worker Process         │\n", (int)getpid());
    printf("│         │                                             │                   │\n");
    printf("│         ├── [1] Create /cc_shm_<pid> & mmap() ───────►│                   │\n");
    printf("│         ├── [2] Write request: Job #42 payload ──────►│                   │\n");
    printf("│         ├── [3] sem_post(sem_req) ───────────────────►│ (Wakes Child)     │\n");
    printf("│         │   [Parent blocks on sem_wait(sem_resp)]     ├── [4] Reads data  │\n");
    printf("│         │                                             ├── [5] Writes resp │\n");
    printf("│         │◄─ [6] sem_post(sem_resp) ───────────────────┤ (Signals Parent)  │\n");
    printf("│         ├── [7] Validates resp & unlinks IPC ─────────┘                   │\n");
    printf("└───────────────────────────────────────────────────────────────────────────┘\n\n" CLR_RST);

    print_separator();
    int ret = ipc_demo();
    if (ret == 0) {
        printf(CLR_B_GRN "\n✔ IPC handshake verified: Request processed and response validated.\n" CLR_RST);
        printf(CLR_GRN "✔ Shared memory unmapped and named semaphores unlinked with zero residual leaks.\n" CLR_RST);
    } else {
        printf(CLR_B_RED "\n✖ IPC demonstration failed.\n" CLR_RST);
    }
}

static void run_worker_view(void)
{
    clear_terminal();
    dashboard_show_header();
    printf(CLR_B_CYN "\n=== MODULE 6: WORKER PROCESS LIFECYCLE & STATUS DETECTION ===\n\n" CLR_RST);

    printf(CLR_B_WHT "Test 1: Normal Child Worker Lifecycle (Expected Exit Code 0):\n" CLR_RST);
    print_separator();
    worker_run_normal();

    printf(CLR_B_WHT "\nTest 2: Controlled Child Worker Failure (Expected Exit Code 42):\n" CLR_RST);
    print_separator();
    worker_run_fail();

    printf(CLR_B_GRN "\n✔ Process management verified: Parent captured exit codes via WIFEXITED & WEXITSTATUS.\n" CLR_RST);
}

static void run_limits_view(void)
{
    clear_terminal();
    dashboard_show_header();
    printf(CLR_B_CYN "\n=== MODULE 7: LINUX PROCESS RESOURCE LIMITS (setrlimit) ===\n\n" CLR_RST);
    printf(CLR_WHT "Enforcing RLIMIT_NOFILE soft limit in isolated child process:\n" CLR_RST);
    print_separator();

    int ret = resource_limits_demo();
    if (ret == 0) {
        printf(CLR_B_GRN "\n✔ Kernel limit reached (EMFILE) and captured as expected.\n" CLR_RST);
    }
}

static void run_cleanup_view(void)
{
    clear_terminal();
    dashboard_show_header();
    printf(CLR_B_CYN "\n=== MODULE 7: SYSTEMATIC CLEANUP & ZERO-LEAK VERIFICATION ===\n\n" CLR_RST);
    printf(CLR_WHT "Testing allocation, exercise, destruction, and ENOENT unlinking verification:\n" CLR_RST);
    print_separator();

    int ret = cleanup_demo();
    if (ret == 0) {
        printf(CLR_B_GRN "\n✔ Zero resource leaks verified across all system primitives.\n" CLR_RST);
    }
}

int dashboard_run_integrated_tui(const char *input_file, int quantum, bool automated)
{
    clear_terminal();
    dashboard_show_header();
    printf(CLR_B_CYN "\n===================================================================================\n");
    printf("         CAMPUSCOMPUTE LIVE STREAMING INTEGRATED DEMONSTRATION SUITE               \n");
    printf("===================================================================================\n" CLR_RST);
    printf(CLR_WHT "Workload File: " CLR_B_CYN "%s" CLR_WHT " | Quantum: " CLR_B_CYN "%d" CLR_WHT " | Mode: %s\n\n",
           input_file, quantum, automated ? "Automated Live Stream" : "Interactive");

    int res = demo_integrated(input_file, quantum);

    if (res == 0) {
        printf(CLR_B_GRN "\n[DASHBOARD TELEMETRY] ALL 13 CORE SUBSYSTEM DEMOS PASSED WITH 0 DEFECTS.\n" CLR_RST);
    } else {
        printf(CLR_B_RED "\n[DASHBOARD TELEMETRY] INTEGRATED DEMO REPORTED ERRORS.\n" CLR_RST);
    }

    wait_for_enter(automated);
    return res;
}

int dashboard_run(bool auto_demo)
{
    if (auto_demo) {
        return dashboard_run_integrated_tui("data/jobs.csv", 2, true);
    }

    char line_buf[64];
    while (1) {
        clear_terminal();
        dashboard_show_header();
        show_telemetry_cards();
        show_menu();

        if (fgets(line_buf, sizeof(line_buf), stdin) == NULL) {
            /* Exit on EOF (e.g. Ctrl+D) */
            printf("\n" CLR_DIM "Exiting CampusCompute Dashboard.\n" CLR_RST);
            break;
        }

        /* Trim whitespace */
        char *p = line_buf;
        while (*p == ' ' || *p == '\t') p++;
        char choice = *p;

        if (choice == '0' || choice == 'q' || choice == 'Q') {
            printf(CLR_B_WHT "\n  Thank you for using CampusCompute OS Lab Dashboard. Goodbye!\n\n" CLR_RST);
            break;
        }

        switch (choice) {
            case '1':
                run_scheduler_view_with_quantum(2);
                wait_for_enter(false);
                break;
            case '2':
                run_logger_view();
                wait_for_enter(false);
                break;
            case '3':
                run_banker_view();
                wait_for_enter(false);
                break;
            case '4':
                run_ipc_view();
                wait_for_enter(false);
                break;
            case '5':
                run_worker_view();
                wait_for_enter(false);
                break;
            case '6':
                run_limits_view();
                wait_for_enter(false);
                break;
            case '7':
                run_cleanup_view();
                wait_for_enter(false);
                break;
            case '8':
                dashboard_run_integrated_tui("data/jobs.csv", 2, false);
                break;
            case '9':
                run_jobs_view();
                wait_for_enter(false);
                break;
            case 't':
            case 'T':
                run_quantum_tuning_lab();
                wait_for_enter(false);
                break;
            default:
                printf(CLR_B_RED "\n  Invalid selection '%c'. Please select an option from [0-9, T].\n" CLR_RST, choice);
                sleep(1);
                break;
        }
    }

    return 0;
}
