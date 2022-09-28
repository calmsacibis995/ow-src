/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)file.cc	1.7	93/02/04 SMI"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>

#include <multimedia/Audio.h>
#include <multimedia/AudioFile.h>
#include <multimedia/AudioPipe.h>
#include <multimedia/AudioRawPipe.h>
#include <multimedia/AudioLib.h>
#include <multimedia/AudioHdr.h>

#include <multimedia/libaudio.h>
#include <multimedia/audio_filehdr.h>

extern char *Stdin;
extern char *Stdout;

#include "convert.h"

// append contents of buffer to output audio stream.

AudioError
write_output(AudioBuffer* buf, AudioStream* ofp)
{
	unsigned char 	*cp;
	size_t		len;
	Double		pos;
	AudioError	err;

	pos = ofp->GetLength();
	len = (size_t)buf->GetHeader().Time_to_Bytes(buf->GetLength());
	cp = (unsigned char *)buf->GetAddress();
	err = ofp->WriteData(cp, len, pos);
	return (err);
}

// open input file and return ptr to AudioUnixFile object
// path is the path to the file (or set to Stdin if standard input).
// ihdr is the input header (only used for openning raw files)
// israw flags if it's a raw file. if fflag is set, ignore an
// any existing header on raw files. offset indicates where to
// start reading the file (raw files only ??).
AudioUnixfile *open_input_file(const char *path, const AudioHdr ihdr, 
			       int israw, int fflag, off_t offset,
			       format_type& fmt)
{
	AudioUnixfile*	ifp;
	int		fd;
	Audio_filehdr	fhdr;
	Audio_hdr	ohdr;	// ignore this ...
	unsigned int	infosize; // ignore this ...
	unsigned int	hsize;
	
	// need to let caller know what format this is. so far, only raw
	// and sun are supported....
	fmt = (israw ? F_RAW : F_SUN); 

	// no file
	if (!path) {
		// no file? shouldn't happen. bomb out.
		Err(MGET("no input file specified\n"));
		exit(1);
	}

	// deal with stdin
	if (path == Stdin) {
		if (isatty(fileno(stdin))) {
			Err(MGET(
			    "Stdin is a tty, please specify an input file\n"));
			exit(1);
		}
		if (israw) {
			// XXX - need to check if stdin has a file
			// header and ignore it if fflag not set.
			ifp = new AudioRawPipe(fileno(stdin), ReadOnly,
					       ihdr, path, offset);
		} else {
			ifp = new AudioPipe(fileno(stdin), ReadOnly, path);
		}

		if (!ifp) {
			Err(MGET("can't open pipe to %s, skipping...\n"),
			    Stdin);
		}
		return(ifp);
	}

	// fall through for real files ...
	if (israw) {
		if ((fd = open(path, O_RDONLY)) < 0) {
			Err(MGET("can't open %s, skipping...\n"), path);
			perror(MGET("open"));
			return(NULL);
		}
		if (!fflag) {
			// check if file already has a hdr. 
			if (hsize = read(fd, (char*) &fhdr, sizeof (fhdr))
			    < 0) {
                          perror("read");
                          exit(1);
			}
			if (lseek(fd, 0, 0) < 0) {  // reset
                          perror("lseek");
                          exit(1);
			}
			if (hsize != sizeof (fhdr)) {
				// no hdr - file too small, 
				// assume data is ok (tho it
				// probably won't be) ...
				ifp = new AudioRawPipe(fd, ReadOnly, ihdr, 
						       path, offset);
			} else {
				// Check the validity of the 
				// header and get the size of 
				// the info field
				if (audio_decode_filehdr((unsigned char*)&fhdr,
							 &ohdr, &infosize)
				    == AUDIO_SUCCESS) {
					close(fd); // create AudioFile()
					// issue a warning
					Err(
				MGET("%s has a file header, ignoring -i ...\n"),
					    path);
					fmt = F_SUN; // was raw ...
					ifp = new AudioFile(path, ReadOnly);
				} else {
					// no hdr, create AudioRawPipe.
					ifp = new AudioRawPipe(fd, ReadOnly, 
							       ihdr, path,
							       offset);
				}
			}

		} else {	// force flag - don't even look for header
			ifp = new AudioRawPipe(fd, ReadOnly, ihdr, 
					       path, offset);
		}
	} else {
		ifp = new AudioFile(path, ReadOnly);
	}

	if (!ifp) {
		Err(MGET("can't open %s, skipping...\n"), path);
	}
	return(ifp);
}

