%
% @(#)xvpscan_cps.cps	1.3 11/11/93 Copyright 1990-1992 Sun Microsystems, Inc.
%

%
% Definitions
%
% C:#define DONE_TAG 2005
#define	FILLBOX		1
#define	XOR_OPCODE	6
#define	SRC_OPCODE	12
#define	BUFSIZ	 4096
#define DONE_TAG 2005
#define VALUETAG 2006
#define ERRORTAG 2007


cdef ps_init_xvpscan()
%
% Given a bbox draw a rectangle around it or fill it
%
/drawbox {	  		% type bbox_llx bbox_lly bbox_urx bbox_ury => -
	newpath			% type bbox_llx bbox_lly bbox_urx bbox_ury
	points2rect		% type bbox_llx bbox_lly w h
	rectpath		% type
	closepath		% type
	XOR_OPCODE setrasteropcode % type
	FILLBOX eq {		% -
		fill		% Fill the box
	}{
		1 setlinewidth	% Draw an outline
		stroke
	} ifelse
	SRC_OPCODE setrasteropcode
} def

#ifdef	DEBUG
(end initialisation\n) [] DebugPrintf
#endif

%
% CPS routines with C interfaces
%

%
% Draw a PostScript bounding box
%
cdef ps_DrawBox(int type,
		int bbox_llx, int bbox_lly, int bbox_urx, int bbox_ury)
#ifdef DEBUG
console (ps_DrawBox: type = % bbox = \(% % % %\)\n)
	type bbox_llx bbox_lly bbox_urx bbox_ury 5 array astore fprintf
#endif
gsave type bbox_llx bbox_lly bbox_urx bbox_ury drawbox grestore


% Change the drawable to the pixmap
cdef ps_ChangeDrawable(int xid)
	xid xlookupid not { framebuffer } if
	setcanvas


%
% Download a couple of functions into the server
% All PostScript routines downloaded to the server begin with "_xvps_"
%
cdef pscanvas_init_canvas()
%
% Flip the coordinate system
%
/_xvps_flip_coords {	% - => -
    clippath pathbbox
    0 exch translate pop pop pop
    1 -1 scale
} def

%
% Re-define initgraphics. If an initgraphics is allowed to go through,
% the canvas ends up in the XView coordinate system
%
/_xvps_initgraphics { initgraphics } def
/initgraphics {		    		% - => -
    _xvps_initgraphics
    currentcanvas dup /XID known {
	/XID get 0 ne {
	    _xvps_flip_coords
	} if
    }{
	pop
    } ifelse
} def

%
% Scroll the canvas and return the current clipping path to the client
%
#ifdef	DEBUG
/_xvps_scroll {			% dx dy => x y w h
    console (_xvps_scroll: entered\n) writestring
    clippath 2 copy copyarea		% dx dy
    currentcanvas false getbbox		% dx dy 0 0 w h
    2 copy 6 2 roll			% dx dy w h 0 0 w h
    rectpath				% dx dy w h
    rectpath				% -
    eoclip
    %
    % Get the current clipping path, transform it into device coordinates, and
    % leave them on the stack so that the client can pick them up
    %
    clippath pathbbox	    	    	% x0 y0 x1 y1
    points2rect				% x y w h
    %
    % Print out the pathbbox
    %
    4 copy 4 array astore console exch
    (_xvps_scroll: pathbbox: \(x=% y=% w=% h=%\)\n)
    exch fprintf
    %
    % Transform the do-hickeys into device space, and cvi them
    %
    dtransform	    	    	    	% x y w' h'
    cvi 4 1 roll			% h' x y w'
    cvi 4 1 roll			% w' h' x y
    transform				% w' h' x' y'
    cvi 4 1 roll			% y' w' h' x'
    cvi 4 1 roll			% x' y' w' h'
    %
    % Print it out again
    %
    4 copy 4 array astore console exch
    (_xvps_scroll: pathbbox = \(x'=% y'=% w'=% h'=%\)\n)
    exch fprintf
} def
#else
/_xvps_scroll {			% dx dy => x y w h
    clippath 2 copy copyarea		% dx dy
    currentcanvas false getbbox		% dx dy 0 0 w h
    2 copy 6 2 roll			% dx dy w h 0 0 w h
    rectpath				% dx dy w h
    rectpath				% -
    eoclip
    %
    % Get the current clipping path, transform it into device coordinates, and
    % leave them on the stack so that the client can pick them up
    %
    clippath pathbbox	    	    	% x0 y0 x1 y1
    points2rect				% x y w h
    %
    % Transform the do-hickeys into device space, cvi them, and leave
    % them on the stack
    %
    dtransform	    	    	    	% x y w' h'
    cvi 4 1 roll			% h' x y w'
    cvi 4 1 roll			% w' h' x y
    transform				% w' h' x' y'
    cvi 4 1 roll			% y' w' h' x'
    cvi 4 1 roll			% x' y' w' h'
} def
#endif


%
% Print warning message on the canvas
%
/_xvps_PrintErrorMsg {	    	% errString => -
    %
    % Print errString on the console
    %
    console exch writestring		% -
    %
    % Warn the user about the error
    %
    gsave
    newpath currentcanvas false getbbox rectpath
    1.0 setgray fill
    %
    % Print Warning on the canvas ...
    %
    .75 setgray				% set a nice gray
    /Courier-Bold findfont 32 scalefont setfont
    currentcanvas false getbbox	    	% 0 0 w h
    4 1 roll pop pop pop .5 mul		% .5h
    currentfont fontheight 1.2 mul exch	% 1.2fontheight .5h
    translate 0 0 moveto		% -
    (Warning: NeWS/PostScript Error!) show
    %
    % Print error message asking the user to look at the console
    %
    0 currentfont fontheight -1.2 mul  % -1.2fontheight
    translate 0 0 moveto	       % -
    (See console for stack trace) show
    grestore
} def

%
% End of initialization
%

%
% Before doing anything else, establish our package
% stack.
%
cdef ps_init_connection()
/owV3? systemdict /findpackage known def
owV3? {
    /NeWS	3 0 findpackage beginpackage
    /X11	3 0 findpackage beginpackage
} if



%
% Initialize some stuff, and start the server loop
%
cdef pscanvas_start_server_loop()
%
% Make sure we get the correct type of error messages
%
currentprocess /ErrorDetailLevel 1 put
%
% Make NeWS ignore timeouts
%
%statusdict begin
%    0 setjobtimeout
%end
%
% Load up some packages if we are under OW3.0
%

owV3? {
    /fillcanvas {		% grayvalue => -
	gsave
	    setgray
	    clipcanvaspath setgray fill
	    fill
	grestore
    } def
} if
%
% token 0 is a Boolean		- true, if an error has occurred
% token 1 is a string		- points to the error message string
% token 2 is a save object	- points to the initial VM state
%
false	0 setfileinputtoken
()	1 setfileinputtoken
save	2 setfileinputtoken
%
% Start the server loop
%
{
    {
	currentfile cvx exec
    }
    stopped {
	clear				% clear the stack
	(PScanvas NeWS Error:\n***ERROR***\n%*****)
	[
	    $error /message get dup null eq
	    {
		pop ( )
	    } if
	] sprintf   	    	    	% newErrStr
	dup _xvps_PrintErrorMsg    	% newErrStr
#ifdef undef
	dup length			% newErrStr nlen
	1 getfileinputtoken dup length	% newErrStr nlen oldErrStr olen
	%
	% If the old error string is big enough, copy the new error
	% string into it
	%
	3 -1 roll lt			% newErrStr oldErrStr
	{
	    pop	    	    	    	% newErrStr
	}
	{
	    copy    	    	    	% copy newErrStr in oldErrStr
	} ifelse
#endif
	1 setfileinputtoken 	    	% save a pointer to ErrStr
	true 0 setfileinputtoken	% an error has occurred
    }
    {   % gets here if the C side goes away...
	2 getfileinputtoken restore     % restore the VM state
	owV3?
	{
	    endpackage
	    endpackage
	} if
	exit				% exit the loop
    } ifelse
} loop


cdef ps_waitcontext ()
    DONE_TAG tagprint flush

cdef ps_resetmatrix (int hght)
   [ 1 0 0 -1 0 hght ] setmatrix
