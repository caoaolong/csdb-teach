CC = gcc
CFLAGS = -g -Wall -Wextra -std=c11
BUILD_DIR = build
TARGET = $(BUILD_DIR)/csdbm
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
