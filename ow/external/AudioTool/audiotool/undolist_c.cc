/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)undolist_c.cc	1.60	93/02/25 SMI"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef SUNOS41
#include <ustat.h>
#else
#include <sys/statvfs.h>
#endif

#include <multimedia/AudioExtent.h>
#include <multimedia/AudioList.h>
#include <multimedia/AudioFile.h>
#include <multimedia/AudioDevice.h>
#include <multimedia/AudioDevicectl.h>
#include <multimedia/AudioPipe.h>
#include <multimedia/AudioBuffer.h>
#include <multimedia/AudioDetect.h>
#include <multimedia/AudioTypeG72X.h>
#include <multimedia/AudioLib.h>
#include <multimedia/AudioGain.h>
#include <multimedia/AudioDebug.h>
#include <multimedia/audio_hdr.h>
#include <multimedia/audio_encode.h>
#include <multimedia/libaudio_c.h>

#include "atool_i18n.h"
#include "atool_debug.h"
#include "undolist_c.h"
#include "Undolist.h"


// XXX - tmp - for converting G72x to u-law
#define CVTBUFSIZ	10240

// Make these routines callable by the C-side AudioTool source
extern "C" {

// XXX - patchable level meter window:
//       meter_window = (1. / meter_time) seconds
int	meter_time = 10;
double	meter_window = .1;


// Reference a given audio object.
void
audio_reference(
	Audio*		ap)	// audio object to dereference
{
	ap->Reference();
}

// Dereference a given audio object.
// The value of the given pointer may become invalid when this routine returns.
void
audio_dereference(
	  Audio*	ap)		// audio object to dereference
{
	ap->Dereference();
}

unsigned char *
audio_getbufaddr(
	AudioBuffer*		abuf) // better be a buffer
{
	if (!abuf)
		return (NULL);

	return (unsigned char *)abuf->GetAddress();
}

// Close the given audio object (this is necessary for pipes)
int
audio_close(
	AudioUnixfile	*ap)
{
	if (ap == NULL) {
		return (AUDIO_ERR_NOEFFECT);
	}
	return (ap->Close());
}

// Return an error message string
char*
audio_errmsg(
	int		e)		// error code
{
	return (((AudioError)e).msg());
}

// Convert a length, in time, to kbytes.  Returns -1 if invalid header.
long
audio_getsize(
	double		time,
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);
	unsigned long	len;

	if (hdr.Validate() != AUDIO_SUCCESS)
		return (-1);
	len = (unsigned long) hdr.Time_to_Bytes(time);
	return ((long)((len + 1023)/ 1024));
}

// Convert a length, in bytes, to a time in the specified format.
double
audio_bytes_to_time(
	long		bytes,
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);

	if (hdr.Validate() != AUDIO_SUCCESS)
		return (0.);
	return (hdr.Bytes_to_Time(bytes));
}

// Return the length, in time, of the given audio object.
double
audio_getlength(
	Audio*		ap)		// audio object
{
	if (ap == NULL)
		return (0.);
	return (ap->GetLength());
}

// fill in an audio file hdr struct for the given audio object.
int
audio_getfilehdr(
	Audio*		ap,
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr;		// local copy of header
	AudioError	err;

	if (ap == NULL || ohdr == NULL)
		return (AUDIO_ERR_BADARG);
	hdr = ap->GetHeader();

#ifdef notdef
	if (Undefined(ap->GetLength()))
		return (AUDIO_ERR_BADARG);
#endif

	*ohdr = (Audio_hdr)hdr;
	if (Undefined(ap->GetLength()))
		ohdr->data_size = 0;
	else
		ohdr->data_size = (u_int)
		    hdr.Time_to_Bytes(ap->GetLength());
	return (AUDIO_SUCCESS);
}

// create a temp AudioBuffer so we can get the actual bits of a particular
// chunk of audio and stuff it in a char *buffer.
// this is actually pretty lame. since we need to keep track of everything
// in bytes, we need to use an AudioBuffer object. so we convert the bytes
// to time values, create a (sub) AudioExtent from the AudioExtent that's
// passed in, create an AudioBuffer and copy from the AudioExtent to
// the AudioBuffer, then do a ReadData to get it into our char *buffer,
// then delete the (sub) AudioExtent and AudioBuffer. not pretty ...
// the other option is to just hold on to the AudioBuffer 'till we're
// done. the only problem there is that we may have a huge chunk to
// xfer. so while this may be a bit CPU intensive, it's relatively
// memory efficient....
int
audio_getbuffer(
	Audio*		ap,
	unsigned char*	buf,
	unsigned int	pos,
	unsigned int*	blen)
{
	AudioError	err;
	AudioBuffer*	tmpbuf;
	AudioExtent*	aext;
	Double		len;	// length in sec's
	Double		start;	// where to start reading, in sec's
	unsigned int	newblen;

	if (ap == NULL || buf == NULL)
		return (AUDIO_ERR_BADARG);

	// XXX - should make sure len & start are valid ...
	len = ap->GetHeader().Bytes_to_Time(*blen);
	start = ap->GetHeader().Bytes_to_Time(pos);

	aext = new AudioExtent(ap, start, start+len);
	if (!aext) {
		return (AUDIO_UNIXERROR);
	}

	tmpbuf = new AudioBuffer(len, "tmp buffer");
	if (!tmpbuf) {
		return (AUDIO_UNIXERROR);
	}

	tmpbuf->SetHeader(aext->GetHeader());

	err = aext->Copy(tmpbuf);	// copy to the audio buffer

	// read the AudioBuffer into the char *buffer.
	newblen = *blen;
	start = 0.0;			// always copying from start to end  
	if (!err)
		err = tmpbuf->ReadData(buf, newblen, start);

	*blen = newblen;
	delete tmpbuf;
	delete aext;

	return (err);
}

// create an AudioBuffer big enough to hold blen bytes of audio
// data using the header info from fhdr.
AudioBuffer*
audio_createbuffer(
	Audio_hdr*	fhdr,
	unsigned int	blen)
{
	AudioError	err;
	AudioHdr	hdr(*fhdr);
	AudioBuffer*	abuf;
	double		len;	// length in sec's

	if (hdr.Validate() || (blen == NULL))
		return (NULL);

	// XXX - should make sure len is valid ...
	len = hdr.Bytes_to_Time(blen);

	abuf = new AudioBuffer(len, "tmp buffer");
	if (!abuf)
		return (NULL);

	abuf->SetHeader(hdr);
	abuf->Reference();
	return (abuf);
}

