CC = gcc
CFLAGS = -Iinc `pkg-config --cflags gtk+-3.0 glib-2.0`
LDFLAGS = `pkg-config --libs gtk+-3.0 glib-2.0`

#-Wall -Wextra
# -lpthread

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = bin
INCLUDE_DIR = inc

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/co_test

.DEFAULT_GOAL: all

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

install: $(TARGET)
	install $(TARGET) /usr/local/bin

uninstall:
	rm -rf /usr/local/bin/co_test