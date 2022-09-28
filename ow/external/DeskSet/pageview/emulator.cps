%!CPS-NeWS3.2
%%Title: emulator.cps
%%Description: LaserWriter server loop emulator for pageview.
%%SccsId: @(#)emulator.cps 3.4 93/04/02
%
%	PageView is a page oriented viewer for PostScript documents
%	that follow Adobe PostScript document structuring conventions
%
% NOTE: Pageview now uses 2 NeWS connections to the server. One connection
%	is used only for PostScript rendering and it's operator set is
%	restricted. A second connection is used to gain access to NeWS-specific
%	features. The cps macros below are commented as to which connection
%	they are used with.
%
%	Also, you may notice that some NeWS-specific operators are used
%	within procedures defined in ps_start_serverloop. These operators
%	get bound into the procedures as a result of autobinding so that
%	they don't have to be present on the dictionary stack when the
%	the procedures are executed.

#include "tags.h"
C: #include "tags.h"

%% fileinputtoken usage for PostScript rendering connection:
%% 0: transformation matrix for page
%% 1: save object for initial state.

% PostScript
cdef ps_start_page()
	gsave
	    1 setgray
	    clippath fill
	grestore
	/page_matrix 6 array currentmatrix def

% PostScript
cdef ps_no_page(string ps_line1, string ps_line2, string ps_line3,
		string locale)
	locale (C) ne {
	    /LucidaSans-Bold findfont
	    dup length dict begin
	    { 1 index /FID ne { def } { pop pop } ifelse } forall
	    /Encoding ISOLatin1Encoding def
	    currentdict end
	    /LucidaSans-Bold-ISOLatin1 exch definefont pop
	} if

	gsave
	    locale (C) ne {
	        /LucidaSans-Bold-ISOLatin1 findfont 
	    } {
	        /LucidaSans-Bold findfont 
	    } ifelse

	    18 scalefont setfont
	    clippath pathbbox 4 2 roll pop pop
	    exch /pagewidth exch 72 sub def
	    /middle pagewidth 72 add 2 div def
	    100 sub middle exch moveto
	    /line1 ps_line1 def
	    /line2 ps_line2 def
	    /line3 ps_line3 def
	    /space ( ) 0 get def
	    /newline1 () def
	    /newline2 () def
	    /newline3 () def
	    /done 0 def

	    /createline {
   	        /newtmp exch store
      	 	/tmp exch store
   		tmp stringwidth pop pagewidth gt {
       		    tmp length 1 sub -1 0 {
           		dup tmp exch get space eq {
               		    /newtmp tmp 0 4 -1 roll getinterval store
               		    newtmp stringwidth pop pagewidth le {
                   		/done 1 store
               		    } if
           		} {
               		    pop
           		} ifelse
           		done 1 eq {
               		    exit
           		} if
       		    } for
   		} {
       		    /newtmp tmp store
   		} ifelse
   		tmp newtmp done
	    } def
 
	    /printline {
    		dup stringwidth exch 2 div neg exch rmoveto show
    		currentpoint exch pop middle exch
    		24 sub moveto
	    } def
  
	    /addline {
    	        /tmp2 exch store
    		/newtmp exch store
    		/tmp exch store
    		tmp newtmp anchorsearch {
		% get rid of first space left over from first string
        	    pop dup length 1 sub 1 exch getinterval
        	    /tmp2 exch ( ) _append tmp2 _append store
    		} if
    		tmp2
	    } def
 
	    line1 newline1 createline
	    /done exch store
	    /newline1 exch store
	    /line1 exch store
	    newline1 printline
 
	    done 1 eq {
    		/done 0 store
		line1 newline1 line2 addline
    		/line2 exch store
	    } if
	     
	    line2 newline2 createline
	    /done exch store
	    /newline2 exch store
	    /line2 exch store
	    newline2 printline
	     
	    done 1 eq {
    		/done 0 store
    		line2 newline2 line3 addline
    		/line3 exch store
	    } if

	    line3 newline3 createline
	    /done exch store
	    /newline3 exch store
	    /line3 exch store
	    newline3 printline
	     
	    {
    		done 1 eq {
        	    /done 0 store
        	    /line4 () store
        	    /newline4 () store
        	    line3 newline3 anchorsearch {
	   	    % get rid of first space left over from first string
            		pop dup length 1 sub 1 exch getinterval
            		/line4 exch def
        	    } if
          	    line4 newline4 createline
        	    /done exch store
        	    /newline4 exch store
        	    /line4 exch store
        	    newline4 printline
        
        	    /newline3 newline4 store
        	    /line3 line4 store
		} {
        	    exit
    		} ifelse
	    } loop 
	grestore

cdef ps_no_page_i18n(string ps_line1, string ps_line2, string ps_line3, string locale)
        gsave
	     /CharToByteFromFirst { % char str => byte
		 0 0 3 -1 roll  % char bytecounter charcounter str
		     {   
			 3 index 2 index eq {
			     pop exit
			 } if
			 dup 16#80 le {
			     pop 1 add exch 1 add exch
			 }{
			     16#8f eq {
			 	 pop exch 1 add exch
			     }{
			        .5 add exch 1 add exch
		       	     } ifelse
		 	} ifelse
		    } forall % char bytecounter charcounter
		 pop exch pop
	     } def

	     /locale_getinterval { % string pos num-of-char => substring
		 2 index 4 2 roll exch
		 CharToByteFromFirst                % char str byte
		 1 index length 1 index sub getinterval  % char substrig
		 exch 1 index
		 CharToByteFromFirst
		 0 exch getinterval
	     } def

	     /locale_length {
		 0 exch % charcter-counter string
		 {   
		     dup 16#80 le {
			pop 1 add
		     }{
			16#8f eq {
			    pop
			}{
		            .5 add
			} ifelse
		     } ifelse
		 } forall  % character-counter
	         cvi 
	    } def

            OW (/lib/locale/) _append locale _append (/print/prolog.ps) _append
            (r) file cvx exec
            /LC_Helvetica-Bold findfont 18 scalefont setfont
 
	    clippath pathbbox 4 2 roll pop pop
	    exch /pagewidth exch 72 sub def
	    /middle pagewidth 72 add 2 div def
	    100 sub middle exch moveto
	    /line1 ps_line1 def
	    /line2 ps_line2 def
	    /line3 ps_line3 def
	    /newline1 () def
	    /newline2 () def
	    /newline3 () def
	    /done 0 def

	    /createline {
   	        /newtmp exch store
      	 	/tmp exch store
   		tmp stringwidth pop pagewidth gt {
       		    tmp locale_length 1 sub -1 0 {
               		/newtmp tmp 0 4 -1 roll locale_getinterval store
               		newtmp stringwidth pop pagewidth le {
                   	    /done 1 store
               		} if
           		done 1 eq {
               		    exit
           		} if
       		    } for
   		} {
       		    /newtmp tmp store
   		} ifelse
   		tmp newtmp done
	    } def
 
	    /printline {
    		dup stringwidth exch 2 div neg exch rmoveto show
    		currentpoint exch pop middle exch
    		24 sub moveto
	    } def
  
	    /addline {
    	        /tmp2 exch store
    		/newtmp exch store
    		/tmp exch store
    		tmp newtmp anchorsearch {
        	    pop /tmp2 exch tmp2 _append store
    		} if
    		tmp2
	    } def
 
	    line1 newline1 createline
	    /done exch store
	    /newline1 exch store
	    /line1 exch store
	    newline1 printline
 
	    done 1 eq {
    		/done 0 store
		line1 newline1 line2 addline
    		/line2 exch store
	    } if
	     
	    line2 newline2 createline
	    /done exch store
	    /newline2 exch store
	    /line2 exch store
	    newline2 printline
	     
	    {
    		done 1 eq {
        	    /done 0 store
        	    /line4 () store
        	    /newline4 () store
        	    line2 newline2 anchorsearch {
            		pop /line4 exch def
        	    } if
          	    line4 newline4 createline
        	    /done exch store
        	    /newline4 exch store
        	    /line4 exch store
        	    newline4 printline
        
        	    /newline2 newline4 store
        	    /line2 line4 store
		} {
        	    exit
    		} ifelse
	    } loop 

	    line3 printline
	     
	grestore