// write the data in buf to the AudioBuffer abuf.
// adjust the length, blen, apropriately.
int
audio_putbuffer(
	AudioBuffer*	abuf,		// AudioBuffer to write to
	unsigned char*	buf,     	// buf to copy from
	unsigned long*	blen,		// length in bytes
	unsigned long*	pos)		// position in bytes
{
	size_t		wlen;			// write length
	AudioError	err;
	Double		start;

	if (abuf == NULL || buf == NULL) {
		return (AUDIO_ERR_BADARG);
	}
	wlen = (size_t) *blen;
	start = abuf->GetHeader().Bytes_to_Time(*pos);
	err = abuf->WriteData(buf, wlen, start);

	*blen = wlen;		// number actually written
	*pos += wlen;		// new start pos in bytes
	return (err);
}

// get name (path) of given audio object
char*
audio_getname(
	Audio*	ap)
{
	if (ap) {
		return (ap->GetName());
	} else {
		return (NULL);
	}
}

// Open audio control device.
// Returns audio error code or AUDIO_SUCCESS.
int
audio_openctldev(
        char*		name,	// input filename
        Audio*&		ap)	// returned audio object
{
	AudioDevicectl*	ctldev;
	AudioError	err;

	ap = NULL;
	ctldev = new AudioDevicectl(name);
	if (ctldev == 0)
		return (AUDIO_UNIXERROR);
	err = ctldev->Open();
	if (err) {
		delete ctldev;
		return (err);
	}

	// Enable SIGPOLL on status change
	ctldev->SetSignal(TRUE);
	ap = ctldev;
	return (AUDIO_SUCCESS);
}

// Return TRUE if the specified format is playable
// The sample rate may be adjusted in the returned header.
int
audio_checkformat(
	AudioDevice*	dp,
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);
	return (dp->CanSetHeader(hdr));
}

// Attempt to set the specified format in the audio device.
// The input should be control device, otherwise this will fail.
// Returns TRUE if successful.
int
audio_setformat(
	AudioDevicectl*	dp,
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);
	AudioDevice*	realdev;
	int		setfmt;

	if (!dp->CanSetHeader(hdr))
		return (FALSE);

	// If the format already matches, we're done
	if (dp->GetHeader() == hdr)
		return (TRUE);

	// Open the real device non-blocking and try to set the format
	realdev = new AudioDevice(dp->GetName(), WriteOnly);
	if (realdev == NULL)
		return (FALSE);
	realdev->SetBlocking(FALSE);
	if (!realdev->Open() && !realdev->SetHeader(hdr))
		setfmt = TRUE;
	else 
		setfmt = FALSE;
	delete realdev;
	return (setfmt);
}

// Set the play waiting flag on the specified audio device
void
audio_setplaywaiting(
	AudioDevice*	dp)
{
	(void) dp->SetPlayWaiting();
}

// Set the record waiting flag on the specified audio device
void
audio_setrecordwaiting(
	AudioDevice*	dp)
{
	(void) dp->SetRecWaiting();
}

// Get the play open flag for the specified audio device
int
audio_getplayopen(
	AudioDevice*	dp)
{
	return (dp->GetPlayOpen());
}


// Open input file readonly.
// Returns audio error code or AUDIO_SUCCESS.
int
audio_openfile(
        char*		name,	// input filename
        Audio*&		ap)	// returned audio object
{
	AudioFile*	inf;
	AudioList*	lp;
	AudioError	err;

	// Open file and decode the header
	inf = new AudioFile(name, ReadOnly);
	if (inf == 0)
		return (AUDIO_UNIXERROR);
	err = inf->Open();
	if (err) {
		delete inf;
		return (err);
	}

	// set to normal access
	inf->SetAccessType(NormalAccess);

	// Create a list object and set it up to reference the file
	lp = new AudioList;
	if (lp == 0) {
		delete inf;
		return (AUDIO_UNIXERROR);
	}
	lp->Insert(inf);
	ap = lp;
	ap->Reference();
	return (AUDIO_SUCCESS);
}

// write the given audio object over a pipe
int
audio_write_pipe(
	Audio*		ap,	// audio object
	int		fd)
{
	AudioPipe*	apipe;
	AudioError	err;

	if (ap == NULL)
		return (AUDIO_ERR_NOEFFECT);
	apipe = new AudioPipe(fd, WriteOnly, "audio write pipe");

	// set the length to the size of the extent we're writing
	apipe->SetLength(ap->GetLength());
	err = apipe->SetHeader(ap->GetHeader());	// set the header
	if (!err)
		err = apipe->Create();			// writes the header 
	if (!err)
		err = ap->Copy(apipe);		// shove it down the pipe
	delete apipe;
	return (err);
}

// read from a pipe and return an audio buffer
int
audio_read_pipe(
	int		fd,
	Audio*&		dp)		// audio object returned
{
	AudioPipe*	apipe;
	AudioError	err;
	AudioBuffer*	abuf;
	Double		len;

	apipe = new AudioPipe(fd, ReadOnly, "audio read pipe");
	err = apipe->Open();			// open & read header
	if (!err)
		len = apipe->GetLength();
	if (err || Undefined(len)) {
		delete apipe;
		return (AUDIO_ERR_BADFILEHDR);
	}

	abuf = new AudioBuffer(len, "temp buffer");
	err = abuf->SetHeader(apipe->GetHeader());
	if (!err) {
		err = apipe->Copy(abuf);	// scoop in data from pipe
	}
	delete apipe;
	if (err) {
		delete abuf;
		return (err);
	}
	abuf->Reference();
	dp = abuf;
	return (AUDIO_SUCCESS);
}

// Convert an Audio_hdr into a printable string
// The string should be freed when no longer needed
char*
audio_printhdr(
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);
	char*		str;
	char*		nstr;

	if (hdr.Validate())
		return (NULL);
	str = hdr.FormatString();

	// Copy the string, to isolate the C++ allocation
	nstr = strdup(str);
	delete str;
	return (nstr);
}

