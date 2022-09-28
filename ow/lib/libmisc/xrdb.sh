#!/bin/sh
# @(#)xrdb.sh 1.2 91/04/16

rfile=
OPENWINHOME=${OPENWINHOME:-/usr/openwin}
LD_LIBRARY_PATH=$OPENWINHOME/lib
export OPENWINHOME LD_LIBRARY_PATH

while [ $# -gt 0 ]
do
	case $1 in
	-d*)	DISPLAY=$2; export DISPLAY; shift;;
	-l*)	rfile=$2; shift;;
	esac
	shift
done

# Set a background
$OPENWINHOME/bin/xsetroot -solid grey

#
$OPENWINHOME/bin/xrdb $rfile
exit
