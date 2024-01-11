#include "util.h"

#ifndef CONVERT_H
#define CONVERT_H
// TODO
enum data_type {
	BYTE, SHORT, INT, LONG, FLOAT, DOUBLE, STRING
};

file_data convert(file_data input, enum mode mode);
#endif
