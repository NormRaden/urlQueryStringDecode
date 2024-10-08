#
# Simple Makefile to build urlDecode 

urlDecode: urlDecode.c Makefile
	gcc -s -O3 -o urlDecode urlDecode.c

all: urlDecode

clean:
	rm urlDecode 2>/dev/null

