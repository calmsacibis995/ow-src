#! /bin/sh
# @(#)mkolcursorh.sh 1.1 90/04/23
# make the olcursor.h file from the .bdf file.

awk '

BEGIN {
    print "/* Copyright (c) 1990, Sun Microsystems, Inc. */"
}

/^STARTCHAR/ {
    charname = $2;
    ignore = 0;
}

/^STARTCHAR.*_m$/ {
    ignore = 1;
}

/^ENCODING/ {
    if (ignore == 0)
	printf ("#define OLC_%s %s\n", charname, $2);
}
' olcursor.bdf > olcursor.h
