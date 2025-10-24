CC=gcc
CFLAGS=-Wall -g

TARGET=ex3

part1:
	$(CC) $(CFLAGS) ex3.c -o $(TARGET)
	./$(TARGET) Focus-Mode 3 4

part2:
	@$(CC) $(CFLAGS) ex3.c -o $(TARGET)
	@./$(TARGET) CPU-Scheduler processes_test.csv 5