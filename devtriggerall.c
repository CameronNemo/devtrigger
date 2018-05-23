#include <glob.h>
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

int trigger_glob(const char* g);
int write_uevent(char path[FILENAME_MAX]);

/*
 * Write "add" to uevent files for all devices present.
 * This is useful in getting the kernel to resend hotplug events
 * at boot after the hotplug daemon/helper is ready to run.
 */

int main(void) {
	openlog("devtriggerall", LOG_CONS | LOG_PID | LOG_PERROR, LOG_DAEMON);

	if (trigger_glob("/sys/class/*/*/uevent") == 0) {
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}

/**
 * trigger_glob:
 * @g: glob pattern that matches to desired uevent files
 *
 * Triggers add events for all uevent files matching the pattern.
 *
 * Returns: 0 on success, -1 on error
 **/
int trigger_glob(const char* g) {
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
 * Writes to a uevent file in sysfs, triggering an add event.
 *
 * Returns: 1 on successful write, 0 if file not found, -1 on error
 **/
int write_uevent(char path[FILENAME_MAX]) {
	int uevent;
	int r;

	/* try to open uevent file, if there is one */
        errno = 0;
	uevent = open(path, O_WRONLY);

	if (uevent >= 0) {
                /* opened the file, so write the add event */
                if (write(uevent, "add", 3) == 3) {
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

/* not currently used or working. recursively triggers add events.
 * intended to be used in /sys/devices/ directory. */
int trigger_dir(char dir[FILENAME_MAX]) {
	int r;
	int total_events = 0;
	DIR *d;
	struct dirent *f;

	/* transform directory path to uevent file path, then try to write to it */
	char uevent_path[FILENAME_MAX];
	strcpy(uevent_path, dir);
        strcat(uevent_path, "/uevent");
	r = write_uevent(uevent_path);

	if (r == 0)
		syslog(LOG_DEBUG, "looking for subdirectories in %s", dir);
	else
		return r;

	/* try opening the directory */
	if ((d = opendir(dir)) == NULL) {
		syslog(LOG_ERR, "could not open directory: %s", dir);
		return -1;
	}

	while ((f = readdir(d)) != NULL) {
		/* skip "." and ".." */
		if(strcmp(f->d_name, ".") == 0 || strcmp(f->d_name, "..") == 0)
			continue;

		int events;
		char fpath[FILENAME_MAX];
		strcpy(fpath, dir);
		strcat(fpath, "/");
		strcat(fpath, f->d_name);

		/* filter for directories and links to dirs */
		if (f->d_type == DT_LNK) {
			/* check if the link points to a directory  */
			struct stat buf;
			if (stat(fpath, &buf) == -1) {
				syslog(LOG_ERR, "stat failed for file: %s", fpath);
				r = -1;
				errno = 0;
				continue;
			} else if (!S_ISDIR(buf.st_mode)) {
				/* not a directory, so skip it */
				continue;
			}
		} else if (f->d_type != DT_DIR) {
			continue;
		}

		/* recursively call trigger_dir */
		events = trigger_dir(fpath);
		if (events < 0)
			r = -1;
		else
			total_events += events;
	}

	if (r != 0) {
		syslog(LOG_ERR, "error while triggering add events in %s", dir);
		return r;
	} else {
		return total_events;
	}
}
