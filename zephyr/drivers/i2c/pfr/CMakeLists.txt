# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_I2C_PFR_FILTER	i2c_filter.c)
zephyr_sources_ifdef(CONFIG_I2C_PFR_MAILBOX	i2c_mailbox.c)
zephyr_sources_ifdef(CONFIG_I2C_PFR_SNOOP	i2c_snoop.c)
