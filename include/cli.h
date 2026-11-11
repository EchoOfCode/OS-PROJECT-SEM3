/*
 * cli.h - Command-line interface definitions for CampusCompute
 */
#ifndef CLI_H
#define CLI_H

#include "common.h"

int cli_run(int argc, char *argv[]);
void cli_print_help(void);
const char *cli_parse_option(int argc, char *argv[], const char *name);
bool cli_parse_int_option(int argc, char *argv[], const char *name, int *out);
int demo_integrated(const char *input_file, int quantum);

#endif /* CLI_H */
