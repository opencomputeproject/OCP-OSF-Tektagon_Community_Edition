# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=Cortex-M4" "--gdbserver=JLinkGDBServerCLExe")
board_runner_args(dfu-util "--detach" "--pid=4153:1030" "--alt=1")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
