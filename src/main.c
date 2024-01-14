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
#include <fcntl.h>
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
#define TTY_ERROR 3

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
	if (argc != 3 && argc != 4) goto invalid;

	enum mode mode;

	if (match_start(argv[1], "stringify"))
		mode = TO_STRING;
	else if (match_start(argv[1], "binary"))
		mode = TO_BINARY;
	else if (match_start(argv[1], "edit"))
		mode = EDIT;
	else
		goto invalid;

	char *input = NULL, *output = NULL;

	input = argv[2];

	if (argc > 3)
		output = argv[3];
	else if (mode == EDIT)
		output = input; // output to same file
	else
		goto invalid;

	int tty_fd = -1;
	FILE *tty_fp = NULL;
	file_data input_data = NULL_FILE, output_data = NULL_FILE, string_data = NULL_FILE, temp_data = NULL_FILE;

	if (!read_filename(input, &input_data, true)) return FILE_ERROR;

	int res = 0;

	if (mode == EDIT) {
		tty_fd = open("/dev/tty", O_RDWR);

		if (tty_fd == -1 || !isatty(tty_fd)) {
			res = TTY_ERROR;
			wgoto(clean, "/dev/tty");
		}

		FILE *tty_fp = fdopen(tty_fd, "rwb");

		if (!tty_fp) {
			res = TTY_ERROR;
			wgoto(clean, "/dev/tty");
		}

		// convert to string
		string_data = convert(input_data, TO_STRING);
		if (!string_data.data) {
			res = CONVERSION_ERROR;
			goto clean;
		}

		free_file(&input_data);

	editor:
		// edit the file
		temp_data = open_editor(string_data);
		if (!temp_data.data) {
			res = FILE_ERROR;
			goto clean;
		}

		free_file(&string_data);
		string_data = temp_data;
		temp_data.data = NULL; // prevent double-free

		// convert back to binary
		output_data = convert(string_data, TO_BINARY);
		if (!output_data.data) {
			// ask user if they want to re-open the editor

			// switch terminal mode to raw
			struct termios termios, termios_set, termios_temp;
			if (tcgetattr(tty_fd, &termios)) wgoto(no, "tcgetattr");
			termios_set = termios;

			// set termios to raw
			cfmakeraw(&termios_set);
			if (tcsetattr(tty_fd, TCSANOW, &termios_set) || tcgetattr(tty_fd, &termios_temp) || !termios_cmp(&termios_set, &termios_temp)) {
				tcsetattr(tty_fd, TCSANOW, &termios);
				res = TTY_ERROR;
				wxgoto(clean, "tcsetattr");
			}

			eprintf("Do you want to continue editing the file? [Y/n] ");
			fflush(stderr);

			for (;;) {
				if (feof(tty_fp) || ferror(tty_fp)) {
				no:
					eprintf("\r\n");
					if (tcsetattr(tty_fd, TCSANOW, &termios)) warnx("tcsetattr");
					res = TTY_ERROR;
					goto clean;
				}

				fpurge(tty_fp);
				int answer = fgetc(tty_fp);
				switch (answer) {
					case 'y':
					case 'Y':
					case '\n':
					case '\r':
					case ' ':
						eprintf("\r\n");
						if (tcsetattr(tty_fd, TCSANOW, &termios)) {
							res = TTY_ERROR;
							wxgoto(clean, "tcsetattr");
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

		fclose(tty_fp);
		close(tty_fd);
		tty_fp = NULL;
		tty_fd = -1;
	} else {
		// run conversion
		if (!(output_data = convert(input_data, mode)).data) {
			res = CONVERSION_ERROR;
			goto clean;
		}

		free_file(&input_data);
	}

	if (!write_filename(output, output_data, true, NULL)) return FILE_ERROR;

	res = 0;
clean:
	if (tty_fp) fclose(tty_fp);
	if (tty_fd != -1) close(tty_fd);
	free_file(&input_data);
	free_file(&output_data);
	free_file(&string_data);
	free_file(&temp_data);
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
