#!/bin/bash

set -e

OP="$1 $2"
OP=${OP:=all}

(cd driver && make $OP)
(cd filesystem && make $OP)
(cd bootloader && make $OP)
(cd kernel && make $OP)
(cd usr/zlibc && make $OP)
(cd usr/init && make $OP)
(cd usr/hello && make $OP)
(cd usr/zesh && make $OP)
