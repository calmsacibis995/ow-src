# Makefile for C++ Audio Library (libAudio.a) 
#
# @(#)Makefile	1.18	97/01/14 SMI
#

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg

TARGETS		= library
DOMAIN		= SUNW_AUDIO_MESSAGES

LIBRARY		= Audio

LIBCSRCS        = filehdr.c \
		  device_ctl.c \
		  hdr_misc.c \
		  silence.c \
		  g721.c \
		  g723.c \
		  tsm.c \
		  zmalloc.c

LIBCCSRCS	= Audio.cc \
		  AudioHdr.cc \
		  AudioHdrParse.cc \
		  AudioExtent.cc \
		  AudioList.cc \
		  AudioStream.cc \
		  AudioBuffer.cc \
		  AudioUnixfile.cc \
		  AudioFile.cc \
		  AudioPipe.cc \
		  AudioRawPipe.cc \
		  AudioDevice.cc \
		  AudioDevicectl.cc \
		  AudioCopy.cc \
		  AudioLib.cc \
		  AudioDetect.cc \
		  AudioTypePcm.cc \
		  AudioTypeG72X.cc \
		  AudioTypeChannel.cc \
		  AudioTypeMux.cc \
		  AudioTypeSampleRate.cc \
		  AudioGain.cc \
		  Fir.cc \
		  Resample.cc \
		  ProcessTimer.cc \
		  AudioError.cc \
		  AudioDebug.cc \
		  libaudio_c.cc

# Dynamically generated tables
ASRC		= g711.c
CSRC		= cosmult.c
FSRC		= g72x_tables.c

GENSRCS		= ${ASRC} ${CSRC} ${FSRC}

LIBSRCS		= ${LIBCSRCS} ${LIBCCSRCS} ${GENSRCS}

INLINE_HDRS	= Audio_inline.h \
		  AudioStream_inline.h \
		  AudioUnixfile_inline.h

PUB_HDRS	= libaudio.h \
		  audio_errno.h \
		  audio_hdr.h \
		  audio_filehdr.h \
		  audio_encode.h \
		  audio_device.h \
		  audio_types.h \
		  archdep.h \
		  AudioDetect.h \
		  AudioTypes.h \
		  AudioError.h \
		  AudioDebug.h \
		  Audio.h \
		  AudioHdr.h \
		  AudioStream.h \
		  AudioUnixfile.h \
		  AudioFile.h \
		  AudioDevice.h \
		  AudioDevicectl.h \
		  AudioPipe.h \
		  AudioRawPipe.h \
		  AudioBuffer.h \
		  AudioExtent.h \
		  AudioList.h \
		  AudioTypeConvert.h \
		  AudioTypePcm.h \
		  AudioTypeG72X.h \
		  AudioTypeChannel.h \
		  AudioTypeMux.h \
		  AudioTypeSampleRate.h \
		  AudioGain.h \
		  Fir.h \
		  Resample.h \
		  ProcessTimer.h \
		  AudioLib.h \
		  libaudio_c.h \
		  zmalloc.h

PRIV_HDRS	= libaudio_impl.h \
		  silence_detect.h \
		  tsm.h

# These are the (only) header files that actually get installed
#
FILES		=  archdep.h libaudio.h audio_errno.h audio_hdr.h \
		   audio_filehdr.h audio_device.h audio_encode.h \
		   audio_types.h

LIBHDRS		= ${PUB_HDRS} ${PRIV_HDRS} ${INLINE_HDRS}


LIBOBJS		= ${LIBCSRCS:%.c=%.o} ${GENSRCS:%.c=%.o} ${LIBCCSRCS:%.cc=%.o}

INCLUDE		= -I. -I/opt/SUNWspro/SC4.0/include/cc
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"'

DIR.install.files = ${DEST.audio.demo.include.multimedia}

include ${BASEDIR}/Makefile.rules

# Build rules for generating static tables.
# NOTE: ${OBJDIR} expands to obj-${ARCH}. We don't care about debugged,
# profiled, or optimized variants of the generation programs.

AGENPROG	= ${OBJDIR}/gen_${ASRC:%.c=%}
CGENPROG	= ${OBJDIR}/gen_${CSRC:%.c=%}
FGENPROG	= ${OBJDIR}/gen_${FSRC:%.c=%}

${AGENPROG}: gen_${ASRC}
	?${CC} -o $@ ${INCLUDE} ${CFLAGS} ${DEFS} ${SYSDEFS} \
		  gen_${ASRC} ${LDFLAGS} -lm

${ASRC}: ${AGENPROG}
	?${AGENPROG} > $@

${CGENPROG}: gen_${CSRC} ${ASRC}
	?${CC} -o $@ ${INCLUDE} ${CFLAGS} ${DEFS} ${SYSDEFS} \
		  gen_${CSRC} ${ASRC} ${LDFLAGS} -lm

${CSRC}: ${CGENPROG}
	?${CGENPROG} > $@

${FGENPROG}: gen_${FSRC}
	?${CC} -o $@ ${INCLUDE} ${CFLAGS} ${DEFS} ${SYSDEFS} \
		  gen_${FSRC} ${LDFLAGS} -lm

${FSRC}: ${FGENPROG}
	?${FGENPROG} > $@
