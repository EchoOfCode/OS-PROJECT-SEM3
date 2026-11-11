/*
 * cli.c - Command-line interface implementation for CampusCompute
 */
#include "campuscompute.h"

const char *cli_parse_option(int argc, char *argv[], const char *name)
{
    if (!argv || !name || argc < 1) {
        return NULL;
    }

    char flag_name[128];
    if (name[0] == '-') {
        snprintf(flag_name, sizeof(flag_name), "%s", name);
    } else {
        snprintf(flag_name, sizeof(flag_name), "--%s", name);
    }

    size_t flag_len = strlen(flag_name);

    for (int i = 1; i < argc; i++) {
        if (!argv[i]) {
            continue;
        }

        /* Check exact match: "--name value" */
        if (strcmp(argv[i], flag_name) == 0) {
            if (i + 1 < argc) {
                /* If next token is another option flag, value is missing */
                if (argv[i + 1][0] == '-' && argv[i + 1][1] == '-') {
                    return NULL;
                }
                return argv[i + 1];
            }
            return NULL;
        }

        /* Check "--name=value" */
        if (strncmp(argv[i], flag_name, flag_len) == 0 && argv[i][flag_len] == '=') {
            return &argv[i][flag_len + 1];
        }
    }

    return NULL;
}

bool cli_parse_int_option(int argc, char *argv[], const char *name, int *out)
{
    if (!out) {
        return false;
    }
    const char *val_str = cli_parse_option(argc, argv, name);
    if (!val_str) {
        return false;
    }
    long lval = 0;
    if (!safe_strtol(val_str, &lval)) {
        return false;
    }
    if (lval < INT_MIN || lval > INT_MAX) {
        return false;
    }
    *out = (int)lval;
    return true;
}

void cli_print_help(void)
{
    printf("\n");
    print_separator();
    printf("  CampusCompute - Operating Systems Mini-Project Suite\n");
    print_separator();
    printf("\nUsage:\n");
    printf("  campuscompute <command> [options]\n\n");
    printf("Commands:\n");
    printf("  help (or --help)\n");
    printf("      Show this help message and exit.\n\n");
    printf("  jobs list --input <file>\n");
    printf("      Parse, validate, and display jobs from a CSV file.\n\n");
    printf("  schedule fcfs --input <file>\n");
    printf("      First-Come, First-Served CPU scheduling simulation.\n\n");
    printf("  schedule rr --quantum <n> --input <file>\n");
    printf("      Round-Robin CPU scheduling simulation with time quantum <n>.\n\n");
    printf("  schedule priority --input <file>\n");
    printf("      Non-preemptive Priority CPU scheduling simulation.\n\n");
    printf("  compare --quantum <n> --input <file>\n");
    printf("      Run FCFS, Round-Robin, and Priority schedulers and compare metrics.\n\n");
    printf("  logger --producers <n> --events <n> --capacity <n> [--output <path>]\n");
    printf("      Run concurrent producer-consumer logger with POSIX threads.\n\n");
    printf("  ipc demo\n");
    printf("      Demonstrate POSIX shared memory IPC with semaphores.\n\n");
    printf("  banker safe\n");
    printf("      Demonstrate Banker's algorithm safe sequence discovery.\n\n");
    printf("  banker unsafe\n");
    printf("      Demonstrate Banker's algorithm deadlock detection on unsafe state.\n\n");
    printf("  worker normal\n");
    printf("      Demonstrate worker process normal termination and exit status.\n\n");
    printf("  worker fail\n");
    printf("      Demonstrate worker process failure detection and error handling.\n\n");
    printf("  limits nofile\n");
    printf("      Demonstrate resource limit enforcement (RLIMIT_NOFILE).\n\n");
    printf("  cleanup demo\n");
    printf("      Demonstrate systematic resource cleanup and leak verification.\n\n");
    printf("  demo integrated --input <file> --quantum <n>\n");
    printf("      Execute the unified demonstration suite covering all modules.\n\n");
    print_separator();
}

