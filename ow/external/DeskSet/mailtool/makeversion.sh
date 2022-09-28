#! /bin/sh

# @(#)makeversion.sh 1.2 - 94/05/04

# mark an executable with its version; hard coded to live in external/DeskSet/*

date=`date`
host=`uname -n`
owversion="../../../config/OWversion"
program="$1"
progname=`echo $program | sed 's,^.*/,,'`

if [ -f "$owversion" ]
then
	version=`cat $owversion`
else
	version="Internal Build"
fi

vstring="$progname compiled $date on $host, $version"

echo "$vstring"

mcs -d "$program"
mcs -a "$vstring" "$program"

