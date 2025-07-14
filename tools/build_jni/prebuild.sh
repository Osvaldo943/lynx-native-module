#!/bin/bash
CURRENT_PATH="$(dirname "$(realpath "$0")")"
JNI_PREBUILD=$CURRENT_PATH"/prebuild_jni.sh"
$JNI_PREBUILD
