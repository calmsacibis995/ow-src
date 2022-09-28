
%
%  @(#)imagetool_ps.cps 1.9 93/02/09
%
%  Copyright (c) 1992 - Sun Microsystems Inc.
%
%

#include "tags.h"
C: #include "tags.h"

%% fileinputtoken usage for PostScript rendering connection:
%% 0: transformation matrix for page
%% 1: save object for initial state.
%% 2: number of pages found in document
%% 3: page number looking for (when rendering)
%%    we always render that page number plus the one after
%% 4: total pages found so far (when rendering)
%% 5: last small canvas is located here also 
%% 6: X canvas to render onto
%% 7: X canvas to copy onto 
%% 8: when paging - did we (some number) or did we not (0) render the page and
%% 9: filename being rendered (nullstring if remote, filename otherwise)
%% 10: true if we think pages are in reverse order
%% 11: when paging - did we stop (1) or get an error (0)
%% 12: when paging - page number having last error 
%% 13: Saved ctm (needed if rendering small pages for page overview)
%% 14: #1 pixmap for PageView pop up
%% 15: #2 pixmap for PageView pop up
%% 16: #3 pixmap for PageView pop up
%% 17: #4 pixmap for PageView pop up
%% 18: #5 pixmap for PageView pop up
%% 19: #6 pixmap for PageView pop up
%% 20: #7 pixmap for PageView pop up
%% 21: #8 pixmap for PageView pop up
%% 22: #9 pixmap for PageView pop up
%% 23: #10 pixmap for PageView pop up
%% 24: #11 pixmap for PageView pop up
%% 25: #12 pixmap for PageView pop up
%% 26: #13 pixmap for PageView pop up
%% 27: #14 pixmap for PageView pop up
%% 28: #15 pixmap for PageView pop up
%% 29: #16 pixmap for PageView pop up
%% etc...

% Various functions we may need..

cdef ps_getoutput (string output) => OUTPUT_TAG (output)
cdef ps_geterror (string error) => ERROR_TAG (error)
cdef ps_geterrorpage (int pageno) => ERROR_PAGE_TAG (pageno)

%%
%% ps_get_current_ctm,
%% ps_put_current_ctm - stores the current ctm. Needed so can
%%			reset the ctm after rendering small
%%			pixmaps for page overview pop up.
%%

cdef ps_get_current_ctm ()
	0 origfile _getfileinputtoken
	13 origfile _setfileinputtoken

cdef ps_put_current_ctm ()
	13 origfile _getfileinputtoken
	0 origfile _setfileinputtoken

%%
%% ps_set_pages - used if we had to close the connection to the
%%		  server and then reopen it... in the mean time,
%%		  fileinputtoken 2 has been cleared so we no
%%		  longer know the number of pages in the document
%%		  This function sets it back to the correct number.
%%

cdef ps_set_pages (int pages)
	pages 2 origfile _setfileinputtoken

cdef ps_getpages (int pages) => TOTAL_PAGES_TAG (pages)
	%
	% We used fileinputtoken 4 to count pages, now copy into
	%  fileinputtoken 2.
	%
	TOTAL_PAGES_TAG _tagprint 
	4 origfile _getfileinputtoken dup
	2 origfile _setfileinputtoken _typedprint _flush
	
cdef ps_pagerendered (int pageno) => RENDER_PAGE_TAG (pageno)
	%
	% if fileinputtoken 8 is still zero, we didn't render a page, so
	%  copy from largecan to x pixmap - this may not even be the page we 
	%  want!
	%
	8 origfile _getfileinputtoken dup 0 eq {
	    7 origfile _getfileinputtoken _setcanvas
	    gsave clippath pathbbox _rectpath 1 setgray fill grestore
	    6 origfile _getfileinputtoken _imagecanvas
	    pop 1
	} if
	RENDER_PAGE_TAG _tagprint _typedprint _flush

cdef ps_render_done ()
	RENDER_DONE_TAG _tagprint _flush

cdef ps_render_small_done ()
	RENDER_SMALL_DONE_TAG _tagprint _flush

cdef ps_done ()
	DONE_TAG _tagprint _flush

cdef ps_all_done ()
	ALL_DONE_TAG _tagprint _flush
	9 origfile _getfileinputtoken _nullstring eq {
	    _quit
	} if

cdef ps_reset_ctm (float a, float b, float c, float d, float tx, float ty)
	[a b c d tx ty] 0 origfile _setfileinputtoken

cdef ps_zoom (float scale_amt, int height)
	[ 0 origfile _getfileinputtoken aload pop pop height] dup
	0 origfile _setfileinputtoken
	setmatrix
	scale_amt scale_amt scale
	6 array currentmatrix 0 origfile _setfileinputtoken

cdef ps_rotate (int angle, int x, int y)
	[ 0 origfile _getfileinputtoken aload pop pop pop x y ] 
	dup 0 origfile _setfileinputtoken
	setmatrix
	angle rotate
	6 array currentmatrix 0 origfile _setfileinputtoken

cdef ps_hflip (int height)
	[ 0 origfile _getfileinputtoken aload pop pop height] dup
	0 origfile _setfileinputtoken
	setmatrix
	1 -1 scale
	6 array currentmatrix 0 origfile _setfileinputtoken

cdef ps_vflip (int width)
	[ 0 origfile _getfileinputtoken aload pop exch pop width exch ] dup
	0 origfile _setfileinputtoken
	setmatrix
	-1 1 scale
	6 array currentmatrix 0 origfile _setfileinputtoken

%%
%% ps_setup - Setup ps environment.
%% 

cdef ps_setup (float a, float b, float c, float d, float tx, float ty,
	       int width, int height, string filename, int remote)

	/NeWS 3 0 findpackage beginpackage

% Debugging.. use myfile to write out stuff
%/myfile (/tmp/foo) (w) file def

% store original ctm

	[ a b c d tx ty ] 0 currentfile setfileinputtoken

% check if remote. If true then store nullstring in fileinputtoken 9

	remote 1 eq {
	    nullstring
	} { 
	    filename 
	} ifelse
	9 currentfile setfileinputtoken

	% Eliminate nasties from our userdict that get
	% inherited from the NeWS listener and libcps.
	currentdict /framebuffer undef
	currentdict /OriginatingHost undef

	% Create a new systemdict and customize
	% it for laserwriter compatibility.
	%
	systemdict maxlength dict
	systemdict exch copy dup begin

	% Store original file as well as other ops we will need
	% in our new systemdict.
	%
	/origfile currentfile def
	/_framebuffer /framebuffer load def
	/_findpackage /findpackage load def
	/_currentprocess /currentprocess load def
	/_beginpackage /beginpackage load def

	% Remove pesky NeWS objects that would
	% create access to NeWS operators and objects
	% and thereby expose us to security problems
	% with "trojan horse" PostScript documents that
	% contain NeWS-specific code.
	%
	currentdict /PackageDirectory undef
	currentdict /findpackage undef
	currentdict /beginpackage undef
	currentdict /clearpackagestack undef
	currentdict /countpackagestack undef
	currentdict /currentpackage undef
	currentdict /definepackage undef
	currentdict /knownpackage undef
	currentdict /packagestack undef
	currentdict /undefinepackage undef
	currentdict /shareddict undef
	currentdict /debugdict undef

	% remove showpage, so if someone does a
	% /showpage load, we always find our defined showpage
	% operator, not the default
	%
	currentdict /showpage undef

	% This is in the OW V3 systemdict as a bug
	% workaround for answerbook which statically
	% linked a version of libxvps with a bug that
	% depends upon currentprocess being in systemdict.
	%
	currentdict /currentprocess undef

	% Ack! This got accidentally left in systemdict in
	% V3 FCS. It will be gone in future releases but
	% we will have to guard against it for the rest of
	% eternity in case we are talking to a V3 server.
	%
	currentdict /rootcanvases undef

	% Yet more goop that shouldn't really be in systemdict.
	%
	currentdict /getnamefromaddr undef
	currentdict /getaddrfromname undef
	currentdict /?def undef

	% Use the new, safe version of the file operator
	% in V3 if it is present. If it's not present then
	% the server is either pre-FCS or the normal file
	% operator has already been made safe (in OW V4).
	% The _file operator is the same as file except
	% it doesn't allow %socket operations and it doesn't
	% allow opening a file for writing.
	%
	statusdict /_file known {
	    statusdict /_file get /file exch def
	} if

	% Need these operators around too.
	%
	/_nullstring /nullstring load def
	/_tagprint /tagprint load def
	/_typedprint /typedprint load def
	/_setfileinputtoken /setfileinputtoken load def
	/_getfileinputtoken /getfileinputtoken load def
	/_isarray { % any => boolean
    	    type dup /arraytype eq  exch /packedarraytype eq or
	} ?def
	/_sprintf /sprintf load def
	/_append /append load def
	/_setcanvas /setcanvas load def
	/_imagecanvas /imagecanvas load def
	/_quit /quit load def

	/_xyadd { % x1 y1 x2 y2 => x1+x2 y1+y2
    	    3 -1 roll add 3 1 roll add exch
	} def

	/_rectpath {     % l b w h => -
    	    3 index 3 index 2 copy                      % l b w h l b l b
    	    moveto                                      % l b w h l b
    	    _xyadd                                       % l b r t
    	    4 -1 roll 1 index                           % b r t l t
    	    lineto                                      % b r t
    	    2 copy lineto                               % b r t
    	    pop exch                                    % r b
    	    lineto                                      % -
    	    closepath                                   % -
	} def

	/serverdict 10 dict def
	serverdict begin
	    /product (NeWS 3.0) def
	    /exitserver { % int => -
		pop
		1 _getfileinputtoken restore
		count { pop } repeat
		countdictstack 2 sub { end } repeat
	    } executeonly def
	end

	% note - we set the timeout value really low, since we
	% loop in ps.c and similate a 60 second timeout.

	statusdict begin
	    120 setjobtimeout
	end

	% I added this because some ps files from adobe looked
	% for /fontalreadydefined in the errordict, and it wasn't
	% there.. I shouldn't have to do this, but enough ps files
	% had this, so I added it.
	errordict begin
	    /fontalreadydefined {} def
	end

	end % new systemdict

	currentprocess /ErrorDetailLevel 1 put
	currentprocess /DictionaryStack get dup
	0 4 -1 roll put
	currentprocess /DictionaryStack 3 -1 roll put

	/_flush {
	    _currentprocess /Stdout get status {
		flush
	    } if
	} def
	/setrealdevice {
	    % work around - this proc required to parse Laser Prep
	    % (AppleDict md) Procset
	} def
	/execjob {
	    % work around - this proc required to parse Laser Prep
	    % (AppleDict md) Procset
	} def
	/quit {
	    % don't let users kill our connection to the server...
	} def
	/#copies 1 def
	/_print { % string => -
	    OUTPUT_TAG _tagprint
	    dup length MAXOUTPUT gt { 0 MAXOUTPUT getinterval } if
	    _typedprint _flush
	} executeonly def
	/print //_print def
	/= {
	    (%\n) _sprintf _print
	} def
	/== //= def
	/writestring { % file string => -
	    1 index (%stdout) (w) file eq {
		exch pop _print
	    } { 
		//writestring
	    } ifelse
	} executeonly def
	/write { % file int => -
	    1 index (%stdout) (w) file eq {
		exch pop cvis _print
	    } { 
		//write
	    } ifelse
	} executeonly def
	/stack {
	    (--top of stack--\n) _print
	    0 1 count 3 sub {
		index (%\n) _sprintf _print
	    } for
	    (--bottom of stack--\n) _print
	} executeonly def

	/pstack //stack def
	/letter {} def
	/note {} def
	/legal {} def
	/a4 {} def
	/b5 {} def
	/lettersmall {} def
	/a4small {} def
	/erasepage {} def
	/findfont {
    	    { //findfont  } stopped {
    	        FontDirectory 1 index known not {
		    /Courier
                } if  
	    //findfont
    	    } if
	} def        
	/eexec where {
	    pop 
	} {
	   /eexec { % file
	       pop
	   } def
	} ifelse

	/initmatrix {
	    0 origfile _getfileinputtoken setmatrix 
	} executeonly def
	/initgraphics { % - => -
	    //initgraphics
	    0 origfile _getfileinputtoken setmatrix 
	} executeonly def
 	/defaultmatrix { % mtx => mtx'
	    0 origfile _getfileinputtoken exch copy
	} executeonly def

	endpackage

	% add this so no one can pop off userdict too!

	_currentprocess /DictionaryStackLock 2 put

        save 1 origfile _setfileinputtoken

	% This is the big loop we use if we are running remotely. 
 	% Basically, we sit here in a loop and just wait for the
	% client to send us postscript.
	%
	9 origfile _getfileinputtoken _nullstring eq {
	    {
		{
	            {
	                 origfile cvx exec
	            } stopped {

		%  ok, we got an error or we stopped for some reason...

		        12 origfile _getfileinputtoken 
		        4 origfile _getfileinputtoken 1 add ne {
			    4 origfile _getfileinputtoken 1 add 
			    12 origfile _setfileinputtoken
                            ERROR_PAGE_TAG _tagprint
			    4 origfile _getfileinputtoken 
                            _typedprint _flush
		        } if
		        clear

		%  check to see if the file is closed.. if so, then quit
		
		       origfile status not {
			   _quit
		       } if

		    } if
		} loop
	    } exec
	} if

%%
%% ps_set_page - set fileinputtokens with xids of server images
%% 		 used on pageview pop up. Always set fileinputtoken
%%		 5 to the canvas too, since it will contain the last one.
%%

cdef ps_set_page (int pageno, int xid)
	/NeWS 3 0 _findpackage _beginpackage
	/X11 3 0 _findpackage _beginpackage
	    xid xlookupid not { _framebuffer } if
	    dup pageno 14 add origfile _setfileinputtoken
	    5 origfile _setfileinputtoken
	endpackage
	endpackage

%%
%% ps_check_for_pages - gets total number of pages in document, while
%%			rendering first page onto pixmap. If we think that
%%			file is in reverse page order, then we still render
%%			the `correct' first page.
%%

cdef ps_check_for_pages (int reversed)
	reversed 10 origfile _setfileinputtoken
	0 8 origfile _setfileinputtoken

% set fileinputtoken 2 and 4 to zero, we haven't found any pages yet.

	0 2 origfile _setfileinputtoken
	0 4 origfile _setfileinputtoken

	0 11 origfile _setfileinputtoken
	0 12 origfile _setfileinputtoken

	/showpage {
	    4 origfile _getfileinputtoken 1 add dup 
	    4 origfile _setfileinputtoken
	    10 origfile _getfileinputtoken 1 eq {
		pop
	        7 origfile _getfileinputtoken _setcanvas
	        gsave clippath pathbbox _rectpath 1 setgray fill grestore
	        6 origfile _getfileinputtoken _imagecanvas
	    } { 
	        1 eq {
	            7 origfile _getfileinputtoken _setcanvas
	            gsave clippath pathbbox _rectpath 1 setgray fill grestore
	            6 origfile _getfileinputtoken _imagecanvas
	        } if
	    } ifelse
	    6 origfile _getfileinputtoken _setcanvas
	    gsave clippath pathbbox _rectpath 1 setgray fill grestore
	    initgraphics
	} executeonly store

	6 origfile _getfileinputtoken _setcanvas

	gsave clippath pathbbox _rectpath 1 setgray fill grestore
	initgraphics

	9 origfile _getfileinputtoken _nullstring ne {
	    /file_to_read 9 origfile _getfileinputtoken (r) file def
	    {
                {
                    file_to_read cvx exec
                } stopped {

	% Ok, we got an error.. send back error tag with page number if we 
	% haven't yet, and update the fileinputtoken

		    12 origfile _getfileinputtoken 1 ne {
			1 12 origfile _setfileinputtoken
                        ERROR_PAGE_TAG _tagprint
			4 origfile _getfileinputtoken 
                        _typedprint _flush
		    } if
		    clear
                } {
 
	% if the number of pages is zero... then there probably isn't a showpage
	% in this file, so assume one page..
 
                    4 origfile _getfileinputtoken 0 eq {
                        1 4 origfile _setfileinputtoken
		        7 origfile _getfileinputtoken _setcanvas
	                gsave clippath pathbbox _rectpath 
			1 setgray fill grestore
	                6 origfile _getfileinputtoken _imagecanvas
                    } if
		    exit
                } ifelse
	    } loop
	} if

%%
%% ps_render_small_pages - Render small pixmaps for page overview
%%			   pop up.
%%

cdef ps_render_small_pages ()
	1 origfile _getfileinputtoken restore

% set fileinputtoken 12 just in case they are running this remotely.

	0 12 origfile _setfileinputtoken

	/showpage {
	    4 origfile _getfileinputtoken 1 add dup dup
	    4 origfile _setfileinputtoken
	    2 origfile _getfileinputtoken ne {
	       13 add origfile _getfileinputtoken _setcanvas
	       gsave clippath pathbbox _rectpath 1 setgray fill grestore
	       5 origfile _getfileinputtoken _imagecanvas
	       5 origfile _getfileinputtoken _setcanvas
	       gsave clippath pathbbox _rectpath 1 setgray fill grestore
	       initgraphics 
	    } {
	       pop
	    } ifelse
	} executeonly store

	0 4 origfile _setfileinputtoken
	5 origfile _getfileinputtoken _setcanvas
	gsave clippath pathbbox _rectpath 1 setgray fill grestore
	initgraphics

	9 origfile _getfileinputtoken _nullstring ne {
	    /file_to_read 9 origfile _getfileinputtoken (r) file store
	    {
                {
                    file_to_read cvx exec
                } stopped {

	% if we get an error, ignore it here since we reported it already.

		    clear
	        } {
		    exit
		} ifelse 
	    } loop
	} if

%%
%% ps_setup_render_page - Setup environment to render individual pages.
%%

cdef ps_setup_render_page ()
	1 origfile _getfileinputtoken restore

	/showpage {
	    4 origfile _getfileinputtoken 1 add dup dup dup
	    4 origfile _setfileinputtoken
	    3 origfile _getfileinputtoken eq {
		7 origfile _getfileinputtoken _setcanvas
	        gsave clippath pathbbox _rectpath 1 setgray fill grestore
	        6 origfile _getfileinputtoken _imagecanvas
		3 origfile _getfileinputtoken 
		8 origfile _setfileinputtoken
	    } if 

	    % if we've rendered two pages, or we only had one
	    %  to render (only one page in the file), then quit

	    3 origfile _getfileinputtoken 1 add ge 
	    2 origfile _getfileinputtoken 1 eq or {

	    % if we have the filename, then stop rendering pages.

		9 origfile _getfileinputtoken _nullstring ne {
		    1 11 origfile _setfileinputtoken 
		    clear stop
		} if
	    } if
	    2 origfile _getfileinputtoken ne {
	        6 origfile _getfileinputtoken _setcanvas
	        gsave clippath pathbbox _rectpath 1 setgray fill grestore
	        initgraphics 
	    } if
	} executeonly store

	save 1 origfile _setfileinputtoken

%%
%% ps_render_page - Render specific page
%%

cdef ps_render_page (int pageno)
	1 origfile _getfileinputtoken restore

	pageno 3 origfile _setfileinputtoken
	0 4 origfile _setfileinputtoken
	0 8 origfile _setfileinputtoken
	0 11 origfile _setfileinputtoken
	0 12 origfile _setfileinputtoken

	6 origfile _getfileinputtoken _setcanvas
	initgraphics
	gsave clippath pathbbox _rectpath 1 setgray fill grestore

	9 origfile _getfileinputtoken _nullstring ne {
	    /file_to_read 9 origfile _getfileinputtoken (r) file store
	    {
                {
                    file_to_read cvx exec
                } stopped {

	% if we got an error, still continue, since we'll eventually
 	% stop when we are done. Send back the error to the client.

                    11 origfile _getfileinputtoken 1 eq {
			exit
		    } {
		        12 origfile _getfileinputtoken 
		        4 origfile _getfileinputtoken 1 add ne {
			    4 origfile _getfileinputtoken 1 add 
			    12 origfile _setfileinputtoken
                            ERROR_PAGE_TAG _tagprint
			    4 origfile _getfileinputtoken 
                            _typedprint _flush
		        } if
		        clear
                    } ifelse
		} { 
		    exit
                } ifelse
	    } loop
	} if


%%
%% ps_switch_pixmaps - Switch the pixmaps stored in fileinputtokens 6 and 7.
%%		       We always render onto the pixmap in token #6, and
%%		       copy onto the pixmap in token #7, and if we're 
%%		       displaying the second pixmap now, then we'll want
%%		       to switch which pixmap we render onto. 
%%

cdef ps_switch_pixmaps ()
	6 origfile _getfileinputtoken
	7 origfile _getfileinputtoken exch
	7 origfile _setfileinputtoken
	6 origfile _setfileinputtoken

%%
%% ps_new_pixmaps - Set fileinputtokens to new pixmaps that were create
%%		    probably as a result of some operation like rotate or
%%		    zoom.
%%

cdef ps_new_pixmaps (int xid1, int xid2)
	/X11 3 0 _findpackage _beginpackage
	    xid1 xlookupid not { _framebuffer } if
	    xid2 xlookupid not { _framebuffer } if
	endpackage
        6 origfile _setfileinputtoken
	7 origfile _setfileinputtoken

