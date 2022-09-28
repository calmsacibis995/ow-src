#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)ds_server_init.c	3.2 01/22/93";
#endif
#endif

#include <stdio.h>
#include <sys/param.h>
#include <locale.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <xview/cms.h>
#include <xview/defaults.h>
#include <ds_colors.h>

#define WORKSPACE_CLASS_RES    "OpenWindows.WorkspaceColor"
#define WORKSPACE_RES          "openwindows.workspacecolor"
#define WINDOW_CLASS_RES       "OpenWindows.WindowColor"
#define WINDOW_RES             "openwindows.windowcolor"
#define BACK_CLASS_RES         "Window.Color.Background"
#define BACK_RES               "window.color.background"
#define FORE_CLASS_RES         "Window.Color.Foreground"
#define FORE_RES               "window.color.foreground"

#define NUM_CANNED_COLORS  72     /* Number of canned colors if file does not exist */
#define MSGFILE    "SUNW_DESKSET_DS_SERVER_INIT"
#define MGET(s)     ( char * )gettext(s)

static Display      *dpy;
static Colormap      cmap;
static int           ncolors = 0;               
static int           alloc_now = False;      /* Flag whether to allocate colors now */

static char *color_file = "$OPENWINHOME/share/xnews/client/ds_server_init/ds_colors.txt";
static char  prop_data[4096];

static char *visual_class[] = {
    "StaticGray",
    "GrayScale",
    "StaticColor",
    "PseudoColor",
    "TrueColor",
    "DirectColor"
};

/*
 * Back-up Set of Colors in case disk file not found.
 */
static Xv_singlecolor canned[] = {
    {   0,   0,   0 },
    {   0,   0,   0 },
    {  76,  76,  76 },
    { 102, 102, 102 },
    { 153, 153, 153 },
    { 178, 178, 178 },
    { 204, 204, 204 },
    { 255, 255, 255 },
    { 191, 152, 152 },
    { 191, 114, 114 },
    { 191,  76,  76 },
    { 191,  38,  38 },
    { 229, 183, 183 },
    { 229, 137, 137 },
    { 229,  91,  91 },
    { 229,  45,  45 },
    { 191, 180, 152 },
    { 191, 170, 114 },
    { 191, 158,  76 },
    { 191, 148,  38 },
    { 229, 216, 183 },
    { 229, 203, 137 },
    { 229, 190,  91 },
    { 229, 177,  45 },
    { 189, 191, 152 },
    { 187, 191, 114 },
    { 185, 191,  76 },
    { 183, 191,  38 },
    { 227, 229, 183 },
    { 224, 229, 137 },
    { 222, 229,  91 },
    { 220, 229,  45 },
    { 152, 191, 162 },
    { 114, 191, 134 },
    {  76, 191, 105 },
    { 38,  191,  76 },
    { 183, 229, 194 },
    { 137, 229, 160 },
    {  91, 229, 126 },
    {  45, 229,  91 },
    { 152, 191, 191 },
    { 114, 191, 191 },
    {  76, 191, 191 },
    {  38, 191, 191 },
    { 183, 229, 229 },
    { 137, 229, 229 },
    {  91, 229, 229 },
    {  45, 229, 229 },
    { 152, 162, 191 },
    { 114, 134, 191 },
    {  76, 105, 191 },
    {  38,  76, 191 },
    { 183, 194, 229 },   
    { 137, 160, 229 },
    {  91, 126, 229 },
    {  45,  91, 229 },
    { 178, 152, 191 },
    { 165, 114, 191 },
    { 152,  76, 191 },
    { 140,  38, 191 },
    { 213, 183, 229 },
    { 198, 137, 229 },
    { 183,  91, 229 },
    { 167,  45, 229 },
    { 191, 152, 182 },
    { 191, 114, 172 },
    { 191,  76, 162 },
    { 191,  38, 152 },
    { 229, 183, 218 },
    { 229, 137, 206 },
    { 229,  91, 194 },
    { 229,  45, 183 },
};


/*
 * string_to_xcolor()
 *   string may be:
 *     color name:   "Blue", "Pink", etc.
 *     hex color:   #a0bf1e
 *     sunview decimal color:   100 220 255
 *
 * if it is not one of these formats (or if the name cannot be found),
 * then string_to_xcolor() returns 1, otherwise it returns 0.
 */