% PostScript
cdef ps_getoutput(string output) => OUTPUT_TAG (output)
cdef ps_geterror(string error) => ERROR_TAG (error)
cdef ps_getpageloc (int pageloc) => SHOWPAGE_TAG (pageloc)

% PostScript
cdef ps_start_serverloop(int xid, float a, float b, float c, float d, int tx, int ty, int method, string filename)
	/NeWS 3 0 findpackage beginpackage
	[ a b c d tx ty ] 0 setfileinputtoken
	/X11 3 0 findpackage beginpackage
	    xid xlookupid not { framebuffer } if
	endpackage
	setcanvas

	/original_ctm 6 array currentmatrix def
	/page_matrix [ a b c d tx ty ] def

	% Eliminate nasties from our userdict that get
	% inherited from the NeWS listener and libcps.
	currentdict /framebuffer undef
	currentdict /OriginatingHost undef

	% Create a new systemdict and customize
	% it for laserwriter compatibility.
	%
	systemdict maxlength dict
	systemdict exch copy dup begin

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

%	currentdict /endpackage undef


	% This is in the OW V3 systemdict as a bug
	% workaround for answerbook which statically
	% linked a version of libxvps with a bug that
	% depends upon currentprocess being in systemdict.
	currentdict /currentprocess undef

	% Ack! This got accidentally left in systemdict in
	% V3 FCS. It will be gone in future releases but
	% we will have to guard against it for the rest of
	% eternity in case we are talking to a V3 server.
	currentdict /rootcanvases undef

	% Yet more goop that shouldn't really be in systemdict.
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
	statusdict /_file known {
	    statusdict /_file get /file exch def
	} if


	% Promote (for ps_sync() above)
	/_tagprint /tagprint load def
	/_setfileinputtoken /setfileinputtoken load def
	/_rectpath /rectpath load def
	/_append /append load def
	/OW OPENWINHOME def

	/serverdict 10 dict def
	serverdict begin
	    /product (NeWS 3.0) def
	    /exitserver { % int => -
		pop
		1 getfileinputtoken restore
		count { pop } repeat
		countdictstack 2 sub { end } repeat
	    } executeonly def
	end
	statusdict begin
	    300 setjobtimeout
	end

	end % new systemdict
	currentprocess /ErrorDetailLevel 1 put

