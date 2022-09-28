#!/bin/ksh

# @(#)answerbook.sh	1.24 97/03/06 Copyright (c) 1992-1993 by Sun Microsystems, Inc.

#
# AnswerBook startup script.
#
# See that AnswerBook environment is properly initialized,
# then start up the AnswerBook Navigator.
#

# Fix for bug #1121183.  Some ksh users "set -o nounset" in their $ENV
# startup files.  This causes our script to fail whenever we reference
# an undefined variable (e.g., "$*").
#
set +o nounset

########################################
# Global variables
########################################
PROG=$(echo $0 | sed -e 's,.*/,,g')	# Name of this program 
CCTMP=/tmp/ab_cardcatalog.$$ 		# Temporary card catalog file
CREATED_CCTMP=false			# Did we create a card catalog file?
PERSLIB=$HOME/.ab_library		# User's personal library file
CREATED_PERSLIB=false			# Did we create personal library file?
AB_SETUP=answerbook_setup		# Name of AnswerBook setup script
AB_LIST=				# List of AnswerBooks in environment
REMOTE_HOST=				# Name of remote host if rlogin session
ECHO=/bin/echo
OPENWINHOME="${OPENWINHOME-/usr/openwin}"
ABADMIN=$OPENWINHOME/bin/ab_admin
AB_CCTMP=				# stores the path+filename of tmp card
					# cat file - if created

TEXTDOMAINDIR=$OPENWINHOME/lib/locale
TEXTDOMAIN=SUNW_DESKSET_ANSWERBOOK

########################################
# Local function definitions
########################################

#
# List the package names of the AnswerBooks installed on this local system.
# We obtain this info by querying the package installation database.
#
# Prints list of package names (PKGINST).
#
# Returns zero on success, else non-zero if problems encountered querying
# package database.
#
ListLocalAnswerBooks()
{
	#
	# Assumes all AnswerBook package titles contain the word "AnswerBook".
	# Also assumes output of "pkginfo -i" is as follows:
	#
	#	<CATEGORY><spaces><PKGINST><more_spaces><NAME>
	#
	# "pkginfo -i" list all fully (vs. partially) installed packages.
	#
	# XXX As a workaround for bug #1123169, remove the "-i" flag
	# XXX because many pre-Mars packages show up as "partially installed"
	# XXX even though they're perfectly viable packages.
	#
	LOCAL_AB_PKGS=$(pkginfo			| \
			egrep -i answerbook	| \
			sed -e 's,  *, ,g'	| \
			cut -d' ' -f2)

	[ $? -eq 0 ] || return 1

	$ECHO $LOCAL_AB_PKGS

	return 0
}

