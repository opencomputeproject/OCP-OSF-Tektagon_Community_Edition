# Introduction
Tektagon Open Edition (OE) is a HRoT orchestration firmware solution for the AST1060 SOC. It provides Intel PFR 2.0 HRoT solution.
# Source code Layout
The Tektagon OE solution based on common core has a baseline functionality that can be adapted and ported to various platforms. Where this can be reused for various platforms by modifying theabstraction layers.In Tektagon source tree OS zephyr will be the base layer and under base application and other modules are added as subdirectories. Where the Tektagon source tree structure under Zephyr base is as follows.

| Folders | Description |
| ------ | ------ |
| ApplicationLayer |	Tektagon OE specific implementation was added here. |
| Arch |	Zephyr OS layer – contains implementation & portings of various architectures |
| Boards |	Zephyr base – contains implementations and porting of different devices  |
| Cmake |	CMake toolchain and zephyr dependencies |
| Doc |	Documentation for zephyr structure and support |
| Drivers |	Zephyr based drivers  |
| Dts |	Bindings for various devices under boards are added here |
| FunctionalBlocks |	Cerberus based core functional implementation |
| HardwareAbstraction |	Contains Functional interface based on Cerberus core and other essential modules for application layer (i.e Tektagon OE) |
| Include |	Zephyr based header definitions are included here |
| Kernel |	Kernel implementations |
| Lib |	Zephyr’s library functions implementations. |
| Misc |	 |
| Modules |	Zephyr supported modules implementations  |
| samples | 	Sample for various devices based on zephyr |
| scripts | 	Scripts and other useful tools for zephyr |
| Share |	Packages shared by zephyr |
| Silicon |	Silicon (i.e., Platforms specific) implementation. (here AST1060) |
| soc | 	SoC file for boards based on zephyr |
| Subsys |	Sub systems supported based on zephyr |
| Tests |	Unit test suite for zephyr samples |
| Wrapper |	Wrapper function for Application layer is implemented |
# Building Firmware sources
## Clone the repo to your Linux machine
Follow the steps below to properly clone the repo from your terminal.

1.	git clone --recurse-submodules -b `Tag Name` `Clone URL` tektagon_oe
2.	cd tektagon_oe

Optional steps to perform
1.	git checkout `Branch Name`
2.	git submodule update --init --recursive
3.	git submodule update --remote

## Setup & Build Zephyr/Tektagon
Follow the numbered steps below to setup your Linux environment to build.

1.	wget http://apt.kitware.com/kitware-archive.sh
2.	sudo bash kitware-archive.sh
3.	sudo apt install --no-install-recommends git cmake ninja-build gperf \ ccache dfu-util device-tree-compiler wget \ python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \ make gcc gcc-multilib g++-multilib libsdl2-dev
4.	pip3 install --user -U west
5.	pip3 install virtualenv
6.	echo 'export PATH=~/.local/bin:"$PATH"' >> ~/.bashrc
7.	source ~/.bashrc
8.	pip3 install --user -r zephyr/scripts/requirements.txt
9.	pip3 install docutils==0.16
10.	pip3 install sphinx-tabs==3.2
11.	pip3 install pyyaml==6.0
12.	pip3 install six==1.15.0
13.	pip3 install testresources
14.	pip3 install click==8
15.	cd ~
16.	wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.13.2/zephyr-sdk-0.13.2-linux-x86_64-setup.run 
17.	chmod +x zephyr-sdk-0.13.2-linux-x86_64-setup.run
18.	./zephyr-sdk-0.13.2-linux-x86_64-setup.run -- -d ~/zephyr-sdk-0.13.2
19.	sudo cp ~/zephyr-sdk-0.13.2/sysroots/x86_64-pokysdk-linux/usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d
20.	sudo udevadm control --reload

## Building Zephyr/Tektagon
To build zephyr run “bash build.sh” under root folder. It will initialize west and update west modules for first build which might take time. From next build it starts with building sources. 

You can find zephyr.elf under build directory.!
# Programming AST1060
Refer to the [Aspeed Zephy SDK User Guide](https://github.com/AspeedTech-BMC/zephyr/releases/download/v00.01.05/Aspeed_Zephy_SDK_User_Guide_v00.01.05.pdf) section 3.4.1.2 SPI Flash Boot for steps to program the firmware onto the AST1060 EVB flash. On other platforms pl. refer to the platform documentation on flashing the firmware.
# Release Information
## version 1.00.0000
-	Intel PFR T-1 Operations
    -	Intel PFR BMC Active region verification
    -	Intel PFR PCH Active region verification
    -	Intel PFR BMC Recovery region verification
    -	Intel PFR PCH Recovery region verification
-	Intel PFR Provisioning
    -	Intel PFR will be enabled after Intel PFR provisioning
    -	Users can use BMC pfrtool to do the PFR provisioning
    -	Command: **pfrtool --bus <bus_id> --slave 0x38 --provision /usr/local/bin/config/rk_pub.pem**
-	Intel PFR SMBus Mailbox
    -	AST1060 is using SMBus Mailbox register file to communication with BMC and PCH
-	BMC Boot Release
-	1060 External Mux Control
-	Log Support
-	Key Management
    - Decommissioning
        -	Intel PFR decommissioning
        -	Users can use Decommissioning image to decommission the Intel PFR
    -	Key Cancellation
        -	Intel PFR key cancellation
        -	Users can generate key cancellation image to cancel specific CSK Key ID
        -	If the Key ID is canceled, capsule image with that key ID is not allowed to update
- SPI Filtering
- SMBus Filtering
-	Firmware Update
    -	YafuFlash tool for users to do the firmware update from BMC
-	Anti-Rollback
    -	BMC/PCH/1060 is not allowed to rollback to old SVN number
# Known Sightings
