CC = gcc
CFLAGS = -g -Wall -Wextra -std=c11 -Iinc
BUILD_DIR = build
TARGET = $(BUILD_DIR)/csdbm
SRC = main.c $(wildcard src/*.c)

all: $(TARGET)

$(TARGET): $(SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	# 只删构建产物，保留数据文件 (*.csv / *.idx 不删)
	rm -f $(TARGET) $(TARGET).exe $(BUILD_DIR)/*.tmp *.tmp

.PHONY: all clean