#
# Add local AnswerBooks to user's Card Catalog environment.
# This way, local AnswerBooks show up automatically in the user's
# AnswerBook Navigator.  No user intervention is required.
#
# Modifies value of $AB_CARDCATALOG environment variable.
#
# Returns zero on success, else non-zero if problems encountered.
#
# NOTE:	Ideally each locally-installed AnswerBook should be included in
#	a network-wide Card Catalog file so that it will be accessible
#	to everyone on the network, and so that we don't have to go
#	through the following rigamaroll every time.
#
SetUpLocalAnswerBooks()
{
	printf "\n`gettext 'Looking for locally installed AnswerBooks...'`\n"

	LOCAL_AB_PKGS=$(ListLocalAnswerBooks)	|| return 1

	if [ "$LOCAL_AB_PKGS" = "" ]; then
		return 0
	fi


	for PKGINST in $LOCAL_AB_PKGS; do

		#
		# An AnswerBook's card catalog or bookinfo file
		# (should) always reside in the directory specified
		# by that AnswerBook's "ABHOME" package parameter.
		# The usage of this parameter has been somewhat inconsistent
		# in the past, so we actually need to check in
		# $BASEDIR/$ABHOME as well as $ABHOME.
		#
		ABHOME=$( pkgparam $PKGINST ABHOME)
		if [ ! -d "$ABHOME" ]
		then
			BASEDIR=$(pkgparam $PKGINST BASEDIR)
			ABHOME=$BASEDIR/$ABHOME
		fi
		
		CCFILE=$ABHOME/ab_cardcatalog
		BIFILE=$ABHOME/bookinfo
		ABTITLE=$(PkgTitle $PKGINST)

		# Each AnswerBook's installation directory should contain
		# either a Card Catalog file or a bookinfo file that was
		# created when that AnswerBook was installed.
		#
		if [ -r $CCFILE ]; then

			# Found a Card Catalog file.
			# Add it to the list.
			#
			printf "\t\"%s\"\n" "$ABTITLE"
			AB_CARDCATALOG=${AB_CARDCATALOG}:$CCFILE

		elif [ -r $BIFILE ]; then

			# Found a bookinfo file.
			# Convert it to an entry in a temporary Card Catalog
			# file, and add that temp file to the list.
			#
			$ABADMIN -file $CCTMP -convert $BIFILE > /dev/null 2>&1
			if [ $? -ne 0 ]; then
				printf "\n\t`gettext 'Can\'t convert AnswerBook configuration file:' %s\n" "$BIFILE"
			else
				printf "\t\"%s\"\n" "$ABTITLE"
				CREATED_CCTMP="true"
				AB_CCTMP=$CCTMP
				export AB_CCTMP
			fi

		else
			printf "\t`gettext 'Can't find configuration info for \'%s\'`\n" "$ABTITLE"
		fi
	done


	#
	# If we converted any old-style AnswerBooks, include the Card
	# Catalog file with those entries in the Card Catalog search path.
	#
	if [ "$CREATED_CCTMP" = "true" ]; then
		AB_CARDCATALOG=${AB_CARDCATALOG}:$CCTMP
	fi

	return 0
}

#
# If user doesn't already have a personal AnswerBook Library file,
# create one for them.
#
CheckPersonalLibrary()
{
        if [ -r $PERSLIB ] ; then                              
		if [ ! -s $PERSLIB ] ; then
			printf "\n`gettext 'Error: The AnswerBook Library file \"%s\" is empty.`\n" "$PERSLIB"
			printf "`gettext 'Delete \"%s\" and rerun \"%s\".'`\n" "$PERSLIB" "$PROG"
			return 1
		fi
		return 0 
	fi 
 
	if [ -f $PERSLIB ] ; then
		printf "\n`gettext 'Error: can't access the AnswerBook Library file \"%s\"'`\n" "$PERSLIB"
		printf "`gettext 'because the file is not readable.'`\n\n"
		printf "`gettext 'Check/reset the permissions on \"%s\",'`\n" "$PERSLIB"
		printf "`gettext 'then rerun \"%s\".'`\n" "$PROG"

		return 1
	fi

	CreateLibrary $PERSLIB			|| return 1
	CREATED_PERSLIB=true

	#
	# Note that $AB_LIST is set elsewhere
	#
	PopulateLibrary $PERSLIB $AB_LIST	|| return 2

	return 0
}

#
# If user doesn't already have a personal AnswerBook Library file,
# create one for them.
#
CreateLibrary()
{
	printf "\n`gettext 'Creating new AnswerBook Library file \'%s\'...'`\n" "$PERSLIB"

	ABLIB=$1
	ABLIB_DIR=$(dirname $ABLIB)

	#
	# Make sure directory is writable.
	#
	if [ ! -w $ABLIB_DIR ] ; then


		printf "\n`gettext 'Error: can't create AnswerBook Library file \"%s\"'`\n" "$ABLIB"
		printf "`gettext 'because the directory is not writable.'`\n\n"
		printf "`gettext 'Check/reset the permissions on \"%s\"'`,\n" "$ABLIB_DIR"
		printf "`gettext 'then rerun \"%s\"'`.\n" "$PROG"

		return 1
	fi


	#
	# Write out Library header.
	#
	cat > $ABLIB <<-__EOF__
	#<AnswerBook Library> version 1
	#
	# This file was generated by "$PROG".
	# DO NOT EDIT THIS FILE BY HAND.
	#
	__EOF__

	return $?
}

