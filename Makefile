CC = g++
CFLAGS = -W -Wall -O2 -Iinclude -fcompare-debug-second -g -fpermissive --std=c++17

CC_PREFIX = ./src/
INC_PREFIX = ./include/
OBJ_PREFIX = ./obj
BUILD_PREFIX = ./bin
CC_LINKS = -lseccomp -lpthread

TARGET = realcore
OBJECTS = main.o runner/child.o runner/seccomp.o runner/timeout.o runner/watch.o util.o
# HEADERS = box.hpp judge.hpp runner.hpp security.hpp timeout.

$(TARGET): $(OBJECTS)
	cd $(OBJ_PREFIX) && $(CC) $(CFLAGS) -o $@ $^ $(CC_LINKS) && mv $(TARGET) ../$(BUILD_PREFIX)

$(OBJECTS):
	$(CC) $(CFLAGS) -o $(OBJ_PREFIX)/$@ -c $(CC_PREFIX)$(@:o=cpp) $(CC_LINKS)

# $(HEADERS):
# 	$(CC) $(CFLAGS) -o $(OBJ_PREFIX)/$@ -c $(CC_PREFIX)$(@:o=hpp) $(CC_LINKS)

init:
	mkdir -p $(BUILD_PREFIX) && mkdir -p $(OBJ_PREFIX)
	mkdir -p $(OBJ_PREFIX)/runner

all: init $(TARGET)
