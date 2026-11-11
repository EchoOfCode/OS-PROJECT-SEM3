CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wformat=2 -Werror
LDFLAGS = -pthread -lrt
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests/unit
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
SAN_DIR = $(BUILD_DIR)/sanitize
TARGET = $(BUILD_DIR)/campuscompute

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Unit test binaries
TEST_JOBS = $(BUILD_DIR)/test_jobs
TEST_SCHED = $(BUILD_DIR)/test_scheduler
TEST_BANKER = $(BUILD_DIR)/test_banker
TEST_BINS = $(TEST_JOBS) $(TEST_SCHED) $(TEST_BANKER)

.PHONY: all clean test test_units sanitize demo help

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Unit test targets
test_units: $(TEST_BINS)

$(TEST_JOBS): $(TEST_DIR)/test_jobs.c $(OBJ_DIR)/jobs.o $(OBJ_DIR)/common.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $^ -o $@ $(LDFLAGS)

$(TEST_SCHED): $(TEST_DIR)/test_scheduler.c $(OBJ_DIR)/scheduler_common.o $(OBJ_DIR)/scheduler_fcfs.o $(OBJ_DIR)/scheduler_rr.o $(OBJ_DIR)/scheduler_priority.o $(OBJ_DIR)/jobs.o $(OBJ_DIR)/common.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $^ -o $@ $(LDFLAGS)

$(TEST_BANKER): $(TEST_DIR)/test_banker.c $(OBJ_DIR)/banker.o $(OBJ_DIR)/common.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $^ -o $@ $(LDFLAGS)

test: $(TARGET) test_units
	@bash tests/run_tests.sh

sanitize:
	@mkdir -p $(SAN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -fsanitize=address,undefined -g $(SRCS) -o $(SAN_DIR)/campuscompute $(LDFLAGS) -fsanitize=address,undefined
	@echo "Sanitized build at $(SAN_DIR)/campuscompute"

demo: $(TARGET)
	./$(TARGET) demo integrated --input data/jobs.csv --quantum 2

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "CampusCompute Build System"
	@echo "Targets: all clean test test_units sanitize demo help"
