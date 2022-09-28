#include <stdio.h>
#include <stream.h>

#ident "@(#)dpsdebug.cc	1.3 06/11/93 Copyright 1992 Sun Microsystems, Inc."


#ifdef DEBUG
int erasePageCount = 0;
int gsaveCount     = 0;
int grestoreCount  = 0;
int saveCtxCount   = 0;
int waitCtxCount   = 0;
int writeDataCount = 0;
int scaleCount     = 0;
int translateCount = 0;
#endif
void
DPSDbgFunc(char* message)
{
#ifdef DEBUG
  cout << message << endl;
#endif
}
