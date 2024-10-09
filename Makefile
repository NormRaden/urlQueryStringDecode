#
# Simple Makefile to build urlQueryStringDecode 

urlQueryStringDecode: urlQueryStringDecode.c Makefile
	gcc -s -O3 -o urlQueryStringDecode urlQueryStringDecode.c

all: urlQueryStringDecode

clean:
	rm urlQueryStringDecode 2>/dev/null

