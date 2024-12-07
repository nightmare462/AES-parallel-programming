CC = gcc
CFLAGS = -g -pthread
SOURCE_DIR = src
BUILD_DIR = build
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/AES.o

all: $(BUILD_DIR) aes

$(BUILD_DIR):
	mkdir -pv $(BUILD_DIR)

$(BUILD_DIR)/main.o: $(SOURCE_DIR)/main.c $(SOURCE_DIR)/AES.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/AES.o: $(SOURCE_DIR)/AES.c $(SOURCE_DIR)/AES.h
	$(CC) $(CFLAGS) -c $< -o $@

aes: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/aes

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean