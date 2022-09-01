# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=Cortex-M3" "--gdbserver=JLinkGDBServerCLExe")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
