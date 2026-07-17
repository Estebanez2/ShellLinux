.RECIPEPREFIX := >

CC := gcc
CFLAGS := -Wall -Wextra -g
LDFLAGS := -pthread
TARGET := shell
SOURCES := Shell_project.c job_control.c
HEADERS := job_control.h parse_redir.h

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
>$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
>./$(TARGET)

clean:
>rm -f $(TARGET) *.o hup.txt
