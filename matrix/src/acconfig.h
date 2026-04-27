/* Define this if your curses library has use_default_colors, for 
   cool transparency =-) */
#undef HAVE_USE_DEFAULT_COLORS

/* Define this if you have the linux consolechars program */
#undef HAVE_CONSOLECHARS

/* Define this if you have the linux setfont program */
#undef HAVE_SETFONT

/* Define this if you have the wresize function in your ncurses-type library */
#undef HAVE_WRESIZE

/* Define this if you have the resizeterm function in your ncurses-type library */
#undef HAVE_RESIZETERM

#define LINES 30 //25 //30  //this really shouldn't be hard coded...
#define COLS 80 //40 //80      //as above. very oldskool approach.

/* colors */
#define COLOR_BLACK  0
#define COLOR_RED 1
#define COLOR_GREEN  2
#define COLOR_YELLOW 3
#define COLOR_BLUE   4
#define COLOR_MAGENTA   5
#define COLOR_CYAN   6
#define COLOR_WHITE  7
#define COLOR_BG 128

// randnum = 93;
// randmin = 33;
// randnum = 127;
// randmin = 128;
// highnum = 123;
// highnum = 250;
#define randmin 93
#define randnum 33
#define highnum 123
