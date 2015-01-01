#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/*
 * Write "add" to all uevent files in /sys directories.
 * This is useful in getting the kernel to resend hotplug events
 * at boot after the hotplug daemon/helper is ready to run.
 */

int main(void) {
	int main_r = EXIT_SUCCESS;
	glob_t uevents;

	main_r = glob("/sys/class/*/*/uevent", 0, NULL, &uevents);

	if (main_r == GLOB_NOMATCH) { return EXIT_SUCCESS; }
	else if (main_r != 0) { return EXIT_FAILURE; }

	for (int i = 0; i < uevents.gl_pathc; i += 1) {
		int r;
		int uevent;

		errno = 0;
		uevent = open(uevents.gl_pathv[i], O_WRONLY);
		if (errno == ENOENT) { continue; } /* device went away, no problem */
		else if (uevent < 0) { main_r = EXIT_FAILURE; continue; }

		r = write(uevent, "add", 3);
		if (r < 0) { main_r = EXIT_FAILURE; continue; }

		close(uevent);
	}

	globfree(&uevents);

	return main_r;
}
