# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_IPMI_KCS_ASPEED	kcs_aspeed.c)
zephyr_library_sources_ifdef(CONFIG_IPMI_BT_ASPEED	bt_aspeed.c)