// Convert an audio encoding into a printable string
// The string should be freed when no longer needed
char*
audio_print_encoding(
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);
	char*		str;
	char*		nstr;

	str = hdr.EncodingString();

	// Copy the string, to isolate the C++ allocation
	nstr = strdup(str);
	delete str;
	return (nstr);
}

// Return a string for a sample rate
// The string should be freed when no longer needed
char *
audio_print_rate(
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);
	char*		str;
	char*		nstr;

	str = hdr.RateString();

	// Copy the string, to isolate the C++ allocation
	nstr = strdup(str);
	delete str;
	return (nstr);
}

// Return a string for channels
// The string should be freed when no longer needed
char *
audio_print_channels(
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);
	char*		str;
	char*		nstr;

	str = hdr.ChannelString();

	// Copy the string, to isolate the C++ allocation
	nstr = strdup(str);
	delete str;
	return (nstr);
}

// Parse a printable string into an Audio_hdr
// Returns error, if any
int
audio_parsehdr(
	char*		str,
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr;
	AudioError	err;

	err = hdr.FormatParse(str);
	*ohdr = (Audio_hdr) hdr;
	return (err);
}

// Parse a printable string into an Audio_hdr
// Returns error, if any
int
audio_parse_rate(
	char*		str,
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);
	AudioError	err;

	err = hdr.RateParse(str);
	*ohdr = (Audio_hdr) hdr;
	return (err);
}

// Parse a printable string into an Audio_hdr
// Returns error, if any
int
audio_parse_channels(
	char*		str,
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);
	AudioError	err;

	err = hdr.ChannelParse(str);
	*ohdr = (Audio_hdr) hdr;
	return (err);
}

// Parse a printable string into an Audio_hdr
// Returns error, if any
int
audio_parse_encoding(
	char*		str,
	Audio_hdr*	ohdr)
{
	AudioHdr	hdr(*ohdr);
	AudioError	err;

	err = hdr.EncodingParse(str);
	*ohdr = (Audio_hdr) hdr;
	return (err);
}


// Return the size, in kbytes, of the free disk space of the named directory.
// Returns -1 if the directory (or named file) is not valid or read-only.
long
getfreespace(
       char*		target)		// target directory or file
{
#ifdef SUNOS41
#define AVAIL	ust.f_tfree
#define BLKSIZ	512

	struct stat	st;
	struct ustat	ust;

	if ((stat(target, &st) < 0) || (ustat(st.st_dev, &ust) < 0))

#else /* SVR4 */
#define AVAIL	fst.f_bavail
#define BLKSIZ	fst.f_frsize

	struct statvfs	fst;

	if ((statvfs(target, &fst) < 0) || (fst.f_flag & ST_RDONLY))
#endif /* SVR4 */
		// could not stat the filesystem or it is read-only
		return (-1);

	if (access(target, W_OK) < 0)
		return (-1);
	return((long)((double)AVAIL * (double)BLKSIZ / 1024.));
}

// Convert an integer number of kilobytes into a printable string
// The string should be freed when no longer needed
char*
audio_printkbytes(
	long	kbytes)
{
	double	b;
	int	mb;
	int	kb;
	char	rbuf[BUFSIZ];

	if (kbytes == 0) {
		// Special case for zero
		(void) sprintf(rbuf, MGET("0"));

	} else if (kbytes < 1010) {
		// If under 1 Mb, print two decimal places
		b = ((double)kbytes / 10.24);
		kb = (int) ceil(b);
		(void) sprintf(rbuf, MGET(".%02d"), kb);

	} else {
		// If > 1Mb, fudge it and round up to .1Mb
		b = ((double)kbytes / 102.4) + .70;
		mb = (int) floor(b);
		kb = mb % 10;
		mb /= 10;
		(void) sprintf(rbuf, MGET("%d.%1d"), mb, kb);
	}
	return (strdup(rbuf));
}

// Set debugging level
// 1 prints most error messages
// 2 prints debugging messages
void
audio_debug(
	int		level)		// debug level
{
	SetDebug(level);
#if !defined(SVR4) && defined(DEBUG)
#ifdef notdef
	if (level > 0)
		malloc_debug(2);
#endif
#endif
}

// Print an audio error message
void
audio_perror(
	int		e,		// error code
	char*		umsg)		// user message
{
	AudioError	err;

	err = (AudioError)e;

	if (err == AUDIO_SUCCESS) {
		if (umsg != NULL)
			fprintf(stderr, "%s\n", umsg);
	} else {
		if (umsg == NULL)
			umsg = "";
		fprintf(stderr, "%s: %s\n", umsg, err.msg());
	}
}

// Print out the detection list
void
print_detect(AudioDetectPts*	pts)
{
	char*	str;
	double	from;
	double	to;

	do {
		from = pts->pos;
		switch (pts->type) {
		case DETECT_SOUND:
			str = "Sound"; to = pts[1].pos; break;
		case DETECT_SILENCE:
			str = "Silence"; to = pts[1].pos; break;
		case DETECT_EOF:
			str = "End of file"; to = -1.; break;
		default:
			str = "Unknown type?!"; to = -1.; break;
		}
		fprintf(stderr, "%s: %f", str, from);
		if (to != -1.)
			fprintf(stderr, " - %f\n", to);
		else
			fprintf(stderr, "\n");
	} while (pts++->type != DETECT_EOF);
}

// Initialize an audio undo list.
// Returns audio error code or AUDIO_SUCCESS.
int
list_init(
	Undolist*&	ulp)		// returned list object
{
	Audio_Init();			// Init C++ runtime
#ifdef DEBUG
	audio_debug(1);
#endif
	ulp = new Undolist;
	if (ulp == NULL)
		return (AUDIO_UNIXERROR);
	return (AUDIO_SUCCESS);
}

// Free an audio undo list.
void
list_destroy(
	Undolist*	ulp)		// list object
{
	delete ulp;
}


