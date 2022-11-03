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
        west init
        west update
        west zephyr-export
        python3 -m pip install --user -r zephyr/scripts/requirements.txt 2>&1 > /dev/null
        pip3 install pyelftools
    else
        python3 -m pip install --user -r zephyr/scripts/requirements.txt 2>&1 > /dev/null
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
    source $wrk_dir/zephyr/zephyr-env.sh
    cd $wrk_dir/zephyr
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

        echo -e "Dependencies updated"
        echo -e "Starting build"

        west_base_build $build_dir $platform $project_name $wrk_dir
    ;;
esac




