CC = g++
CFLAGS = -W -Wall -O2 -Iinclude

CC_PREFIX = ./src/
INC_PREFIX = ./include/
OBJ_PREFIX = ./obj
BUILD_PREFIX = ./bin
CC_LINKS = -lseccomp

TARGET = realcore
OBJECTS = judge.o security.o security_x64.o box.o box_x64.o main.o

$(TARGET): $(OBJECTS)
	cd $(OBJ_PREFIX) && $(CC) $(CFLAGS) -o $@ $^ $(CC_LINKS) && mv $(TARGET) ../$(BUILD_PREFIX)

judge.o:
	$(CC) $(CFLAGS) -o $(OBJ_PREFIX)/judge.o -c $(CC_PREFIX)judge.cpp $(CC_LINKS)

security.o:
	$(CC) $(CFLAGS) -o $(OBJ_PREFIX)/security.o -c $(CC_PREFIX)security.cpp $(CC_LINKS)

security_x64.o:
	$(CC) $(CFLAGS) -o $(OBJ_PREFIX)/security_x64.o -c $(CC_PREFIX)security_x64.cpp $(CC_LINKS)

box.o:
	$(CC) $(CFLAGS) -o $(OBJ_PREFIX)/box.o -c $(CC_PREFIX)box.cpp $(CC_LINKS)

box_x64.o:
	$(CC) $(CFLAGS) -o $(OBJ_PREFIX)/box_x64.o -c $(CC_PREFIX)box_x64.cpp $(CC_LINKS)

main.o:
	$(CC) $(CFLAGS) -o $(OBJ_PREFIX)/main.o -c $(CC_PREFIX)main.cpp $(CC_LINKS)

init:
	mkdir -p $(BUILD_PREFIX) && mkdir -p $(OBJ_PREFIX)

all: init $(TARGET)