// given an audio object that contains G72x compressed data,
// replace with decoded data
// XXX - this routine should not be used; exec audioconvert instead
int
list_g72x_decode(
	Audio*		ap,
	Audio*&		outap)
{
	AudioBuffer*	ibuf;
	AudioError	err;
	Double		to;
	Double		from;
	Double		limit;
	AudioHdr	hdr;	// for new buffer/tmpfile
	AudioTypeG72X	conv;

	// header to use to create tmp file & output buffer
	// XXX - assumes one file type in list ...
	hdr = ap->GetHeader();
	if (!conv.CanConvert(hdr))
		return (AUDIO_ERR_BADHDR);

	// input buffer: entire file
	// XXX - this has to change!
	if (!(ibuf = new AudioBuffer()))
		return (AUDIO_UNIXERROR);
	if (err = ibuf->SetHeader(hdr))
		return (err);
	ibuf->SetSize(ap->GetLength());

	// output buf: big enough for the whole file ...
	// XXX - need to specify ulaw/alaw/pcm for conversion
	hdr.encoding = ULAW;
	hdr.bytes_per_unit = 1;
	hdr.samples_per_unit = 1;
	if (!conv.CanConvert(hdr))
		return (AUDIO_ERR_BADHDR);

	// read the file into the buffer and decode
	from = 0.0;
	to = 0.0;
	limit = AUDIO_UNKNOWN_TIME;
	err = ap->Copy(ibuf, from, to, limit);
	if (!err) {
		err = conv.Convert(ibuf, hdr);
	}
	if (err) {
		delete ibuf;
		return (err);
	}
	outap = ibuf;
	return (err);
}

// Clear the current list, open input file readonly,
// set it up as the first list entry.
// Returns audio error code or AUDIO_SUCCESS.
int
list_newfile(
	Undolist*	ulp,		// list object
	char*		name)		// input filename
{
	Audio*		ap = NULL;
	AudioError	err;

	err = audio_openfile(name, ap);
	if (err)
		return (err);

#ifdef notdef
	// XXX - not needed, since load file now uses audioconvert
	// if compressed, decode in to new buffer ...
	if ((ap->GetHeader().encoding == G721) ||
	    (ap->GetHeader().encoding == G723)) {
		Audio*		cvtbuf;
		AudioList*	lp;

		err = list_g72x_decode(ap, cvtbuf);
		if (!err) {
			// get rid of old object, create new one ... 
			ap->Dereference();	// referenced in audio_openfile
			if (!(lp = new AudioList)) {
				delete cvtbuf;
				return (AUDIO_UNIXERROR);
			}
			lp->Insert(cvtbuf);
			ap = lp;
		}
	}
	if (err) {
		ap->Dereference();	// referenced in audio_openfile
		return (err);
	}
#endif

	ulp->clearlist();		// clear current list
	ulp->addentry(ap);		// set file as current entry
	ap->Dereference();		// referenced in audio_openfile
	return (AUDIO_SUCCESS);
}


// Return the current Audio* in the list, or NULL if list is empty.
Audio*
list_getaudio(
	Undolist*	ulp)		// list object
{
	Audio*		ap;

	ap = ulp->currentaudio();
	if (ap != NULL)
		ap->Reference();
	return (ap);
}

// Return the size, in kbytes, of the current list entry.
long
list_getfilesize(
	Undolist*	ulp)		// list object
{
	Audio*		ap;
	unsigned long	len;

	ap = ulp->currentaudio();
	if (ap == NULL)
		return (0);
	len = (unsigned long) ap->GetHeader().Time_to_Bytes(ap->GetLength());
	return ((long)((len + 1023)/ 1024));
}

// Return the length, in time, of the current list entry.
double
list_getlength(
	Undolist*	ulp)		// list object
{
	Audio*		ap;

	ap = ulp->currentaudio();
	if (ap == NULL)
		return (0.);
	return (ap->GetLength());
}

// Return the read position, in time, of the current list entry.
double
list_readpos(
	Undolist*	ulp)		// list object
{
	Audio*		ap;

	ap = ulp->currentaudio();
	if (ap == NULL)
		return (0.);
	return (ap->ReadPosition());
}

// reset read position
double
list_rewind(
	Undolist*	ulp)		// list object
{
	Audio*		ap;

	ap = ulp->currentaudio();
	if (ap == NULL)
		return (0.);
	return (ap->SetReadPosition(0.0));
}

// fill in an audio file hdr struct for the given audio object.
int
list_getfilehdr(
	Undolist*	ulp,
	double		frompos,
	double		/*topos*/,
	Audio_hdr*	ohdr)
{
	Audio*		ap;
	AudioHdr	hdr;	// local copy of header
	AudioError	err;
	AudioExtent	*asel;

	// XXX - Set up the libaudio hdr
	ap = ulp->currentaudio();
	if (ap == NULL || ohdr == NULL)
		return (AUDIO_ERR_BADARG);

	// XXX - for now ignore topos and get all
	if (Undefined(frompos)) /*  || Undefined(topos)) */
		return (AUDIO_ERR_BADARG);

	asel = new AudioExtent(ap, frompos /* ,topos */);
	if (!asel)
		return (AUDIO_UNIXERROR);

	// XXX - header should be set to type we're sending...
	err = audio_getfilehdr(asel, ohdr);
	delete asel;
	return (err);
}

// Return an unreferenced audio buffer which is a copy of the extent
// from "start" to  "start + len"
AudioBuffer*
list_getrange(
	Undolist*	ulp,
	double		start,
	double		len)
{
	Double		end;
	AudioBuffer*	abuf;
	Audio*		ap;
	AudioError	err;
	AudioExtent*	aext;

	if (start < 0.)
		return (NULL);

	// Get endpoint
	end = start + len;
	ap = ulp->currentaudio();
	if ((ap == NULL) || (len == 0.) || (ap->GetLength() < end))
		return (NULL);

	aext = new AudioExtent(ap, (Double)start, end);
	if (aext == NULL)
		return (NULL);

	abuf = new AudioBuffer(len, "range buffer");

	// if we get this far and there's no abuf, we're hosed
	if (abuf == NULL) {
		delete aext;
		return (NULL);
	}
	err = abuf->SetHeader(ap->GetHeader());
	if (!err)
		err = aext->Copy(abuf);

	if (err) {
		delete abuf;
		abuf = NULL;
	}
	delete aext;
	return (abuf);
}

// Returns TRUE if there is an edit left to undo; else returns FALSE.
int
list_canundo(
	Undolist*	ulp)		// list object
{
	return ((int)ulp->canundo());
}

