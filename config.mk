TARGET = nbt-convert
VERSION = 1.0.0

EXTRA_SRC_FILES =
EXTRA_BINARY_FILES =
CFLAGS += -Wall
ifeq ($(shell uname -s),Linux)
	LDLIBS += -lbsd
endif
