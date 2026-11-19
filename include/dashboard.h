/*
 * dashboard.h - Terminal User Interface (TUI) Dashboard for CampusCompute
 */
#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "common.h"

/* Run the interactive or automated Terminal Dashboard */
int dashboard_run(bool auto_demo);

/* Run the integrated demonstration with enhanced visual TUI cards */
int dashboard_run_integrated_tui(const char *input_file, int quantum, bool automated);

/* Print dashboard top header banner with system and kernel telemetry */
void dashboard_show_header(void);

#endif /* DASHBOARD_H */
