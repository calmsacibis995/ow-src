#ifndef _DPS_DEBUG_H
#define _DPS_DEBUG_H

#ident "@(#)dpsdebug.h	1.3 06/11/93 Copyright 1992 Sun Microsystems, Inc."



typedef enum {
  ERASEPAGE = 0,
  GSAVE,
  GRESTORE,
  SAVECTX,
  WAITCTX,
  WRITEDATA,
  SCALE,
  TRANSLATE
} DPSOperation;
#ifdef DEBUG
extern int erasePageCount;
extern int erasePageCount;
extern int gsaveCount;
extern int grestoreCount;
extern int saveCtxCount;
extern int waitCtxCount;
extern int writeDataCount;
extern int scaleCount;
extern int translateCount;
#endif

extern
void
DPSDbgFunc(char* message);


#endif _DPS_DEBUG_H
