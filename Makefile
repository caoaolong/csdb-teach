SRC = ./src
BUILD = ./build
INC = $(SRC)/inc
CFLAGS = -g -I$(INC)
NAME = csdb

$(BUILD)/%.o: $(SRC)/%.c
	$(shell mkdir -p $(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all clean reset

all: $(BUILD)/db_file.o $(BUILD)/db_file_page.o $(BUILD)/db_file_block.o $(BUILD)/db_row.o \
	$(BUILD)/ds/sl_list.o \
	$(BUILD)/ds/array.o \
	$(BUILD)/lib/str.o $(BUILD)/lib/log.o $(BUILD)/lib/code.o \
	$(BUILD)/main.o
	$(CC) -o $(BUILD)/$(NAME) $^

clean:
	@rm -rf $(BUILD)/*.o $(BUILD)/$(NAME)

reset:
	@rm -rf ./data/*