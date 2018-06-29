# devtrigger

This tool triggers events for devices that are currently present. It is intended to be used at boot after a device manager or hotplug helper has been started or configured. During boot, devices may be plugged in before userspace is ready to process those events. By triggering "add" events for all currently present devices, this tool allows the device manager or hotplug helper to process the events it may have missed. Furthermore, the tool can be used for testing and debugging when the startup process is complete.

## Compilation, Installation, and Usage

Compilation requires `help2man` and is a simple:

    make

To install:

    sudo make install

By default, `devtrigger` will be installed to `/usr/local`. Set the variable `PREFIX` to change this:

    sudo env PREFIX=/usr/local make install

Then call the program directly after launching your preferred device manager:

    udevd &
    devtrigger

You can also target specific subsystems:

    devtrigger -s net

Or trigger remove events rather than add events:

    devtrigger -r
    devtrigger -r -s acpi

## Notes and Limitations

The subsystem option takes a glob pattern, which is `*` by default.

The tool looks for the patterns `/sys/class/SUBSYSTEM/*/uevent` and `/sys/bus/SUBSYSTEM/devices/*/uevent` when triggering events.

This tool can only be sure that events are triggered, not processed. Consult the device manager or hotplug helper documentation for determining the latter.

## See Also

Device managers:

* [udev](https://www.freedesktop.org/software/systemd/man/udev.html)
* [eudev](https://github.com/gentoo/eudev)
* [busybox-uevent](https://git.busybox.net/busybox/tree/util-linux/uevent.c)
* [nldev](https://core.suckless.org/nldev)
* [vdev (not maintained)](https://git.devuan.org/unsystemd/vdev)

Hotplug helpers:

* [mdev](https://git.busybox.net/busybox/tree/docs/mdev.txt)
* [smdev](https://core.suckless.org/smdev)
