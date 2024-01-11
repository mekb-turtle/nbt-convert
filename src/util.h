#include <stdbool.h>
#include <stdio.h>

#ifndef UTIL_H
#define UTIL_H
#define wreturn(returnval, ...) \
	{                           \
		warn(__VA_ARGS__);      \
		return returnval;       \
	}
#define wgoto(label, ...)  \
	{                      \
		warn(__VA_ARGS__); \
		goto label;        \
	}
#define wxreturn(returnval, ...) \
	{                           \
		warnx(__VA_ARGS__);      \
		return returnval;       \
	}
#define wxgoto(label, ...)  \
	{                      \
		warnx(__VA_ARGS__); \
		goto label;        \
	}

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

enum mode {
	TO_STRING,
	TO_BINARY,
	EDIT
};

#define NULL_FILE ((struct file_data){.data = NULL, .size = 0})

typedef struct file_data {
	void *data;
	size_t size;
} file_data;

bool read_file(FILE *fp, file_data *data);
bool write_file(FILE *fp, file_data data);
bool read_filename(char *filename, file_data *data, bool allow_stdin);
bool write_filename(char *filename, file_data data, bool allow_stdout, bool *created_file);
file_data open_editor(file_data data);
#endif
