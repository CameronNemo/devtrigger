# devtrigger

This tool is intended to be used in conjunction with a hotplug daemon/helper. During boot, devices may be plugged in before the hotplug daemon/helper is launched.

By triggering "add" events for all currently present devices, this tool allows the hotplug daemon/helper to process the events it may have missed.

## Compilation, Installation, and Usage

Compilation is a simple:

    make

To install:

    sudo make install

By default, `devtrigger` will be installed to `/sbin`. Set the environment variable `INSTALL_PREFIX` to change this:

    sudo env INSTALL_PREFIX=/usr/local make install

Then call the program directly after launching your preffered hotplug daemon.

    device_manager_daemon &
    devtrigger

You can also target specific subsystems and trigger other events for testing purposes:

    devtrigger -s net
    devtrigger -s drm -a remove
