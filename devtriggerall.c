#include <glob.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define VERSION "0.0.1"
#define OPT_STR "vhda:s:"

static const char *action;
static int debug;

void print_usage(char *name);
void print_version(char *name);
int trigger_glob(char g[FILENAME_MAX]);
int write_uevent(char path[FILENAME_MAX]);

/*
 * Write "add" to uevent files for all devices present.
 * This is useful in getting the kernel to resend hotplug events
 * at boot after the hotplug daemon/helper is ready to run.
 */

int main(int argc, char **argv) {
	int   opt;
	int   r;
	char  g[FILENAME_MAX] = "/sys/class/*/*/uevent";
	char *name = basename(argv[0]);

	while ((opt = getopt(argc, argv, OPT_STR)) != -1) {
		switch (opt) {
			case 'v':
				print_version(name);
				return EXIT_SUCCESS;
			case 'd':
				debug = 1;
				break;
			case 'a':
				action = optarg;
				break;
			case 's':
				/* 'all' is a special case */
				if (strncmp(optarg, "all", 4) == 0)
					break;
				r = snprintf(g, FILENAME_MAX,
				             "/sys/class/%s/*/uevent", optarg);
				if (r < 0 || r >= FILENAME_MAX)
					return EXIT_FAILURE;
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
		action = "add";

	openlog("devtriggerall", LOG_CONS | LOG_PID | LOG_PERROR, LOG_DAEMON);

	if (trigger_glob(g) == 0) {
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}

void print_usage(char *name) {
	fprintf(stderr, "Usage: %s [OPTIONS]\n\n", name);
	fprintf(stderr, "\t-h\t\tshow this help message\n");
	fprintf(stderr, "\t-v\t\tprint version string\n");
	fprintf(stderr, "\t-d\t\tenable debugging messages\n");
	fprintf(stderr, "\t-s SUBSYSTEM\ttrigger events for the specified subsystem (default: all)\n");
	fprintf(stderr, "\t-a ACTION\ttrigger the specified event (default: add)\n");
	fprintf(stderr, "\n");
}

void print_version(char *name) {
	fprintf(stderr, "%s (%s)\n", name, VERSION);
}

/**
 * trigger_glob:
 * @g: glob pattern that matches to desired uevent files
 *
 * Triggers add events for all uevent files matching the pattern.
 *
 * Returns: 0 on success, -1 on error
 **/
int trigger_glob(char g[FILENAME_MAX]) {
	int r;
	glob_t uevents;

	r = glob(g, 0, NULL, &uevents);

	if (r == GLOB_NOMATCH) {
		syslog(LOG_DEBUG, "glob did not find any matches for pattern %s", g);
		return 0;
	} else if (r != 0) {
		syslog(LOG_ERR, "glob returned error for pattern %s", g);
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
 * @path: path of a uevent file to write to
 *
 * Writes to a uevent file in sysfs, triggering an add or remove event.
 *
 * Returns: 1 on successful write, 0 if file not found, -1 on error
 **/
int write_uevent(char path[FILENAME_MAX]) {
	int uevent;
	int len;
	int r;

	/* try to open uevent file, if there is one */
	errno = 0;
	uevent = open(path, O_WRONLY);

	if (uevent >= 0) {
		/* opened the file, so write the event */
		len = strlen(action);
		if (write(uevent, action, len) == len) {
			syslog(LOG_DEBUG, "wrote to device: %s", path);
			r = 1;
		} else {
			syslog(LOG_ERR, "could not write to device: %s", path);
			r = -1;
		}
		close(uevent);
	} else {
		/* couldn't open uevent file, but why? */
		if (errno == ENOENT) {
			/* there was no uevent file, this is not an error */
			r = 0;
		} else {
			syslog(LOG_ERR, "could not open device: %s", path);
			r = -1;
		}
	}

	if (errno != 0)
		errno = 0;

	return r;
}