%
% The following NeWS goop (isarray and sprintf) are currently
% used by the PostScript connection. I have imported their
% definition here for now.
%

% Return true if an object is an array.
/_isarray? { % any => boolean
    type dup /arraytype eq  exch /packedarraytype eq or
} ?def
 
%
% Sprintf: replace %'s in format string with argument as a string.
% Note the args can either be before or after the format string.
% If they precede the format string, they are in reverse order:
% argN .. arg0 formatstring   -or-  formatstring [arg0 .. argN]
%
/_sprintf { % args fmtstr -or- fmttstr [args] => str
    dup _isarray? {                      % convert second form to first form
        3 exch {                        % fmtstr i+3 ai
            1 index 1 roll              % ai .. a1 fmtstr i+3
            1 add
        } forall
        pop 
    } if                                % aN .. ai fmtstr
    () 250 string 3 -1 roll (%) {       % aN .. ai resultstr tempstr fmtstr (%)
        search {                        % aN .. ai result temp post (%) pre
            6 -1 roll                   % aN .. result temp post (%) pre ai
            dup type /stringtype ne     % is ai a string?  If so, convert.
                {4 index cvs} if        % aN .. result temp post (%) pre (ai)
            append                      % aN .. result temp post (%) pre'
            5 -1 roll exch append       % aN .. temp post (%) result'
            4 1 roll                    % aN .. result' temp post (%)
        } {                             % result temp fmt
            exch pop append exit
        } ifelse
    } loop  
} executeonly def    

	/setrealdevice {
	    % work around - this proc required to parse Laser Prep
	    % (AppleDict md) Procset
	} def
	/execjob {
	    % work around - this proc required to parse Laser Prep
	    % (AppleDict md) Procset
	} def
	/#copies 1 def
	/_print { % string => -
	    OUTPUT_TAG tagprint
	    dup length MAXOUTPUT gt { 0 MAXOUTPUT getinterval } if
	    typedprint flush
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
	/lettertray {} def
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
        	    (% font not found, using Courier\n) _sprintf _print
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
%	/initmatrix {
%	    0 getfileinputtoken setmatrix
%	} executeonly def
%	/initgraphics { % - => -
%	    //initgraphics
%	    0 getfileinputtoken setmatrix
%	} executeonly def

	method SHOWPAGE_METHOD eq {
	    /showpage {
		SHOWPAGE_TAG tagprint
		currentfile fileposition typedprint flush
	    } executeonly def
	    /stroke { } def
	    /fill { } def
	    //initgraphics
	    0 getfileinputtoken setmatrix
	} {
	    /initmatrix {
	        0 getfileinputtoken setmatrix
	    } executeonly def
	    /initgraphics { % - => -
	        //initgraphics
	        0 getfileinputtoken setmatrix
	    } executeonly def
	    /defaultmatrix { % mtx => mtx'
	        0 getfileinputtoken exch copy
	    } executeonly def
	    /showpage { % - => -
	        initgraphics
	        0 getfileinputtoken setmatrix
	    } executeonly def
	    showpage
	} ifelse

 
%	save 1 setfileinputtoken
	{ 
	    {
		current_file cvx exec
	    } stopped {
		$error /message get (Error:) search pop pop
		(\n) exch append exch append
		(At: Reading file\(?,W,R\)) search { 
		    exch pop exch pop 
		} if
		ERROR_TAG tagprint
		dup length MAXOUTPUT gt { 
		    0 MAXOUTPUT getinterval 
		} if
		typedprint flush
	    } executeonly if

	    % either the C side went away, we just sent ERROR_TAG.
	    % or we just finished reading a file (if we're using the
	    % SHOWPAGE_METHOD). In either event, restore state and eat
	    % all input forever. If SHOWPAGE_METHOD being used, send
	    % back DONE_TAG and quit.

	    1 getfileinputtoken restore
	    method SHOWPAGE_METHOD eq {
		DONE_TAG _tagprint flush
		quit
	    } if

	    {
		current_file 1024 string readstring {
		    pop
		} {
		    quit
		} ifelse
	    } loop
	}
	false setautobind

	% install new systemdict
	exch					% proc nsys
	currentprocess /DictionaryStack get dup	% proc nsys dstack dstack
	0 4 -1 roll put				% proc dstack
	currentprocess /DictionaryStack 3 -1 roll put
	
	endpackage % NeWS 

	% determine file we are processing...
	method SHOWPAGE_METHOD eq {
	    filename (r) file
	} {
	    currentfile
	} ifelse
	/current_file exch def

	% save state
	save 1 _setfileinputtoken

	% execute server loop
	executeonly exec

% Both PostScript and NeWS
cdef ps_synch()
	DONE_TAG _tagprint flush

% NeWS
cdef ps_initconnection()
	/NeWS 3 0 findpackage beginpackage
	/X11 3 0 findpackage beginpackage
	/_tagprint /tagprint load def	% both connections can use ps_synch()


cdef ps_fix (int scr, int xclip, int yclip, int width_clip, int height_clip)
%	/tmp_matrix 6 array currentmatrix def
	original_ctm setmatrix
	xclip yclip width_clip height_clip _rectpath clip
	page_matrix setmatrix

% NeWS
cdef ps_AAfix (int scr, int offscr, int xo, int yo, int vh,
	       int xclip, int yclip, int width_clip, int height_clip)
	offscr xlookupid not { framebuffer } if
	scr xlookupid not { framebuffer } if
	dup setcanvas
	xclip yclip width_clip height_clip rectpath clip
	128 setpixel clippath fill
	xo vh yo add translate
	/AntiAliasedScale get 1 exch div dup neg scale
	imagecanvas

% NeWS
cdef ps_AAimagecanvas(int scr, int offscr, int xo, int yo, int vh)
	offscr xlookupid not { framebuffer } if
	scr xlookupid not { framebuffer } if
	dup setcanvas
	xo vh yo add translate
	/AntiAliasedScale get 1 exch div dup neg scale
	imagecanvas

%NeWS
cdef ps_AAclearedge(int scr, int xclip, int yclip,
			int width_clip, int height_clip)
	scr xlookupid not { framebuffer } if
	setcanvas
	xclip yclip width_clip height_clip rectpath clip
	128 setpixel clippath fill

% NeWS
cdef ps_AAon(int scr)
	scr xlookupid
		{ { /AntiAliased 1 put } stopped
		  { pop true put } if
		} if

% NeWS
cdef ps_AAoff(int scr)
	scr xlookupid
		{ { /AntiAliased 0 put } stopped
		  { pop false put } if
		} if

% NeWS
cdef ps_AAscale(int scr, int AAscale) => KNOWN_TAG (AAscale)
	scr xlookupid
		{ dup /AntiAliased known
			{ /AntiAliasedScale get } { 0 } ifelse }
		{ 0 }
	ifelse
	KNOWN_TAG tagprint typedprint
