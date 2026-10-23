CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic
TARGET = scheduler
SOURCES = main.c metrics.c fcfs_sjf.c srtf_priority.c round_robin.c gantt.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c scheduler.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET) test_fcfs test_priority test_rr
