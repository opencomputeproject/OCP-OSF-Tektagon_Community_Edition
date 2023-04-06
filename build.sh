#!/bin/bash

#---------------------------------------------------------------------------------------------------
# Check for dependencies
#---------------------------------------------------------------------------------------------------

west_dependencies() {

    wrk_dir=$1
    cur_dir="$(pwd)"
    cd $wrk_dir
    echo $wrk_dir $cur_dir
    if [ ! -d ".west" ]; then 
        west init -l west_init/
        west update
        west zephyr-export
        python3 -m pip install --user -r oe-src/scripts/requirements.txt 2>&1 > /dev/null
        pip3 install pyelftools
    else
        python3 -m pip install --user -r oe-src/scripts/requirements.txt 2>&1 > /dev/null
    fi
    cd $cur_dir
}

#---------------------------------------------------------------------------------------------------
# Build Steps for Zephyr
#---------------------------------------------------------------------------------------------------

west_base_build() {

    build_dir=$1
    platform=$2
    project_name=$3
    wrk_dir=$4
    source $wrk_dir/oe-src/zephyr-env.sh
    cd $wrk_dir/oe-src
    west $build_dir -p -b $platform $project_name
    cd ..
}

#---------------------------------------------------------------------------------------------------
# Main Script
#---------------------------------------------------------------------------------------------------

action=$1

case "$action" in
    tektagon)
        args=( $@ )

        build_dir=build
        platform=ast1060_evb
        project_name='ApplicationLayer/tektagon'
        wrk_dir=$(pwd)

        west_dependencies $wrk_dir
        
        cp zephyr_patches/Kconfig.zephyr oe-src/.
        cp zephyr_patches/CMakeLists.txt oe-src/.
        cp -r zephyr_patches/pfr_aspeed.h oe-src/include/drivers/misc/aspeed/
        cp -r zephyr_patches/pfr_ast1060.c oe-src/drivers/misc/aspeed/
        cp zephyr_patches/i2c_aspeed.c oe-src/drivers/i2c/
        cp cerberus_patches/Kconfig oe-src/FunctionalBlocks/Cerberus/
        cp cerberus_patches/CMakeLists.txt oe-src/FunctionalBlocks/Cerberus/
        cp -r cerberus_patches/zephyr/ oe-src/FunctionalBlocks/Cerberus/projects/
        cp cerberus_patches/i2c_slave_common.h oe-src/FunctionalBlocks/Cerberus/core/i2c/
        cp cerberus_patches/i2c_filter.h oe-src/FunctionalBlocks/Cerberus/core/i2c/
        cp cerberus_patches/debug_log.h oe-src/FunctionalBlocks/Cerberus/core/logging/
		cp cerberus_patches/rsah.patch oe-src/FunctionalBlocks/Cerberus/core/crypto
        cp cerberus_patches/logging_flash.h oe-src/FunctionalBlocks/Cerberus/core/logging/
        cp cerberus_patches/logging_flash.c oe-src/FunctionalBlocks/Cerberus/core/logging/
		git apply cerberus_patches/rsah.patch
        cd $wrk_dir/oe-src/
        git apply --reject --whitespace=fix ../zephyr_patches/*.patch
        echo -e "Dependencies updated"
        echo -e "Starting build"

        west_base_build $build_dir $platform $project_name $wrk_dir
    ;;
esac




