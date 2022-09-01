.. _i3c-sample:

Improved-Inter-Integrated Circuit (I3C)
#################################

Overview
********

This sample demonstrates how to use the I3C driver API.

Building and Running
********************

The I3C peripheral and pinmux is configured in the board's ``.dts`` file. Make
sure that the I3C is enabled (``status = "okay";``).

Building and Running for ST Nucleo L073RZ
=========================================

The sample can be built and executed for the
:ref:`ast2600_board` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i3c
   :board: ast2600_evb
   :goals: build flash
   :compact:

To build for another board, change "ast2600_evb" above to that board's name
and provide a corresponding devicetree overlay.

Sample output
=============

You should get a similar output as below, repeated every second:

.. code-block:: console

   I3C reading(s): 42 (raw)

.. note:: If the I3C is not supported, the output will be an error message.
