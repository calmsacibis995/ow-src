/*	static char sccsid[] = "@(#)garbage.h 3.1 92/04/03 Copyr 1988 Sun Micro"
	garbage.h

	Housekeeping mechanism to clean up the calendar log
	file.  It copies the log file to a backup file, dumps
	the red/black tree to a temp file, copies the temp
	file back to the original log file, and deletes the
	temp and backup files.  Any errors encountered along
	the way abort the process.  The garbage collector
	runs at midnight every.
*/

extern void garbage_collect(/* int signal; */);