// Returns TRUE if there is an edit left to redo; else returns FALSE.
int
list_canredo(
	Undolist*	ulp)		// list object
{
	return ((int)ulp->canredo());
}

// Undo the previous edit operation.
// Returns TRUE if no-op or at end of Undo list; else returns FALSE.
int
list_undo(
	Undolist*	ulp)		// list object
{
	return ((int)ulp->undo());
}

// Undo all edit operations.
// Returns TRUE if no-op; else returns FALSE.
int
list_undoall(
	Undolist*	ulp)		// list object
{
	return ((int)ulp->undoall());
}

// Redo the previous edit operation.
// Returns TRUE if no-op; else returns FALSE.
int
list_redo(
	Undolist*	ulp)		// list object
{
	return ((int)ulp->redo());
}

// Redo all edit operations.
// Returns TRUE if no-op; else returns FALSE.
int
list_redoall(
	Undolist*	ulp)		// list object
{
	return ((int)ulp->redoall());
}

// Insert audio data into a list.
// Returns audio error code or AUDIO_SUCCESS.
int
list_insert(
	Undolist*	ulp,		// list object
	Audio*		ap,		// audio list to insert
	double		pos)		// time, in seconds, at which to insert
{
	Audio*		old;
	Audio*		cvtbuf;
	AudioList*	lp;
	AudioError	err;

	// Check if compressed & decompress
	if ((ap->GetHeader().encoding == G721) ||
	    (ap->GetHeader().encoding == G723)) {
		err = list_g72x_decode(ap, cvtbuf);
		if (err)
			return (err);
		// caller will deref the original audio object
		ap = cvtbuf;
	}

	old = ulp->currentaudio();
	if (old == NULL) {
		ulp->addentry(ap);	// this is the first entry
	} else {
		// Duplicate list by making a new list pointing to the old one
		lp = new AudioList;
		if (lp == NULL)
			return (AUDIO_UNIXERROR);
		if ((err = lp->Insert(old)) || (err = lp->Insert(ap, pos))) {
			delete lp;
			return (err);
		}
		// XXX - update cached detection array
		ulp->addentry(lp);
	}
	return (AUDIO_SUCCESS);
}

// Remove a section of audio data from a list.
// Returns audio error code or AUDIO_SUCCESS.
int
list_remove(
	Undolist*	ulp,		// list object
	double		frompos,	// start time of section to cut
	double		topos)		// end time of section to cut
{
	Audio*		old;
	AudioList*	lp;
	Double		oldlen;
	AudioExtent*	start;
	AudioExtent*	end;
	AudioError	err;

	old = ulp->currentaudio();
	if (old == NULL)
		return (AUDIO_ERR_NOEFFECT);

	oldlen = old->GetLength();
	if (Undefined(oldlen))
		return (AUDIO_ERR_BADARG); // indeterminate list length?!

	// Remove data by referencing only what's left
	lp = new AudioList;
	if (lp == NULL)
		return (AUDIO_UNIXERROR);

	// If not cutting start of file, insert beginning extent
	if (frompos > 0.) {
		start = new AudioExtent(old, 0., frompos);
		if (start == NULL)
			return (AUDIO_UNIXERROR);
		err = lp->Insert(start);
		if (err) {
			delete start;
			delete lp;
			return (err);
		}
	}

	// If not cutting end of file, insert end extent
	if (!Undefined(topos) && (topos < oldlen)) {
		end = new AudioExtent(old, topos, AUDIO_UNKNOWN_TIME);
		if (end == NULL)
			return (AUDIO_UNIXERROR);
		err = lp->Append(end);
		if (err) {
			delete end;
			delete lp;
			return (err);
		}
	}
	// XXX - update cached detection array
	ulp->addentry(lp);
	return (AUDIO_SUCCESS);
}

// Get the sound characterization for the current list entry.
// Returns audio error code or AUDIO_SUCCESS.
int
list_getdetect(
	Undolist*	ulp,		// list object
	AudioDetectPts*& vptr)		// address of array pointer
{
	vptr = ulp->currentdetect();

	// If no detection array, calculate one
	if (vptr == NULL) {
		AudioDetect*	state;
		Audio*		ap;
		AudioError	err;

		ap = ulp->currentaudio();
		if (ap == NULL)
			return (AUDIO_ERR_NOEFFECT);
		state = ulp->detectstate();
		err = state->Analyze(vptr, ap);
		if (!err)
			ulp->setcurrentdetect(vptr);
		return (err);
	}
	return (AUDIO_SUCCESS);
}

// Get a configuration parameter for the audio detection code.
// [See AudioDetect::GetParam() for 'type' and 'valp' description.]
// Returns the value.
double
list_getdetectconfig(
	Undolist*		ulp,		// list object
	AudioDetectConfig	type)		// type flag
{
	AudioDetect*	dp;
	Double		val;

	dp = ulp->detectstate();
	(void) dp->GetParam(type, val);
	return ((double)val);
}

// Configure the audio detection code.
// [See AudioDetect::SetParam() for 'type' and 'valp' description.]
// Returns audio error code or AUDIO_SUCCESS.
int
list_configdetect(
	Undolist*		ulp,		// list object
	AudioDetectConfig	type,		// type flag
	double			valp)		// value
{
	AudioDetect*	dp;
	AudioError	err;

	dp = ulp->detectstate();
	err = dp->SetParam(type, valp);
	if (!err) {
		// Clear cached detection arrays
		ulp->cleardetect();
	}

	return (AUDIO_SUCCESS);
}

