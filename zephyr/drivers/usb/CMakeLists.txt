# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_USB device)
zephyr_library_sources_ifdef(CONFIG_SHELL_CMDS_USB usb_shell.c)
