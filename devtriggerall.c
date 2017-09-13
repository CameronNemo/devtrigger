#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/*
 * Write "add" to uevent files for all devices present.
 * This is useful in getting the kernel to resend hotplug events
 * at boot after the hotplug daemon/helper is ready to run.
 */

int main(void) {
	int main_r = EXIT_SUCCESS;
	glob_t uevents;

	openlog("devtriggerall", LOG_CONS | LOG_PID | LOG_PERROR, LOG_DAEMON);
	main_r = glob("/sys/class/*/*/uevent", 0, NULL, &uevents);

	if (main_r == GLOB_NOMATCH) {
		syslog(LOG_NOTICE, "glob did not find any matches");
		return EXIT_SUCCESS;
	} else if (main_r != 0) {
		syslog(LOG_ERR, "glob returned error: %d", main_r);
		return EXIT_FAILURE;
	}

	for (int i = 0; i < uevents.gl_pathc; i += 1) {
		int r;
		int uevent;

		errno = 0;
		uevent = open(uevents.gl_pathv[i], O_WRONLY);
		if (errno == ENOENT) {
			/* device went away, no problem */
			continue;
		} else if (uevent < 0) {
			syslog(LOG_ERR, "device could not be opened: %s", uevents.gl_pathv[i]);
			main_r = EXIT_FAILURE;
			continue;
		}

		r = write(uevent, "add", 3);
		if (r < 0) {
			syslog(LOG_ERR, "could not write to device: %s", uevents.gl_pathv[i]);
			main_r = EXIT_FAILURE;
			continue;
		}

		close(uevent);
	}

	globfree(&uevents);

	return main_r;
}