static int
string_to_xcolor(s, xcolor)
    char       *s;
    XColor     *xcolor;
{
    if (!XParseColor(dpy, cmap, s, xcolor)) {
	int         red,
	            green,
	            blue;
	if (sscanf(s, "%d %d %d", &red, &green, &blue) == 3) {
	    xcolor->red = red << 8;
	    xcolor->green = green << 8;
	    xcolor->blue = blue << 8;
	    xcolor->flags = DoRed | DoGreen | DoBlue;
            return ( 1 );
	}
        else
          return( 0 );
    }
    else
      return( 1 );
}


static void
init_xrdb()
{
  char  user_defaults[256];
 
  sprintf( user_defaults, "%s/.Xdefaults", getenv( "HOME" ) );
  /*
   * Initilize the resource manager.
   * Load the resource database residing in the server.
   * Load the user's own resource database.
   */
  defaults_init_db();
/*  defaults_load_db( NULL ); */
  defaults_load_db( user_defaults );

}


static void
read_colors( screen )
     int  screen;
{
    FILE         *fp;
    int           depth;
    int           class = 5;
    int           i;
    int           use_file = True;
    int	          max_allocated = False;
    int           max_colors;
    Visual       *visual;
    XVisualInfo   visual_info;
    char          hex_color[24];
    char          filename[MAXPATHLEN];
    XColor        xcolor;

    cmap = DefaultColormap( dpy, screen );

    if ( alloc_now ) {
      /*
       * Try to allocate colors for PseudoColor, TrueColor, 
       * DirectColor, and StaticColor; use black and white
       * for StaticGray and GrayScale.
       */
      depth = DefaultDepth( dpy, screen );
      if ( depth < 8 ) {
#ifdef DEBUG
          fprintf( stderr, MGET( "Colors: No color visual available at default depth.\n") );
#endif
          exit( 0 );
      }

      while ( !XMatchVisualInfo( dpy, screen, depth, class--, &visual_info ) );
 
#ifdef DEBUG
      printf( MGET("Colors: Found a %s class visual at default depth.\n" ), visual_class[++class] );
#endif
      if ( class < StaticColor ) { /* Color visual classes are 2 to 5 */
        fprintf( stderr, MGET("Colors: No color visual available at default depth\n" ) );
        return;
      }
    
      /*
       * The visual we found at the default depth
       * is not necessarily the default
       * visual, and therefore it is not necessarily the one
       * we used to create our window; however, we now know 
       * for sure that color is supported, so the following
       * code will work (or fail in a controlled way).
       * Let's check just out of curiosity:
       */
      visual = DefaultVisual( dpy, screen );
      if ( visual_info.visual != visual ) {
        printf( MGET("Colors: %s class visual at default depth"), visual_class[class] );
        printf( MGET(" is not the default visual! continuing anyway...\n") );
      }

    } /* end if alloc_now */

    /*
     * Open the file with the specified colors.
     * If the file if not found, use the hard-coded set of
     * colors we have.
     */
    ds_expand_pathname( color_file, filename );
    fp = fopen( filename, "r" );
    if ( !fp ) {
      fprintf( stderr, MGET( "Could not find color file %s - continuing with own colors\n" ), color_file );
      use_file = False;
    }

    /*
     * For each color in the file:
     * Parse the hex value into an RGB.
     * Allocate the color read-only from the DefaultColormap.
     */
    sprintf( prop_data, "%d ", ( int )cmap );

    if ( use_file ) 
      max_colors = MAX_DS_COLORS;
    else
      max_colors = NUM_CANNED_COLORS;

    for ( i = 0; i < max_colors; i++ ) {

      if ( use_file ) {
        hex_color[0] = NULL;
        fscanf( fp, "%s\n", hex_color );
        if ( hex_color[0] == NULL ) break;
      }
      else
        sprintf( hex_color, "#%02x%02x%02x", canned[i].red, canned[i].green, canned[i].blue );

      /*
       * Allocate colors only if command line specified to do so.
       */
      if ( alloc_now ) {
        if ( !XParseColor( dpy, cmap, hex_color, &xcolor ) ) {
          fprintf( stderr, MGET( "ds_server_init: Could not look up color `%s` in color database.\n" ), hex_color );
          max_allocated = True;
          continue;
        }
        if ( !XAllocColor( dpy, cmap, &xcolor ) ) {
          fprintf( stderr, MGET( "ds_server_init: Cannot allocate any more colors after %d colors.\n" ), i );
          fprintf( stderr, MGET( "All colorcells allocated and no matching cell found for %s\n" ), hex_color );
	  max_allocated = True;
          break;
        }
#ifdef DEBUG
      printf( MGET( "Allocated RGB values %d %d %d at pixel value %d\n" ),
               xcolor.red, xcolor.green, xcolor.blue, xcolor.pixel );
#endif
      }

      /*
       * Save data in character stream to place
       * as a property in the server later.
       */
      strcat( prop_data, hex_color );
      strcat( prop_data, " " );
      ncolors++;

    } /* end for */

    /*
     * This is a check to see if any more colors are
     * in the file.
     * This is just a warning message that not all colors 
     * in the color file were allocated.
     */
    if ( use_file && !max_allocated ) {
      hex_color[0] = NULL;
      fscanf( fp, "%s\n", hex_color );
      if ( hex_color[0] != NULL ) 
        fprintf( stderr, MGET( "Maximum DeskSet colors allocated after %d colors.\n" ), ncolors );
    }
    
    if ( use_file )
      ( void )fclose( fp );

}