// Remove silent ends, if any, from the current list entry.
// Returns audio error code or AUDIO_SUCCESS.
// Returns AUDIO_ERR_NOEFFECT if there was no silence to trim.
int
list_trimends(
	Undolist*	ulp,		// list object
	AudioDetectPts*	vptr)		// detection array
{
	Audio*		old;
	AudioList*	lp;
	Double		oldlen;
	AudioExtent*	save;
	Double		start;
	Double		end;
	AudioError	err;

	old = ulp->currentaudio();
	if ((old == NULL) || (vptr == NULL))
		return (AUDIO_ERR_NOEFFECT);

	oldlen = old->GetLength();
	if (Undefined(oldlen))
		return (AUDIO_ERR_BADARG); // indeterminate list length?!

	// Calculate the new start and end times
	if (vptr[0].type == DETECT_SILENCE) {
		start = vptr[1].pos;
	} else {
		start = 0.0;
	}
	if (vptr[0].type == DETECT_EOF) {
		end = vptr[0].pos;
	} else {
		// Skip to end of list
		while (vptr[1].type != DETECT_EOF)
			vptr++;
		if (vptr[0].type == DETECT_SILENCE) {
			end = vptr[0].pos;
		} else {
			end = vptr[1].pos;
		}
	}
	// If no change, don't make a new undolist entry
	if ((start == 0.0) && (end >= oldlen))
		return (AUDIO_ERR_NOEFFECT);

	// Remove data by referencing only what's wanted
	lp = new AudioList;
	if (lp == NULL)
		return (AUDIO_UNIXERROR);

	// Insert new extent
	save = new AudioExtent(old, start, end);
	if (save == NULL)
		return (AUDIO_UNIXERROR);
	err = lp->Insert(save);
	if (err) {
		delete save;
		delete lp;
		return (err);
	}

	// XXX - update cached detection array
	ulp->addentry(lp);
	return (AUDIO_SUCCESS);
}

// Remove all silent segments, if any, from the current list entry.
// Returns audio error code or AUDIO_SUCCESS.
// Returns AUDIO_ERR_NOEFFECT if there was no silence to trim.
int list_trimsilence(
	Undolist*	ulp,		// list object
	AudioDetectPts*	vptr)		// detection array
{
	Audio*		old;
	AudioList*	lp;
	Double		oldlen;
	AudioDetect*	dp;
	Double		halfsilence;
	Double		start;
	Double		end;
	AudioExtent*	save;
	AudioError	err;

	old = ulp->currentaudio();
	if ((old == NULL) || (vptr == NULL))
		return (AUDIO_ERR_NOEFFECT);

	oldlen = old->GetLength();
	if (Undefined(oldlen))
		return (AUDIO_ERR_BADARG); // indeterminate list length?!

	// Calculate half of the silence detection minimum silence
	dp = ulp->detectstate();
	(void) dp->GetParam(DETECT_MINIMUM_SILENCE, halfsilence);
	halfsilence = halfsilence / 2.;

	// Remove data by referencing only what's wanted
	lp = new AudioList;
	if (lp == NULL)
		return (AUDIO_UNIXERROR);

	// Loop through all segments, inserting sound
	while (vptr->type != DETECT_EOF) {
		if (vptr->type == DETECT_SOUND) {
			start = vptr->pos - halfsilence;
			if (start < 0.)
				start = 0.;
			end = vptr[1].pos + halfsilence;
			if (end > oldlen)
				end = oldlen;
			save = new AudioExtent(old, start, end);
			if (save == NULL) {
				delete lp;
				return (AUDIO_UNIXERROR);
			}
			err = lp->Append(save);
			if (err) {
				delete save;
				delete lp;
				return (err);
			}
		}
		vptr++;
	}

	// If no change, throw away this new list
	if (oldlen == lp->GetLength()) {
		delete lp;
		return (AUDIO_ERR_NOEFFECT);
	}

	// XXX - update cached detection array
	ulp->addentry(lp);
	return (AUDIO_SUCCESS);
}


// Remove a all but a specified section of audio data from a list.
// Returns audio error code or AUDIO_SUCCESS.
int
list_prune(
	Undolist*	ulp,		// list object
	double		frompos,	// start time of section to leave
	double		topos)		// end time of section to leave
{
	Audio*		old;
	AudioList*	lp;
	Double		oldlen;
	AudioExtent*	save;
	AudioError	err;

	old = ulp->currentaudio();
	if (old == NULL)
		return (AUDIO_ERR_NOEFFECT);

	oldlen = old->GetLength();
	if (Undefined(oldlen))
		return (AUDIO_ERR_BADARG); // indeterminate list length?!

	// Remove data by referencing only what's wanted
	lp = new AudioList;
	if (lp == NULL)
		return (AUDIO_UNIXERROR);

	// If not cutting start of file, insert beginning extent
	save = new AudioExtent(old, frompos, topos);
	if (save == NULL)
		return (AUDIO_UNIXERROR);
	err = lp->Insert(save);
	if (err) {
		delete save;
		delete lp;
		return (err);
	}

	// XXX - update cached detection array
	ulp->addentry(lp);
	return (AUDIO_SUCCESS);
}

// Combined cut/paste operation.
// Replace a region of audio data from a list.
// Returns audio error code or AUDIO_SUCCESS.
// XXX - Should check *all* error conditions before doing the 'cut'
int
list_replace(
	Undolist*	ulp,		// list object
	double		frompos,	// start time of section to cut
	double		topos,		// end time of section to cut
	Audio*		ap)		// audio list to insert
{
	Audio*		old, *cvtbuf;
	AudioList*	lp;
	Double		oldlen;
	AudioExtent*	start;
	AudioExtent*	end;
	AudioError	err;

	old = ulp->currentaudio();
	if (old != NULL) {
		oldlen = old->GetLength();
		if (Undefined(oldlen)) {
			// indeterminate list length?!
			return (AUDIO_ERR_BADARG);
		}
	} else {
		// If nothing in list, make sure there's no attempt to cut data
		if ((frompos > 0.) || (topos > 0.))
			return (AUDIO_ERR_BADARG);
		oldlen = 0.;
		old = new AudioList;
		ulp->addentry(old);
	}

	// Remove data by referencing only what's left
	lp = new AudioList;
	if (lp == NULL)
		return (AUDIO_UNIXERROR);

	// If not cutting start of file, insert beginning extent
	if (frompos > 0.) {
		start = new AudioExtent(old, 0., frompos);
		if (start == NULL)
			return (AUDIO_UNIXERROR);
		err = lp->Insert(start);
		if (err) {
			delete start;
			delete lp;
			return (err);
		}
	}

	// Insert new region, if specified
	if (ap != NULL) {
		// check if compressed & decompress
		if ((ap->GetHeader().encoding == G721) ||
		    (ap->GetHeader().encoding == G723)) {
			err = list_g72x_decode(ap, cvtbuf);
			if (!err) {
				// caller will deref the original audio object
				ap = cvtbuf;
			}
		}
		if (!err)
			err = lp->Append(ap);
		if (err) {
			delete lp;
			return (err);
		}
	}

	// If not cutting end of file, insert end extent
	if (!Undefined(topos) && (topos < oldlen)) {
		end = new AudioExtent(old, topos, AUDIO_UNKNOWN_TIME);
		if (end == NULL)
			return (AUDIO_UNIXERROR);
		err = lp->Append(end);
		if (err) {
			delete end;
			delete lp;
			return (err);
		}
	}
	// XXX - update cached detection array
	ulp->addentry(lp);
	return (AUDIO_SUCCESS);
}

