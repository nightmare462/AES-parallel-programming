CC = gcc
CFLAGS = -g
CFLAGS_OPENMP = -g -fopenmp
CFLAGS_PTHREAD = -g -pthread

SOURCE_DIR = src
BUILD_DIR = build

OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/AES.o
OBJECTS_OPENMP = $(BUILD_DIR)/main_openmp.o $(BUILD_DIR)/AES.o
OBJECTS_PTHREAD = $(BUILD_DIR)/main_pthread.o $(BUILD_DIR)/AES.o

all: $(BUILD_DIR) aes aes_openmp aes_pthread

$(BUILD_DIR):
	mkdir -pv $(BUILD_DIR)

$(BUILD_DIR)/main.o: $(SOURCE_DIR)/main.c $(SOURCE_DIR)/AES.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main_openmp.o: $(SOURCE_DIR)/main_openmp.c $(SOURCE_DIR)/AES.h
	$(CC) $(CFLAGS_OPENMP) -c $< -o $@

$(BUILD_DIR)/main_pthread.o: $(SOURCE_DIR)/main_pthread.c $(SOURCE_DIR)/AES.h
	$(CC) $(CFLAGS_PTHREAD) -c $< -o $@

$(BUILD_DIR)/AES.o: $(SOURCE_DIR)/AES.c $(SOURCE_DIR)/AES.h
	$(CC) $(CFLAGS) -c $< -o $@

aes: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/aes

aes_openmp: $(OBJECTS_OPENMP)
	$(CC) $(CFLAGS_OPENMP) $^ -o $(BUILD_DIR)/aes_openmp

aes_pthread: $(OBJECTS_PTHREAD)
	$(CC) $(CFLAGS_PTHREAD) $^ -o $(BUILD_DIR)/aes_pthread

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean