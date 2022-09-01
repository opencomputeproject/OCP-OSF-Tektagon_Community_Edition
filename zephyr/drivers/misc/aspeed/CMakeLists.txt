# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SNOOP_ASPEED	snoop_aspeed.c)
zephyr_library_sources_ifdef(CONFIG_MAILBOX_ASPEED	mbox_aspeed.c)
zephyr_library_sources_ifdef(CONFIG_PFR_FLOW_CTRL_AST1060	pfr_ast1060.c)
zephyr_library_sources_ifdef(CONFIG_PFR_SHELL_ASPEED	pfr_shell.c)
