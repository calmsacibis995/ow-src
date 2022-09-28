%
% @(#)dv_cps.cps	1.3 02/23/93 Copyright 1990-1992 Sun Microsystems, Inc.

%
% Initialize the docviewer->NeWS connection
%

#ifdef	DEBUG
cdef ps_InitNeWS(int debugBoolean)
#else
cdef ps_InitNeWS()
#endif


%
% Initialize Variables
%

#ifdef	DEBUG
	debugBoolean 0 eq {
	    /debugOn false def			% Turn debugging off
	}{
	    /debugOn true def			% Turn it on
	} ifelse
#endif

%
% Define PostScript Procedures
%

#ifdef   DEBUG
	/DebugPrintf {				% fmtString [array] => -
	    debugOn { 				%ifelse
		dup
		type /stringtype eq {		%ifelse
		    console exch
		}{				%else
		    console 3 1 roll
		} ifelse
		fprintf
	    }{				    	% debugOn is false, pop arguments
		type /stringtype ne {		%if
		    pop
		} if
	    } ifelse
	} def

(Initialising server ... ) DebugPrintf
#endif

#ifdef	DEBUG
(end initialisation\n) [] DebugPrintf
#endif

%
% Limit the amount of detail in default error handler's
% error report to be the minimum.
%
currentprocess /ErrorDetailLevel 0 put

%
% CPS routines with C interfaces
%

%
% Erase the canvas
%
cdef ps_ClearCanvas()
#ifdef	DEBUG
	console (ps_ClearCanvas: entered\n) [] fprintf
#endif
	gsave
	    newpath clippath 1.0 setgray fill
	grestore

cdef ps_ShowPage()
	newpath initclip 1 setlinewidth 0 setlinecap 0 setlinejoin
	[] 0 setdash 0 setgray 10 setmiterlimit
	clear
%
%
%
cdef ps_dvRestoreObject()
#ifdef	DEBUG
	console (ps_Restore: entered\n) [] fprintf
	clear
	{
	    currentdict userdict ne { end } { exit } ifelse
	} loop
	userdict /dv_SaveObject known
	{
		userdict /dv_SaveObject get restore
	} {
	    console (ps_Restore: /dv_SaveObject unknown!!\n) [] fprintf
	} ifelse
#else
	{
	    currentdict userdict ne { end } { exit } ifelse
	} loop
	userdict /dv_SaveObject known
	{
	    userdict /dv_SaveObject get restore
	} if
#endif

cdef ps_dvSaveObject()
#ifdef	DEBUG
	console (ps_Save: entered\n) [] fprintf
#endif
	userdict /dv_SaveObject save put

%
% Scale the Canvas to scaleVal
%
cdef ps_ScaleCanvas(float scaleVal)
#ifdef	DEBUG
	console (ps_ScaleCanvas: scaling canvas by %\n)
		scaleVal 1 array astore fprintf
#endif
	scaleVal scaleVal scale

%
% Translate the origin
%
cdef ps_XlateOrigin(int x, int y)
#ifdef	DEBUG
	console (ps_XlateOrigin: xlate origin to % %\n) x y 2 array astore
	fprintf
#endif
	x y translate

cdef ps_GRestore()
	grestore

cdef ps_GSave()
	gsave
