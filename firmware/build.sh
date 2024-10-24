#!/bin/bash

set -e

OP="$1 $2"
OP=${OP:=all}

(cd driver && make $OP)
(cd filesystem && make $OP)
(cd bootloader && make $OP)
(cd kernel && make $OP && cp kernel.bin KERNEL.IMG)
