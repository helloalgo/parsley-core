CC = g++
CFLAGS = -W -Wall -O2 -Iinclude -fpermissive

CC_PREFIX = ./src/
INC_PREFIX = ./include/
OBJ_PREFIX = ./obj
BUILD_PREFIX = ./bin
CC_LINKS = -lseccomp

TARGET = realcore
OBJECTS = judge.o security.o security_x64.o box.o box_x64.o main.o

$(TARGET): $(OBJECTS)
	cd $(OBJ_PREFIX) && $(CC) $(CFLAGS) -o $@ $^ $(CC_LINKS) && mv $(TARGET) ../$(BUILD_PREFIX)

$(OBJECTS):
	$(CC) $(CFLAGS) -o $(OBJ_PREFIX)/$@ -c $(CC_PREFIX)$(@:o=cpp) $(CC_LINKS)

init:
	mkdir -p $(BUILD_PREFIX) && mkdir -p $(OBJ_PREFIX)

all: init $(TARGET)
