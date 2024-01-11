#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <err.h>
#ifdef __linux__
#include <bsd/stdio.h>
#endif
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include "util.h"
#include "convert.h"

bool match_start(char *input, char *match) {
	size_t len = strlen(input);
	if (len > strlen(match)) return false;
	return strncmp(input, match, len) == 0;
}

FILE *open_file(char *path, FILE *dash) {
	if (path[0] == '-' && path[1] == '\0') {
		return dash;
	}

	FILE *fp = fopen(path, "rb");
	if (!fp) {
		warn("%s", path);
		return NULL;
	}

	return fp;
}

// https://minecraft.fandom.com/wiki/NBT_format

#define FILE_ERROR 2
#define CONVERSION_ERROR 1

enum prompt_choice {
	UNSET,
	NO,
	YES
};

bool termios_cmp(struct termios *t1, struct termios *t2) {
	if (t1->c_iflag != t2->c_iflag) return false;
	if (t1->c_oflag != t2->c_oflag) return false;
	if (t1->c_cflag != t2->c_cflag) return false;
	if (t1->c_lflag != t2->c_lflag) return false;
	return true;
}

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 4) goto invalid;

	enum mode mode;

	if (match_start(argv[1], "stringify"))
		mode = TO_STRING;
	else if (match_start(argv[1], "binary"))
		mode = TO_BINARY;
	else if (match_start(argv[1], "edit"))
		mode = EDIT;
	else
		goto invalid;

	char *input = "-";
	char *output = "-";

	if (argc > 2) input = argv[2];
	if (argc > 3) output = argv[3];
	else if (mode == EDIT)
		output = input;

	file_data input_data = NULL_FILE, output_data = NULL_FILE, string_data = NULL_FILE, temp_data = NULL_FILE;

	if (!read_filename(input, &input_data, true)) return FILE_ERROR;

	int res = 0;

	if (mode == EDIT) {
		// convert to string
		if (!(string_data = convert(input_data, TO_STRING)).data) {
			res = CONVERSION_ERROR;
			goto clean;
		}

		free(input_data.data);
		input_data.data = NULL;

	editor:
		// edit the file
		if (!(temp_data = open_editor(string_data)).data) {
			res = FILE_ERROR;
			goto clean;
		}

		free(string_data.data);
		string_data = temp_data;

		// convert back to binary
		if (!(output_data = convert(string_data, TO_BINARY)).data || 1) {
			// ask user if they want to re-open the editor

			// switch terminal mode to raw
			struct termios termios, termios_set, termios_temp;
			if (tcgetattr(STDIN_FILENO, &termios)) wgoto(no, "tcgetattr");
			termios_set = termios;

			// set termios to raw
			cfmakeraw(&termios_set);
			if (tcsetattr(STDIN_FILENO, TCSANOW, &termios_set) || tcgetattr(STDIN_FILENO, &termios_temp) || !termios_cmp(&termios_set, &termios_temp)) {
				tcsetattr(STDIN_FILENO, TCSANOW, &termios);
				warnx("tcsetattr");
				res = CONVERSION_ERROR;
				goto clean;
			}

			printf("Do you want to continue editing the file? [Y/n] ");
			fflush(stdout);

			for (;;) {
				if (feof(stdin) || ferror(stdin)) {
				no:
					printf("\r\n");
					if (tcsetattr(STDIN_FILENO, TCSANOW, &termios)) warnx("tcsetattr");
					res = CONVERSION_ERROR;
					goto clean;
				}

				fpurge(stdin);
				int answer = fgetc(stdin);
				switch (answer) {
					case 'y':
					case 'Y':
					case '\n':
					case '\r':
					case ' ':
						printf("\r\n");
						if (tcsetattr(STDIN_FILENO, TCSANOW, &termios)) {
							warnx("tcsetattr");
							res = CONVERSION_ERROR;
							goto clean;
						}
						goto editor;
					case 'n':
					case 'N':
					case '\x03': // ^C
					case '\x04': // ^D
					case '\x1a': // ^Z
						goto no;
					default:
						break;
				}
			}
		}

		free(string_data.data);
		string_data.data = NULL;
	} else {
		// run conversion
		if (!(output_data = convert(input_data, mode)).data) {
			res = CONVERSION_ERROR;
			goto clean;
		}

		free(input_data.data);
		input_data.data = NULL;
	}

	if (!write_filename(output, output_data, true, NULL)) return FILE_ERROR;

	res = 0;
clean:
	free(input_data.data);
	free(output_data.data);
	return res;

invalid:
	char *bin = argv[0];
	// get filename of bin path
	char *slash = strrchr(bin, '/');
	bin = slash ? slash + 1 : bin;

	eprintf("Usage:\n\
%s string <input> <output>\n\
%s binary <input> <output>\n\
%s edit <input> <output>\n",
	        bin, bin, bin);
	return 1;
}
