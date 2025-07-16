#!/bin/sh

cd -- "$(dirname -- "$0")"
export LD_LIBRARY_PATH=@TOPDIR@/ringing:@TOPDIR@/utils:$LD_LIBRARY_PATH
exec "./.objs/$(basename -- "$0")" "$@"