// Copy a region of the current list entry.
// Returns audio error code or AUDIO_SUCCESS.
int
list_copy(
	Undolist*	ulp,		// list object
	double		frompos,	// start time of section to copy
	double		topos,		// end time of section to copy
	Audio*&		cp)		// returned pointer to audio data
{
	Audio*		obj;
	Audio*		newa;
	AudioError	err;

	obj = ulp->currentaudio();
	if (obj == NULL)
		return (AUDIO_ERR_NOEFFECT);

	// XXX - should reduce to target elements?
	newa = new AudioExtent(obj, frompos, topos);
	if (newa == NULL)
		return (AUDIO_UNIXERROR);
	newa->Reference();
	cp = newa;
	return (AUDIO_SUCCESS);
}

// Write the current list entry to the named file.
// The file is overwritten.  If this is not necessarily desired,
// you must deal with this first (ie, stat the file, put up an alert, etc)
// Returns audio error code or AUDIO_SUCCESS.
int
list_async_writefile(
	Undolist*	ulp,		// list object
	Audio*		output,		// output audio object
	double		*frompos,	// from what position
	double		*topos)		// to what position
{
	int		bsize = 32768;	// how much to write at one time ...
	Audio*		input;
	AudioError	err;
	Double		len, inlen;
	Double		tmp_frompos;
	Double		tmp_topos;

	input = ulp->currentaudio();
	if (input == NULL) {
		return (AUDIO_ERR_NOEFFECT);
	}
	tmp_frompos = *frompos;
	tmp_topos = *topos;

	// how much is left on input file?
	inlen = input->GetLength() - tmp_frompos;

	// If there is less data than our arbitrary limit, copy the rest out
	len = output->GetHeader().Bytes_to_Time(bsize);
	if (inlen < len) {
		len = AUDIO_UNKNOWN_TIME;
	}

	DBGOUT((9, "--list_asyncwrite: trying to write %d bytes\n", bsize));

	err = input->AsyncCopy(output, tmp_frompos, tmp_topos, len);

	DBGOUT((9, "--list_asyncwrite: wrote %d bytes\n", 
		output->GetHeader().Time_to_Bytes(len)));

	*frompos = tmp_frompos;
	*topos = tmp_topos;
	return (err);
}

// Create output file that has given header (and name, of course)
int
list_createfile(
	char*		name,		// output filename
	Audio_hdr*	ohdr,		// output file header
	Audio*&		ofp)		// output file object
{
	AudioFile*	output;
	AudioHdr	hdr(*ohdr);
	AudioError	err;

	if (err = hdr.Validate())
		return (err);

	output = new AudioFile(name, WriteOnly);
	if (output == 0) {
		return (AUDIO_UNIXERROR);
	}

	// Set audio file header and create file
	if ((err = output->SetHeader(hdr)) || (err = output->Create())) {
		delete output;
		return (err);
	}

	// ref before, de-ref after so tmp buf is deleted after write....
	output->Reference();
	ofp = output;
	return (AUDIO_SUCCESS);
}

// Create output pipe from of the same type as the current list object
// with the given name.
int
list_createpipe(
	Undolist*	ulp,
	int		fd,
	char*		name,		// output filename
	Audio*&		ofp)		// output file object
{
	AudioPipe*	output;
	AudioHdr	hdr;
	AudioError	err;
	Audio*		ap;

	ap = ulp->currentaudio();
	if (ap == NULL) {
		return (AUDIO_ERR_NOEFFECT);
	}

	output = new AudioPipe(fd, WriteOnly, name);
	if (output == 0) {
		return (AUDIO_UNIXERROR);
	}

	// Set audio file header and create file
	if ((err = output->SetHeader(ap->GetHeader())) ||
	    (err = output->Create())) {
		delete output;
		return (err);
	}
	
	output->SetBlocking(TRUE);

	// ref before, de-ref after so tmp buf is deleted after write....
	output->Reference();

	ofp = output;

	return (AUDIO_SUCCESS);
}

// Start a 'play' operation for the current list entry.
// This routine queues data to the output device until it blocks.
// The list_async() routine must be called when SIGPOLL is caught
// to keep the device queues full.
// Velocity < 0 means:  play in reverse.
// Returns audio error code or AUDIO_SUCCESS.
int
list_playstart(
	Undolist*	ulp,		// list object
	char*		devname,	// output device name
	double		leftpos,	// left end of region
	double		rightpos,	// right end of region
	double		startpos,	// starting time
	double		velocity)	// play speed (1. is real-time)
{
	PlayRecord*	prp;
	Audio*		ap;
	AudioError	err;

	ap = ulp->currentaudio();
	if (ap == NULL)
		return (AUDIO_ERR_NOEFFECT);

	// Get player object and set up for output
	prp = ulp->getplayrec();

	err = prp->play_init(devname, ap, leftpos, rightpos, startpos,
	    velocity);
	return (err);
}


// Flush 'play' and 'record' queues..
// Returns audio error code or AUDIO_SUCCESS.
int
list_flush(
	Undolist*	ulp)		// list object
{
	PlayRecord*	prp;

	// Get player object and flush it
	prp = ulp->getplayrec();
	return (prp->flush());
}


// Return TRUE if output to the device has drained.
int
list_eof(
	Undolist*	ulp)		// list object
{
	PlayRecord*	prp;

	// Get player object
	prp = ulp->getplayrec();
	return ((int)prp->eof());
}


// Stop 'play' and 'record' operation.
// Returns audio error code or AUDIO_SUCCESS.
int
list_stop(
	Undolist*	ulp)		// list object
{
	PlayRecord*	prp;

	// Get player object and stop it
	prp = ulp->getplayrec();
	return (prp->stop());
}