# Add entries for specified AnswerBooks to the indicated AnswerBook Library.
#
# Usage: PopulateLibrary <library_file> <list of answerbook ids>
#
# Returns zero if library successfully populated, else returns non-zero.
#
PopulateLibrary()
{
	ABLIB=$1
	shift
	ABIDS=$*

	[ -r $ABLIB ]			|| return 1
	[ "$ABIDS" = "" ]		&& return 0


	for ID in $ABIDS; do

		#
		# Handle "id,version" syntax used by "ab_admin".
		#
		ID=$(     echo $ID | cut -d"," -f1)
		VERSION=$(echo $ID | cut -d"," -f2 -s)

		#
		# Add AnswerBook entry to library if not already in it
		#
		grep "<bs=$ID;vr=$VERSION>" $ABLIB > /dev/null
		if [ $? -eq 1 ]; then 
			$ECHO "bookshelf.name:	<bs=$ID;vr=$VERSION>" >> $ABLIB
		fi
	done

	return 0
}

#
# Validate basic OpenWindows environment.
#
CheckOpenWindows()
{
	#
	# $OPENWINHOME must be set.
	#
	if [ "${OPENWINHOME:-NotSet}" = "NotSet" ]; then
		printf "\n`gettext 'The \"OPENWINHOME\" environment variable is not set.`"
		printf "\n`gettext 'Are you running OpenWindows?'`"
		printf "\n\n`gettext 'Check/reset the value of \$OPENWINHOME, then rerun \"%s\"'`.\n", "$PROG"

		return 1
	fi

	#
	# Make sure we're running the right version of OpenWindows.
	#
	if [ ! -f ${OPENWINHOME}/bin/ab_admin ]; then
		printf "\n`gettext 'Could not find the AnswerBook Administration utility \"ab_admin\"'`\n"
		printf "`gettext 'in %s/bin.'`\n\n" "$OPENWINHOME"
		printf "`gettext 'Verify that you are running the Solaris 2.2 (or later)'`\n"
		printf "`gettext 'version of OpenWindows, then rerun \"%s\"'`.\n" "$PROG"

		return 2
	fi

	#
	# If this is an rlogin session, $DISPLAY should be set in *most*
	# cases.  If it isn't issue a notice, but keep on.
	#
	if IsRemoteLoginSession; then

		#
		# XXX Need to check whether they used "-display" or "-Wr"
		# XXX on the command line, as well as checking $DISPLAY.
		#

		if [ "${DISPLAY:-notset}" = "notset" ]; then
			printf "\n`gettext 'You are logged in from machine \"%s\"'`,\n" "$REMOTE_HOST"
			printf "`gettext 'but you have not set the \"\$DISPLAY\" environment variable.'`\n\n"
			printf "`gettext 'If you want the AnswerBook Navigator to display on %s'`,\n" "$REMOTE_HOST"
			printf "`gettext 'you should set \$DISPLAY, or use \"%s -display %s:0\"'`\n" "$PROG" "$REMOTE_HOST"
		fi
	fi


	return 0
}

#
# Look around the environment to make sure there are some AnswerBooks
# out there somewhere.
#
# Returns zero if there are, non-zero if not.
#
CheckForAnswerBooks()
{
	#
	# Use "ab_admin" to determine what Card Catalog files are in
	# the environment.
	#
	CARDCATS=$($ABADMIN -listpaths) > /dev/null 2>&1
	if [ $? -ne 0 -o "$CARDCATS" = "" ]; then

		printf "\n`gettext 'Could not find any AnswerBook Card Catalog files in your environment.'`\n"
		printf "`gettext 'The AnswerBook Navigator cannot access AnswerBooks'`\n"
		printf "`gettext 'that are not listed in a Card Catalog file.'`\n\n"
		printf "`gettext 'Contact your system administrator for assistance,'`\n"
		printf "`gettext 'or consult the Solaris AnswerBook Administration Guide for more info.'`\n"

		return 1
	fi

	#
	# Use "ab_admin" to list the AnswerBooks in each Card Catalog file.
	# As a side effect, initialize the $AB_LIST variable which gets
	# used elsewhere.
	#
	AB_LIST=$(for CC in $CARDCATS; do $ABADMIN -file $CC -list; done)
	if [ $? -ne 0 -o "$AB_LIST" = "" ]; then

		printf "\n`gettext 'Could not find any AnswerBooks in your environment.'`\n\n"

		printf "`gettext 'Contact your system administrator for assistance,'`\n"
		printf "`gettext 'or consult the Solaris AnswerBook Administration Guide for more info.'`\n"

		return 2
	fi


	return 0
}