int demo_integrated(const char *input_file, int quantum)
{
    print_section("CampusCompute Integrated Demonstration Suite");
    CC_INFO("Input file: %s | Round-Robin Quantum: %d", input_file, quantum);

    typedef struct {
        const char *description;
        bool passed;
    } DemoStep;

    DemoStep steps[13] = {
        { "1. Job CSV Loading & Validation", false },
        { "2a. CPU Scheduling - FCFS", false },
        { "2b. CPU Scheduling - Round Robin", false },
        { "2c. CPU Scheduling - Priority", false },
        { "3. CPU Scheduling - Comparison Table", false },
        { "4a. Banker's Algorithm - Safe State", false },
        { "4b. Banker's Algorithm - Unsafe State", false },
        { "5. Multithreaded Logger (Producer-Consumer)", false },
        { "6. POSIX Shared Memory IPC Demo", false },
        { "7a. Worker Process - Normal Execution", false },
        { "7b. Worker Process - Failure Handling", false },
        { "8. Resource Limits (RLIMIT_NOFILE)", false },
        { "9. Systematic Resource Cleanup", false }
    };

    /* 1. Load & display jobs */
    print_section("Step 1: Job Management - Load & Display Jobs");
    JobList *jobs = job_list_load_csv(input_file);
    if (jobs) {
        job_list_print(jobs);
        steps[0].passed = true;
    } else {
        CC_ERROR("Failed to load jobs from '%s'", input_file);
    }

    /* 2 & 3. CPU Scheduling and comparison */
    if (jobs) {
        print_section("Step 2a: CPU Scheduling - First-Come, First-Served (FCFS)");
        ScheduleResult r_fcfs = schedule_fcfs(jobs);
        schedule_print_result("FCFS", &r_fcfs, jobs);
        schedule_print_gantt(&r_fcfs);
        schedule_result_free(&r_fcfs);
        steps[1].passed = true;

        print_section("Step 2b: CPU Scheduling - Round-Robin (RR)");
        ScheduleResult r_rr = schedule_rr(jobs, quantum);
        schedule_print_result("Round-Robin", &r_rr, jobs);
        schedule_print_gantt(&r_rr);
        schedule_result_free(&r_rr);
        steps[2].passed = true;

        print_section("Step 2c: CPU Scheduling - Non-preemptive Priority");
        ScheduleResult r_prio = schedule_priority(jobs);
        schedule_print_result("Priority", &r_prio, jobs);
        schedule_print_gantt(&r_prio);
        schedule_result_free(&r_prio);
        steps[3].passed = true;

        print_section("Step 3: Scheduler Metrics Comparison");
        schedule_compare_all(jobs, quantum);
        steps[4].passed = true;

        job_list_free(jobs);
        jobs = NULL;
    }

    /* 4. Banker's Algorithm safe and unsafe */
    print_section("Step 4a: Banker's Algorithm - Safe State Check");
    steps[5].passed = (banker_run_safe() == 0);

    print_section("Step 4b: Banker's Algorithm - Unsafe State & Deadlock Detection");
    steps[6].passed = (banker_run_unsafe() == 0);

    /* 5. Multithreaded logger - IMPORTANT: must run BEFORE fork-based demos */
    print_section("Step 5: Multithreaded Activity Logger (2 Producers, 5 Events, Capacity 5)");
    LoggerConfig lcfg;
    lcfg.num_producers = 2;
    lcfg.events_per_producer = 5;
    lcfg.queue_capacity = 5;
    lcfg.output_path = "logs/integrated_demo.log";
    steps[7].passed = (logger_run(&lcfg) == 0);

    /* 6. POSIX Shared Memory IPC */
    print_section("Step 6: POSIX Shared Memory IPC Demo");
    steps[8].passed = (ipc_demo() == 0);

    /* 7. Worker processes normal and fail */
    print_section("Step 7a: Worker Process Management - Normal Execution");
    steps[9].passed = (worker_run_normal() == 0);

    print_section("Step 7b: Worker Process Management - Failure Handling");
    steps[10].passed = (worker_run_fail() == 0);

    /* 8. Resource limits */
    print_section("Step 8: Resource Limits Demo (RLIMIT_NOFILE)");
    steps[11].passed = (resource_limits_demo() == 0);

    /* 9. Cleanup demo */
    print_section("Step 9: Resource Cleanup Demo & Leak Verification");
    steps[12].passed = (cleanup_demo() == 0);

    /* 10. Summary table */
    print_section("Step 10: Integrated Demonstration Summary");
    printf("%-52s | %-8s\n", "Demonstration Step", "Status");
    print_separator();
    int passed_count = 0;
    int total_steps = 13;
    for (int i = 0; i < total_steps; i++) {
        printf("%-52s | [%s]\n", steps[i].description, steps[i].passed ? "PASS" : "FAIL");
        if (steps[i].passed) {
            passed_count++;
        }
    }
    print_separator();
    printf("Result: %d / %d demo components passed successfully.\n", passed_count, total_steps);

    if (passed_count == total_steps) {
        CC_INFO("Integrated demo SUCCEEDED: all modules operating as expected.");
        return 0;
    } else {
        CC_ERROR("Integrated demo FAILED: %d component(s) encountered errors.", total_steps - passed_count);
        return 1;
    }
}