// Continue 'play' and 'record' operations.
// Call this routine every time SIGPOLL is caught.
// Returns audio error code or AUDIO_SUCCESS.
// A return of AUDIO_ERR_INTERRUPTED means there is more copying to do.
int
list_async(
	Undolist*	ulp)		// list object
{
	PlayRecord*	prp;

	// Get player object and kick it
	prp = ulp->getplayrec();
	return (prp->async());
}


// Reset operating parameters for a play in-progress.
// Returns audio error code or AUDIO_SUCCESS.
int
list_playreset(
	Undolist*	/*ulp*/,	// list object
	double		/*frompos*/,	// new position [or AUDIO_UNKNOWN_TIME]
	double		/*velocity*/,	// new play speed
	int		/*forward*/)	// TRUE: play forward, FALSE: reverse
{
	return (AUDIO_SUCCESS);
}


// Return current data position for a play in-progress.
// This refers to the position of the data being played, rather than
// the position of the input file currently being queued.
double
list_playposition(
	Undolist*	ulp)		// list object
{
	PlayRecord*	prp;

	// Get player object and return position
	prp = ulp->getplayrec();
	return (prp->play_position());
}



// Return current audio level for a position in the current buffer
AudioLevel
list_getlevel(
	Undolist*	ulp,		// list object
	double		pos,		// start position
	double		window)		// window length
{
	AudioBuffer*	buf;
	AudioGain	gp;
	AudioLevel	lev;

	lev.level = 0.;
	lev.clip = FALSE;
	if (list_getlength(ulp) == 0.)
		return (lev);

	// Get data to scan for volume
	buf = list_getrange(ulp, pos, window);

	// If error scanning, fake a result
	if ((buf != NULL) &&
	    (gp.Process(buf, AUDIO_GAIN_INSTANT) == AUDIO_SUCCESS)) {
		lev.level = gp.InstantGain();
		lev.clip = gp.Clipped();
	}
	return (lev);
}

// Return current audio level for a play in-progress.
AudioLevel
list_playlevel(
	Undolist*	ulp,		// list object
	double		pos)		// position
{
	// XXX - dynamically set meter time for run-time tuning
	meter_window = 1. / (double) meter_time;

	return (list_getlevel(ulp, pos, meter_window));
}

// Return audio level for a record in-progress.
AudioLevel
list_recordlevel(
	Undolist*	ulp,		// list object
	double		pos)		// position
{
	// XXX - dynamically set meter time for run-time tuning
	meter_window = 1. / (double) meter_time;

	return (list_getlevel(ulp, pos - meter_window, meter_window));
}


// Start a 'record' operation for the current list entry.
// The list_async() routine must be called when SIGPOLL is caught
// to keep the device queues empty.
// Returns audio error code or AUDIO_SUCCESS.
int
list_recordstart(
	Undolist*	ulp,		// list object
	char*		devname,	// input device name
	char*		tmpdir,		// temporary file directory (or NULL)
	Audio_hdr*	ohdr,		// output file header
	double		frompos,	// start time of region to cut
	double		topos)		// end time of region to cut
{
	PlayRecord*	prp;
	AudioFile*	ap;
	AudioHdr	hdr(*ohdr);
	AudioError	err;

	if (err = hdr.Validate())
		return (err);

	// Set temporary file directory before creating new file
	if (tmpdir != NULL) {
		err = AudioFile::SetTempPath(tmpdir);
		if (err != AUDIO_SUCCESS)
			return (err);
	}

	// Create new file
	ap = new AudioFile;
	if (ap == NULL)
		return (AUDIO_UNIXERROR);
	ap->Reference();

	// Get player object and set up for input
	prp = ulp->getplayrec();

	// Open output file, cut the marked region, insert this new file,
	// and set up the record device
	if ((err = ap->SetHeader(hdr)) || (err = ap->Create()))
		goto errret;
	if (err = prp->record_init(devname, ap, 0., AUDIO_UNKNOWN_TIME)) {
		err = AUDIO_ERR_NOTDEVICE;
		goto errret;
	}
	if (err = list_replace(ulp, frompos, topos, ap))
		goto errret;

	// When the list entry is released, the temp file goes away
errret:
	ap->Dereference();
	return (err);
}


// Return current data position for a record in-progress.
// This corresponds to the position in the current list, not
// the amount of data actually converted.
double
list_recordposition(
	Undolist*	ulp)		// list object
{
	PlayRecord*	prp;

	// Get player object and return position
	prp = ulp->getplayrec();
	return (prp->record_position());
}

// Return current data position for a record in-progress.
// This corresponds to the amount of data actually converted.
double
list_recordbufposition(
	Undolist*	ulp)		// list object
{
	PlayRecord*	prp;

	// Get player object and return position
	prp = ulp->getplayrec();
	return (prp->record_bufposition());
}


#ifdef notdef
// Adjust a selection point to the nearest zero-crossing.
// Takes a time, in seconds, of the target point.
// Returns the time, in seconds, of the adjysted target point.
// Calculations are done relative to the current list entry.
// Times within milliseconds of the start (or end) of file are set
// to the start (or end) of the file.
double
list_adjustpt(
	double		pos)	// time, in seconds of target position
{
	// XXX - implement!
	return (pos);
}

// Initialize for a 'scrub' operation for the current list entry.
// Call list_scrub() to actually perform the scrubbing.
// Returns audio error code or AUDIO_SUCCESS.
int
list_scrubstart(
	Undolist*	/*ulp*/,	// list object
	char*		/*devname*/)	// output device name
{
	return (AUDIO_SUCCESS);
}


// Perform a 'scrub' operation for the current list entry.
// 'Otime' is the desired output time;  that is, the data
// between the given positions will be played in 'otime' seconds.
// Returns audio error code or AUDIO_SUCCESS.
int
list_scrub(
	Undolist*	/*ulp*/,	// list object
	double		/*frompos*/,	// start time
	double		/*topos*/,	// end time (could be backwards)
	double		/*otime*/)	// output time
{
	return (AUDIO_SUCCESS);
}
#endif /* notdef */

}				//end of extern "C"