#
# See if this is an rlogin session.
#
# Returns zero if this *is* an rlogin session, else returns non-zero.
#
IsRemoteLoginSession()
{
	#
	# XXX turn it off for now.
	#
	#return 1

	#
	# If this is an rlogin session, the "who" output for this tty
	# will show the local host in parentheses.  Otherwise, no parentheses.
	#
	TTY=$(/bin/tty | sed 's,/dev/,,g')

	/bin/who | egrep -s "$TTY.*(.*)"
	if [ $? -eq 0 ]; then
		REMOTE_HOST=$(	/bin/who			| \
				egrep "$TTY.*(.*)"		| \
				sed -e 's,.*(,,g' -e 's,).*,,g')
		return 0
	fi

        return 1
}

#
# Run the navigator	
#
StartNavigator()
{
	printf "\n`gettext 'Starting AnswerBook Navigator...'`\c\n"

	$OPENWINHOME/bin/toolwait $OPENWINHOME/bin/navigator $*

#
#	XXX "toolwait" exits with the wrong status, so we can't depend
#	XXX on that here.
#
#	if [ $? -ne 0 ]; then
#		$ECHO "Could not start AnswerBook Navigator."
#		$ECHO "Contact your system administrator for assistance."
#		return 1
#	fi

	printf "\n"

	return 0
}

#
# Get the title (NAME) of a package given the package abbreviation.
#
# Usage: PkgTitle <pkg_abbrev>
#
PkgTitle()
{
	#
	# Package name is simply the value of the "NAME" package parameter.
	#
	echo $(pkgparam $1 NAME)
}

#
# Clean up after ourselves.
#
CleanUpAndExit()
{
	[ "$CREATED_PERSLIB" = "true" ]  &&  rm -f $PERSLIB
	[ "$CREATED_CCTMP"   = "true" ]  &&  rm -f $CCTMP

	exit $1
}

########################################################
# End of function definitions
# Begin main shell processing
########################################################

#
# Handle signals gracefully
#
trap CleanUpAndExit HUP INT QUIT TERM


printf "`gettext 'Sun%cs documentation has changed format and is now viewed from a Web'`\n" "'"
printf "`gettext 'browser. The answerbook command (/usr/openwin/bin/answerbook) does'`\n"
printf "`gettext 'not support this functionality. To view Sun%cs new documentation,'`\n" "'"
printf "`gettext 'use the new command: /usr/dt/bin/answerbook2.'`\n\n"

printf "`gettext 'Verifying AnswerBook environment...'`\n"

#
# Make sure the OpenWindows environment is set up properly.
#
CheckOpenWindows			|| CleanUpAndExit 1

#
# $AB_CARDCATALOG contains a colon-separated list of
# AnswerBook Card Catalog file names.
# Make sure it is properly initialized.
#
export AB_CARDCATALOG=${AB_CARDCATALOG:-""}

#
# If there is an executable script named "answerbook_setup"
# somewhere in the user's shell search path, assume it contains
# AnswerBook setup commands and "source" it.
#
AB_SETUP=$(whence $AB_SETUP)		&& . $AB_SETUP

#
# Make sure locally-installed AnswerBooks show up in the
# AnswerBook Navigator.
#
SetUpLocalAnswerBooks			|| CleanUpAndExit 2

#
# Make sure there is at least *one* AnswerBook out there.
#
CheckForAnswerBooks			|| CleanUpAndExit 3

#
# Check user's Personal Library.  Create one if necessary.
#
CheckPersonalLibrary			|| CleanUpAndExit 4

#
# Run the navigator.  Pass on any command line arguments.
#
StartNavigator "$*"			|| CleanUpAndExit 6

exit 0
