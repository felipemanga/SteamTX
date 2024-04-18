#!/bin/sh

set -e
CXX=clang++
LN=clang++
CPP_FILES=$(find src -name "*.cpp")
CPPM_FILES=$(find src -name "*.cppm")
CPP_FLAGS=$(cat compile_flags.txt)
LD_FLAGS=$(cat link_flags.txt)
BUILD=build
DEPENDENCIES=""

function build_module() {
    CPPM_FILE=$1
    echo "$CXX: $CPPM_FILE"
    OUTPUT_PATH="$BUILD/$(dirname $CPPM_FILE)"
    PCM_FILE=${CPPM_FILE%.*}.pcm
    M_FILE="$BUILD/$PCM_FILE"
    mkdir -p $OUTPUT_PATH
    #echo "PRE:" $CXX $CPPM_FLAGS "$CPPM_FILE" -o "$M_FILE"
    $CXX $CPP_FLAGS --precompile "$CPPM_FILE" -o "$M_FILE" &

    O_FILE="$BUILD/${CPPM_FILE%.*}.o"
    LD_FLAGS="$O_FILE $LD_FLAGS"
    #echo "CMP:" $CXX $CPP_FLAGS "$CPPM_FILE" -o "$O_FILE"
    $CXX $CPP_FLAGS -c "$CPPM_FILE" -o "$O_FILE" &
}

function build_std() {
    if [ ! -f build/$1.pcm ]; then
        echo "$CXX: $1"
        $CXX --std=c++20 -fprebuilt-module-path=./build/src -Os -x c++-system-header $1 --precompile -o ./build/$1.pcm &
    fi
    DEPENDENCIES="-fmodule-file=./build/$1.pcm $DEPENDENCIES"
}

mkdir -p "$BUILD/src"

build_std algorithm
build_std any
build_std atomic
build_std chrono
build_std cstddef
build_std cstdint
build_std cstdio
build_std exception
build_std filesystem
build_std fstream
build_std iostream
build_std memory
build_std mutex
build_std queue
build_std shared_mutex
build_std string
build_std string_view
build_std thread
build_std type_traits
build_std typeindex
build_std typeinfo
build_std unordered_map
build_std utility
build_std vector
build_std cstring
build_std functional
build_std random
wait

build_module src/Injectable.cppm
build_module src/EventBus.cppm
build_module src/Event.cppm
build_module src/Convert.cppm
wait
build_module src/utils.cppm
build_module src/Throttled.cppm
build_module src/System.cppm
build_module src/Registry.cppm
wait
build_module src/SDLSystem.cppm
build_module src/UI.cppm
build_module src/SerialPort.cppm
build_module src/Camera.cppm
wait
build_module src/LinuxSerialPort.cppm
build_module src/V4LCamera.cppm
build_module src/Radio.cppm
build_module src/UICamera.cppm
wait
build_module src/Main.cppm
wait

for CPP_FILE in $CPP_FILES; do
    echo "$CXX: $CPP_FILE"
    OUTPUT_PATH="$BUILD/$(dirname $CPP_FILE)"
    O_FILE="$BUILD/${CPP_FILE%.*}.o"
    LD_FLAGS="$O_FILE $LD_FLAGS"
    mkdir -p $OUTPUT_PATH
    #echo "CPP:" $CXX $CPP_FLAGS "$CPP_FILE" -o "$O_FILE"
    $CXX $CPP_FLAGS -c "$CPP_FILE" -o "$O_FILE" &
done
wait
echo $LN $LD_FLAGS
$LN $LD_FLAGS
