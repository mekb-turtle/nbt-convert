#include "util.h"
#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#define BLOCK 512

bool read_file(FILE *fp, file_data *data) {
	void *buffer = NULL, *new_buffer = NULL;
	size_t size = 0, read;

	for (;;) {
		new_buffer = realloc(buffer, size + BLOCK);
		if (!new_buffer) {
			// realloc does not free if it fails
			free(buffer);
			wreturn(false, "realloc");
		}
		buffer = new_buffer;

		// read in blocks
		read = fread(buffer + size, 1, BLOCK, fp);
		size += read;

		if (ferror(fp)) {
			if (buffer) free(buffer);
			eprintf("Read error\n");
			return false;
		}
		if (feof(fp)) break;

		if (!read) {
			if (buffer) free(buffer);
			wreturn(false, "fread");
		}
	}

	// set the file data struct
	data->data = buffer;
	data->size = size;
	return true;
}

bool write_file(FILE *fp, file_data data) {
	if (!data.data) {
		eprintf("Cannot write NULL data\n");
		return false;
	}

	size_t written = fwrite(data.data, 1, data.size, fp);

	if (ferror(fp)) {
		eprintf("Write error\n");
		return false;
	}

	if (written != data.size) {
		eprintf("Write error: Not all data was written\n");
	}

	return !fflush(fp);
}

bool read_filename(char *filename, file_data *data, bool allow_stdin) {
	FILE *fp = NULL;
	if (allow_stdin && filename[0] == '-' && filename[1] == '\0') {
		fp = stdin;
	} else {
		fp = fopen(filename, "rb");
		if (!fp) wreturn(false, filename);
	}

	if (!read_file(fp, data)) return false;

	if (fclose(fp)) wreturn(false, "fclose");

	return true;
}

bool write_filename(char *filename, file_data data, bool allow_stdout, bool *created_file) {
	FILE *fp = NULL;
	if (allow_stdout && filename[0] == '-' && filename[1] == '\0') {
		fp = stdout;
	} else {
		fp = fopen(filename, "wb");
		if (!fp) wreturn(false, filename);
	}

	// for deleting it later
	if (created_file) *created_file = true;

	if (!write_file(fp, data)) return false;

	if (fclose(fp)) wreturn(false, "fclose");

	return true;
}

file_data open_editor(file_data data) {
	char *filename = NULL;
	bool created = false;

	char *editor = getenv("EDITOR");
	if (!editor) {
		eprintf("EDITOR variable not set\n");
		goto clean;
	}

	// get temp filename
	char template[] = P_tmpdir "/nbt-convert.XXXXXX";
	char *filename_ = mktemp(template);
	if (!filename_ || filename_[0] == '\0') wgoto(clean, "mktemp");
	filename = malloc(strlen(filename_) + 8);
	if (!filename) wgoto(clean, "malloc");
	if (!strcpy(filename, filename_)) wgoto(clean, "strcpy");
	if (!strcat(filename, ".nbt")) wgoto(clean, "strcat");

	// write data to the file
	if (!write_filename(filename, data, false, &created)) goto clean;

	// start editor
	pid_t pid = fork();
	if (pid < 0) {
		wgoto(clean, "fork");
	} else if (pid == 0) {
		execvp(editor, (char *[]){editor, filename, NULL});
		exit(0);
		goto clean;
	} else {
		int status;
		waitpid(pid, &status, 0);
		if (!WIFEXITED(status)) {
			eprintf("Editor exited abnormally\n");
			goto clean;
		}
	}

	// read the file
	file_data data_out;
	if (!read_filename(filename, &data_out, false)) goto clean;
	if (created && unlink(filename)) warn("unlink: %s", filename); // delete the temp file
	return data_out;

clean:
	if (created && unlink(filename)) warn("unlink: %s", filename); // delete the temp file
	return NULL_FILE;
}

