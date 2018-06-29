#include <stdlib.h>
#include <glob.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define UTI_ACTION_DEFAULT "add"
#define UTI_ACTION_REMOVE "remove"

#define UTI_VERSION "0.0.1"
#define UTI_OPT_STR "vhdrs:"

static const char* action;
static int log_priority = 0;

void print_usage(const char *name);
void print_version(const char *name);
int trigger_subsystem(const char* subsystem);
int trigger_glob(const char* pattern);
int write_uevent(const char* path);

int main(int argc, char **argv) {
	int   opt;
	char *subsystem = NULL;

	char *name = basename(argv[0]);

	while ((opt = getopt(argc, argv, UTI_OPT_STR)) != -1) {
		switch (opt) {
			case 'v':
				print_version(name);
				return EXIT_SUCCESS;
			case 'd':
				log_priority = 1;
			case 'r':
				action = UTI_ACTION_REMOVE;
				break;
			case 's':
				subsystem = optarg;
				break;
			case 'h':
				print_usage(name);
				return EXIT_SUCCESS;
			case '?':
				print_usage(name);
			default:
				return EXIT_FAILURE;
		}
	}

	if (optind < argc) {
		print_usage(name);
		return EXIT_FAILURE;
	}

	if (!action)
		action = UTI_ACTION_DEFAULT;

	if (!subsystem)
		subsystem = "*";

	return trigger_subsystem(subsystem) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

void print_usage(const char *name) {
	fprintf(stderr, "Usage: %s [OPTIONS]\n\n", name);
	fprintf(stderr, "\t-h\t\tshow this help message\n");
	fprintf(stderr, "\t-v\t\tprint version string\n");
	fprintf(stderr, "\t-d\t\tenable debugging messages\n");
	fprintf(stderr, "\t-r\t\ttrigger a remove event rather than add\n");
	fprintf(stderr, "\t-s SUBSYSTEM\ttrigger events for the specified subsystem (default: all)\n");
	fprintf(stderr, "\n");
}

void print_version(const char *name) {
	fprintf(stderr, "%s (%s)\n", name, UTI_VERSION);
}

/**
 * trigger_subsystem:
 *
 * @subsystem:	glob pattern to match to subsystem names
 *
 * Triggers events for devices in /sys/class/ and /sys/bus/.
 *
 * Returns: 0 on success, -1 on failure
 **/
int trigger_subsystem(const char* subsystem) {
	int r, bytes;
	char* pattern;

	pattern = malloc(FILENAME_MAX);
	if (!pattern)
		return -1;
	r = 0;

	bytes = snprintf(pattern, FILENAME_MAX, "/sys/class/%s/*/uevent", subsystem);
	if (bytes < 0 || bytes >= FILENAME_MAX)
		r = -1;
	else if (trigger_glob(pattern) != 0)
		r = -1;

	bytes = snprintf(pattern, FILENAME_MAX, "/sys/bus/%s/devices/*/uevent", subsystem);
	if (bytes < 0 || bytes >= FILENAME_MAX)
		r = -1;
	else if (trigger_glob(pattern) != 0)
		r = -1;

	if (pattern)
		free(pattern);
	return r;
}

/**
 * trigger_glob:
 *
 * @g: glob pattern that matches to desired uevent files
 *
 * Triggers add events for all uevent files matching the pattern.
 *
 * Returns: 0 on success, -1 on error
 **/
int trigger_glob(const char* pattern) {
	int r;
	glob_t uevents;

	r = glob(pattern, 0, NULL, &uevents);

	if (r == GLOB_NOMATCH) {
		if (log_priority > 0)
			fprintf(stderr, "glob: %m: %s\n", pattern);
		return 0;
	} else if (r != 0) {
		perror("glob");
		return -1;
	}

	for (int i = 0; i < uevents.gl_pathc; i += 1) {
		if (write_uevent(uevents.gl_pathv[i]) < 0)
			r = -1;
	}

	globfree(&uevents);
	return r;
}

/**
 * write_uevent:
 *
 * @path: path of a uevent file to write to
 *
 * Writes to a uevent file in sysfs, triggering an add or remove event.
 *
 * Returns: 1 on successful write, 0 if file not found, -1 on error
 **/
int write_uevent(const char* path) {
	int uevent;
	int len;
	int r;

	if (log_priority > 0)
		fprintf(stderr, "%s %s\n", action, path);

	/* try to open uevent file, if there is one */
	errno = 0;
	uevent = open(path, O_WRONLY);

	if (uevent >= 0) {
		/* opened the file, so write the event */
		len = strlen(action);
		if (write(uevent, action, len) == len) {
			/* wrote to device */
			r = 1;
		} else {
			perror("write");
			r = -1;
		}
		close(uevent);
	} else {
		/* couldn't open uevent file, but why? */
		if (errno == ENOENT) {
			/* there was no uevent file, this is not an error */
			r = 0;
		} else {
			perror("open");
			r = -1;
		}
	}

	if (errno != 0)
		errno = 0;

	return r;
}
