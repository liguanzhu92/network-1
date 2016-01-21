# Makefile for client and server

CC = gcc
TARGET = count
CFLAGS = -O3
LIBS = 
RM = rm

all: $(TARGET)
	
$(TARGET):$(TARGET).c
	$(CC) $(CFLAGS) -o $@ $(TARGET).c $(LIBS)

clean:
	$(RM) $(TARGET)
