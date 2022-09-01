# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_OTP_ASPEED otp_shell.c)
zephyr_library_sources_ifdef(CONFIG_OTP_ASPEED sha256.c)