static int
process_resource_color( color )
     char  *color;
{
  int           status;
  char          hex_color[24];
  XColor        xcolor;

    /*
     * Convert the string into an XColor.
     * Convert to hex format for storing in the property.
     * Allocate color if command-line options specifies to do so.
     * Add color to property list.
     */
    status = string_to_xcolor( color, &xcolor );
    if ( status && alloc_now )
      if ( XAllocColor( dpy, cmap, &xcolor ) ) {
#ifdef DEBUG
      printf( MGET( "Allocated RGB values %d %d %d at pixel value %d\n" ),
               xcolor.red, xcolor.green, xcolor.blue, xcolor.pixel );
#endif
        return( 0 );
      }

    sprintf( hex_color, "#%04x%04x%04x", xcolor.red, xcolor.green, xcolor.blue );
    strcat( prop_data, hex_color );
    strcat( prop_data, " " );
    ncolors++;
    return( 1 );
}

static void
read_resources()
{
  char  *s;

  s = defaults_get_string( WORKSPACE_RES, WORKSPACE_CLASS_RES, NULL );
  if ( s )
    process_resource_color( s );
  s = defaults_get_string( WINDOW_RES, WINDOW_CLASS_RES, NULL );
  if ( s )
    process_resource_color( s );
  s = defaults_get_string( BACK_RES, BACK_CLASS_RES, "White" );
  if ( s )
    process_resource_color( s );
  s = defaults_get_string( FORE_RES, FORE_CLASS_RES, "Black" );
  if ( s )
    process_resource_color( s );

}

static void
set_color_property( screen )
     int  screen;
{
  Atom  atom;

  atom = XInternAtom( dpy, COLOR_PROP_NAME, False );

  XChangeProperty( dpy, RootWindow( dpy, screen ), atom, XA_STRING, sizeof( char ) * 8, 
                   PropModeReplace, ( unsigned char * )prop_data, 
                   strlen( prop_data ) );
}


static void
parse_options( argc, argv )
    int   argc;
    char *argv[];
{
    int i;

    for ( i = 1; i < argc; ++i ) {
      if ( ( strcmp( argv[i], "-a" ) == 0 ) || ( strcmp( argv[i], "-alloc" ) == 0 ) )
        alloc_now = True;
      else if ( ( strcmp( argv[i], "-f" ) == 0 ) || ( strcmp( argv[i], "-file" ) == 0 ) )
        color_file = argv[++i];
     
    }
}


main( argc, argv )
    int         argc;
    char       *argv[];
{
    int  total_screens;
    int  screen;
    char bind_home[MAXPATHLEN];

  /*
   * Parse command line options.
   */
  parse_options( argc, argv );

  /*
   * Connect to the server.
   */
  dpy = XOpenDisplay( NULL );
  if ( dpy == ( Display * )NULL ) {
    fprintf( stderr, MGET( "ds_server_init: Couldn't connect to display\n" ) );
    exit( 0 );
  }
  
  setlocale(LC_ALL, "");
  ds_expand_pathname( "$OPENWINHOME/lib/locale", bind_home );
  bindtextdomain( MSGFILE, bind_home );
  textdomain( MSGFILE );

  /*
   * Read in Resource Database values.
   */
  init_xrdb();

  total_screens = ScreenCount( dpy );

  for ( screen = 0; screen < total_screens; screen++ ) {

    ncolors = 0;

    /*
     * Read the hex colors from the file or
     * use the canned set of colors and store them in
     * a character stream separated by spaces.
     */
    read_colors( screen );
    /*
     * Add any default colors set in the server
     * since these colors are already allocated
     * anyway.
     */
    read_resources();

    /*
     * Set this property in the server so other
     * applications can read these colors.
     */
    set_color_property( screen );

  }

  /*
   * Disconnect from the server.
   */
  XSync( dpy, 0 );
  XCloseDisplay( dpy );

}
