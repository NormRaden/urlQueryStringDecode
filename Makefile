#
# Makefile to build urlQueryStringDecode 

CC=gcc
CFLAGS=-s -O3
TARGET=urlQueryStringDecode
SOURCES=urlQueryStringDecode.c

.PHONY: all clean verify

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

verify: all
	sh testUrlQueryStringDecode.sh

