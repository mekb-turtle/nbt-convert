#include "convert.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

file_data convert(file_data input, enum mode mode) {
	// TODO: implement NBT conversion
	switch (mode) {
		case TO_STRING:
			break;
		case TO_BINARY:
			break;
		default:
			return NULL_FILE;
	}

	// for testing, clone the data
	file_data output = {0};
	output.size = input.size;
	output.data = malloc(input.size);
	if (!output.data) wreturn(NULL_FILE, "malloc");
	if (!memcpy(output.data, input.data, input.size)) wreturn(NULL_FILE, "memcpy");

	return output;
}
