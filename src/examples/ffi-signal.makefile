#
# ffi-signal.makefile
#
# Build script for ffi-signal.so. Please follow instructions
# in ffi-signal.k.
#

INCLUDES := -I..
CFLAGS := -O2 -g -std=gnu99 -Wall -m32 -shared -fPIC \
          -DKLISP_USE_POSIX -DKUSE_LIBFFI=1

ffi-signal.so: ffi-signal.c
	gcc $(CFLAGS) $(INCLUDES) -o $@ ffi-signal.c

clean:
	rm -f ffi-signal.so