int cli_run(int argc, char *argv[])
{
    if (argc < 2) {
        CC_ERROR("No command specified.");
        cli_print_help();
        return 1;
    }

    const char *cmd = argv[1];

    /* Help command */
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        cli_print_help();
        return 0;
    }

    /* jobs list --input <file> */
    if (strcmp(cmd, "jobs") == 0) {
        if (argc < 3 || strcmp(argv[2], "list") != 0) {
            CC_ERROR("Unknown or missing subcommand for 'jobs'. Expected: 'jobs list'");
            cli_print_help();
            return 1;
        }

        const char *input = cli_parse_option(argc, argv, "input");
        if (!input) {
            CC_ERROR("Missing required option: --input <file>");
            return 1;
        }

        FILE *f = fopen(input, "r");
        if (!f) {
            CC_ERROR("File not found or unreadable: '%s'", input);
            return 1;
        }
        fclose(f);

        JobList *jobs = job_list_load_csv(input);
        if (!jobs) {
            CC_ERROR("Failed to load jobs from '%s'", input);
            return 1;
        }

        job_list_print(jobs);
        job_list_free(jobs);
        return 0;
    }

    /* schedule fcfs / rr / priority --input <file> [--quantum <n>] */
    if (strcmp(cmd, "schedule") == 0) {
        if (argc < 3) {
            CC_ERROR("Missing algorithm for 'schedule'. Expected: fcfs, rr, or priority");
            cli_print_help();
            return 1;
        }

        const char *algo = argv[2];
        const char *input = cli_parse_option(argc, argv, "input");
        if (!input) {
            CC_ERROR("Missing required option: --input <file>");
            return 1;
        }

        FILE *f = fopen(input, "r");
        if (!f) {
            CC_ERROR("File not found or unreadable: '%s'", input);
            return 1;
        }
        fclose(f);

        JobList *jobs = job_list_load_csv(input);
        if (!jobs) {
            CC_ERROR("Failed to load jobs from '%s'", input);
            return 1;
        }

        if (strcmp(algo, "fcfs") == 0) {
            ScheduleResult res = schedule_fcfs(jobs);
            schedule_print_result("FCFS", &res, jobs);
            schedule_print_gantt(&res);
            schedule_result_free(&res);
            job_list_free(jobs);
            return 0;
        }

        if (strcmp(algo, "rr") == 0) {
            const char *q_str = cli_parse_option(argc, argv, "quantum");
            if (!q_str) {
                CC_ERROR("Missing required option: --quantum <n>");
                job_list_free(jobs);
                return 1;
            }

            int quantum = 0;
            if (!cli_parse_int_option(argc, argv, "quantum", &quantum) || quantum <= 0) {
                CC_ERROR("Invalid quantum: must be greater than 0");
                job_list_free(jobs);
                return 1;
            }

            ScheduleResult res = schedule_rr(jobs, quantum);
            schedule_print_result("Round-Robin", &res, jobs);
            schedule_print_gantt(&res);
            schedule_result_free(&res);
            job_list_free(jobs);
            return 0;
        }

        if (strcmp(algo, "priority") == 0) {
            ScheduleResult res = schedule_priority(jobs);
            schedule_print_result("Priority", &res, jobs);
            schedule_print_gantt(&res);
            schedule_result_free(&res);
            job_list_free(jobs);
            return 0;
        }

        CC_ERROR("Unknown scheduling algorithm '%s'. Expected: fcfs, rr, priority", algo);
        job_list_free(jobs);
        cli_print_help();
        return 1;
    }

    /* compare --quantum <n> --input <file> */
    if (strcmp(cmd, "compare") == 0) {
        const char *q_str = cli_parse_option(argc, argv, "quantum");
        if (!q_str) {
            CC_ERROR("Missing required option: --quantum <n>");
            return 1;
        }

        int quantum = 0;
        if (!cli_parse_int_option(argc, argv, "quantum", &quantum) || quantum <= 0) {
            CC_ERROR("Invalid quantum: must be greater than 0");
            return 1;
        }

        const char *input = cli_parse_option(argc, argv, "input");
        if (!input) {
            CC_ERROR("Missing required option: --input <file>");
            return 1;
        }

        FILE *f = fopen(input, "r");
        if (!f) {
            CC_ERROR("File not found or unreadable: '%s'", input);
            return 1;
        }
        fclose(f);

        JobList *jobs = job_list_load_csv(input);
        if (!jobs) {
            CC_ERROR("Failed to load jobs from '%s'", input);
            return 1;
        }

        schedule_compare_all(jobs, quantum);
        job_list_free(jobs);
        return 0;
    }

    /* logger --producers <n> --events <n> --capacity <n> [--output <path>] */
    if (strcmp(cmd, "logger") == 0) {
        const char *p_str = cli_parse_option(argc, argv, "producers");
        const char *e_str = cli_parse_option(argc, argv, "events");
        const char *c_str = cli_parse_option(argc, argv, "capacity");

        if (!p_str) {
            CC_ERROR("Missing required option: --producers <n>");
            return 1;
        }
        if (!e_str) {
            CC_ERROR("Missing required option: --events <n>");
            return 1;
        }
        if (!c_str) {
            CC_ERROR("Missing required option: --capacity <n>");
            return 1;
        }

        int producers = 0;
        if (!cli_parse_int_option(argc, argv, "producers", &producers) || producers <= 0) {
            CC_ERROR("Invalid option --producers: must be greater than 0");
            return 1;
        }

        int events = 0;
        if (!cli_parse_int_option(argc, argv, "events", &events) || events <= 0) {
            CC_ERROR("Invalid option --events: must be greater than 0");
            return 1;
        }

        int capacity = 0;
        if (!cli_parse_int_option(argc, argv, "capacity", &capacity) || capacity <= 0) {
            CC_ERROR("Invalid option --capacity: must be greater than 0");
            return 1;
        }

        const char *output = cli_parse_option(argc, argv, "output");

        LoggerConfig cfg;
        cfg.num_producers = producers;
        cfg.events_per_producer = events;
        cfg.queue_capacity = capacity;
        cfg.output_path = output ? output : "logs/campuscompute.log";

        return logger_run(&cfg);
    }

    /* ipc demo */
    if (strcmp(cmd, "ipc") == 0) {
        if (argc < 3 || strcmp(argv[2], "demo") != 0) {
            CC_ERROR("Unknown or missing subcommand for 'ipc'. Expected: 'ipc demo'");
            cli_print_help();
            return 1;
        }
        return ipc_demo();
    }

    /* banker safe / unsafe */
    if (strcmp(cmd, "banker") == 0) {
        if (argc < 3) {
            CC_ERROR("Missing subcommand for 'banker'. Expected: 'safe' or 'unsafe'");
            cli_print_help();
            return 1;
        }

        if (strcmp(argv[2], "safe") == 0) {
            return banker_run_safe();
        } else if (strcmp(argv[2], "unsafe") == 0) {
            return banker_run_unsafe();
        } else {
            CC_ERROR("Unknown subcommand for 'banker': '%s'. Expected: 'safe' or 'unsafe'", argv[2]);
            cli_print_help();
            return 1;
        }
    }

    /* worker normal / fail */
    if (strcmp(cmd, "worker") == 0) {
        if (argc < 3) {
            CC_ERROR("Missing subcommand for 'worker'. Expected: 'normal' or 'fail'");
            cli_print_help();
            return 1;
        }

        if (strcmp(argv[2], "normal") == 0) {
            return worker_run_normal();
        } else if (strcmp(argv[2], "fail") == 0) {
            return worker_run_fail();
        } else {
            CC_ERROR("Unknown subcommand for 'worker': '%s'. Expected: 'normal' or 'fail'", argv[2]);
            cli_print_help();
            return 1;
        }
    }

    /* limits nofile */
    if (strcmp(cmd, "limits") == 0) {
        if (argc < 3 || strcmp(argv[2], "nofile") != 0) {
            CC_ERROR("Unknown or missing subcommand for 'limits'. Expected: 'limits nofile'");
            cli_print_help();
            return 1;
        }
        return resource_limits_demo();
    }

    /* cleanup demo */
    if (strcmp(cmd, "cleanup") == 0) {
        if (argc < 3 || strcmp(argv[2], "demo") != 0) {
            CC_ERROR("Unknown or missing subcommand for 'cleanup'. Expected: 'cleanup demo'");
            cli_print_help();
            return 1;
        }
        return cleanup_demo();
    }

    /* demo integrated --input <file> --quantum <n> */
    if (strcmp(cmd, "demo") == 0) {
        if (argc < 3 || strcmp(argv[2], "integrated") != 0) {
            CC_ERROR("Unknown or missing subcommand for 'demo'. Expected: 'demo integrated'");
            cli_print_help();
            return 1;
        }

        const char *input = cli_parse_option(argc, argv, "input");
        if (!input) {
            CC_ERROR("Missing required option: --input <file>");
            return 1;
        }

        FILE *f = fopen(input, "r");
        if (!f) {
            CC_ERROR("File not found or unreadable: '%s'", input);
            return 1;
        }
        fclose(f);

        const char *q_str = cli_parse_option(argc, argv, "quantum");
        if (!q_str) {
            CC_ERROR("Missing required option: --quantum <n>");
            return 1;
        }

        int quantum = 0;
        if (!cli_parse_int_option(argc, argv, "quantum", &quantum) || quantum <= 0) {
            CC_ERROR("Invalid quantum: must be greater than 0");
            return 1;
        }

        return demo_integrated(input, quantum);
    }

    /* Unknown command */
    CC_ERROR("Unknown command: '%s'", cmd);
    cli_print_help();
    return 1;
}