// given a path, find the file it really points to (if it's a
// sym-link). return it's stat buf and real path.
void
get_realfile(char*& path, struct stat* st)
{
	static char	tmpf[MAXPATHLEN]; // for reading sym-link
	int		err;	// for stat err
	
	// first see if it's a sym-link and find real file
	err = 0;
	while (err == 0) {
		if (err = lstat(path, st) < 0) {
		  perror("lstat");
		  exit(1);
		}
		if (!err && S_ISLNK(st->st_mode)) {
			err = readlink(path, tmpf,
					(sizeof (tmpf) - 1));
			if (err > 0) {
				tmpf[err] = '\0';
				path = tmpf;
				err = 0;
			}
		} else {
			break;
		}
	}

}

// create output audio file. if no path is supplied, use stdout.
// returns a ptr to an AudioUnixFile object.

AudioUnixfile*
create_output_file(
	const char* path, 
	const AudioHdr ohdr, 
	format_type ofmt,
	const char *infoString)
{
	AudioUnixfile*	ofp = 0;
	AudioError	err;	// for error msgs
	int		fd;
	
	if (!path) {
		if (isatty(fileno(stdout))) {
			Err(
		    MGET("Stdout is a tty, please specify an output file\n"));
			exit(1);
		}

		path = Stdout;
		if (ofmt == F_RAW) {
			if (!( ofp = new AudioRawPipe(fileno(stdout),
						      WriteOnly, ohdr,
						      path))) {
				Err(
			    MGET("can't create audio raw stdout pipe\n"));
				exit(1);
			}
		} else if (ofmt == F_SUN) {
			if (!( ofp = new AudioPipe(fileno(stdout), WriteOnly, 
						   path))) {
				Err(
			    MGET("can't create audio pipe for stdout\n"));
				exit(1);
			}
		} else {
			// XXX - should never happen ...
			Err(MGET("can't create output file, unknown format\n"));
			exit(1);
		}
	} else {
		if (ofmt == F_RAW) {
			// first open file, then attach pipe to it
			if ((fd = open(path, O_WRONLY|O_CREAT|O_TRUNC,
				       0666)) < 0) {
				perror(MGET("open"));
				Err(MGET("can't create output file %s\n"),
				    path);
				exit(1);
			}
			if (!(ofp = new AudioRawPipe(fd, WriteOnly, ohdr, 
						     path))) {
				Err(MGET("can't create raw audio pipe %s\n"),
				      path);
				exit(1);
			}
		} else if (ofmt == F_SUN) {
			if (!( ofp = new AudioFile(path, ReadWrite))) {
				Err(MGET("can't create output file %s\n"),
				    path);
				exit(1);
			}
		} else {
			// XXX - should never happen ...
			Err(MGET("can't create output file, unknown format\n"));
			exit(1);
		}
	}

	// set the info string.
	ofp->SetInfostring( infoString, -1);
	
	// set the header and create the output audio object
	if ((err = ofp->SetHeader(ohdr)) != AUDIO_SUCCESS ) {
		Err(MGET("can't set hdr on output file: %s\n"), err.msg());
		exit(1);
	}
	if ((err = ofp->Create()) != AUDIO_SUCCESS ) {
		Err(MGET("can't create output file: %s\n"), err.msg());
		exit(1);
	}
	
	return (ofp);
}

