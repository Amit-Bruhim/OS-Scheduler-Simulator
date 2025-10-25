# Makefile for OS Scheduler Simulator

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Source and binary
SRC = src/CPU-Scheduler.c
BIN_DIR = bin
BIN = $(BIN_DIR)/scheduler

# Default target: compile and run
.PHONY: run clean

run: $(BIN)
	@./$(BIN) data/processes.csv 2

# Compile binary and create bin directory if missing
$(BIN): $(SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

# Clean target: remove bin directory
clean:
	@rm -rf $(BIN_DIR)
