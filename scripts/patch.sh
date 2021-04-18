#! /bin/bash

SCRIPT_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
ROOT_DIR=$SCRIPT_PATH/..
DIFF=$ROOT_DIR/cmake/jerry.diff

cd $ROOT_DIR/deps/jerryscript && git apply $DIFF 2>/dev/null
