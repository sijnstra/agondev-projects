/**************************************************************************
 *   amatrix.c                                                            *
 *   Agon-ized by Shawn Sijnstra (c) 2024-25                              *
 *   Heavily adjusted from the original                                   *
 *   Changes to keyboard input, font and screen handling                  *
 *                                                                        *
 *   Original versoin copyright (C) 1999 Chris Allegretta                 *
 *   This program is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by *
 *   the Free Software Foundation; either version 1, or (at your option)  *
 *   any later version.                                                   *
 *                                                                        *
 *   This program is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *   GNU General Public License for more details.                         *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program; if not, write to the Free Software          *
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            *
 *                                                                        *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>


#include <agon/vdp.h> //added
#include <agon/keyboard.h> //added

#include "getopt.h"	//added

#include "acconfig.h"

#include "font8x8_misaki_x.h"

/* Matrix typedef */
typedef struct cmatrix {
	short val;
    char bold;
} cmatrix;

/* Global variables, unfortunately */
cmatrix **matrix = (cmatrix **) NULL;   /* The matrix has you */
char *length = NULL;			/* Length of cols in each line */
char *spaces = NULL;			/* spaces left to fill */
char *updates = NULL;			/* What does this do again? :) */

static char _current_key = 0;


/* What we do when we're all set to exit */
void finish(void)
{
    vdp_cursor_enable( true );
    vdp_clear_screen();
	vdp_reset_system_font();
	vdp_swap();
	vdp_mode(3);	//single buffer, 64 colours
	kbuf_deinit();
	vdp_set_text_colour( COLOR_WHITE + 8 );
	vdp_set_text_colour( COLOR_BLACK + COLOR_BG);
	vdp_cursor_enable( true );
    exit(0);
}

void init_colours(int colour)
{
	int	r,g,b;

	vdp_define_colour(0,0xff,0,0,0); //black
	r = colour & 1;
	g = (colour & 2) >> 1;
	b = (colour & 4) >> 2;
	vdp_define_colour(1,0xff,r * 0x55,g * 0x55,b * 0x55);
	vdp_define_colour(2,0xff,r * 0xAA,g * 0xAA,b * 0xAA);
	vdp_define_colour(3,0xff,r * 0xff,g * 0xff,b * 0xff);
	vdp_define_colour(4,0xff,0xff,0xff,0xff);
}
/* What we do when we're all set to exit */
void c_die(char *msg, ...)
{
    va_list ap;

    vdp_cursor_enable( true );
    vdp_clear_screen();
	vdp_reset_system_font();
	vdp_swap();
	vdp_mode(3);	//single buffer, 64 colours
	kbuf_deinit();
	vdp_set_text_colour( COLOR_WHITE + 8 );
	vdp_set_text_colour( COLOR_BLACK + COLOR_BG);
	vdp_cursor_enable( true );

    va_start(ap, msg);
    vfprintf(stderr, "%s", ap);
    va_end(ap);
    exit(0);
}

void usage(char * argv[])
{
    printf(" Usage: %s -[ahosv] [-C color]\n",argv[0]);	//NOTE: this is static at compile time!
    printf(" -a: Asynchronous scroll\n");
    printf(" -h: Print usage and exit\n");
    printf(" -o: Use old-style scrolling\n");
    printf(" -s: \"Screensaver\" mode, exits on first keystroke\n");
    printf(" -v: Print version information and exit\n");
    printf(" -c [color]: Use this color for matrix (default green)\n");
}

void version(void)
{
	printf(" Agon Matrix by Shawn Sijnstra (c) 2024 based on\n\r");
    printf(" CMatrix by Chris Allegretta (c) 1999 - GPL\n");
}


/* nmalloc from nano by Big Gaute */
void *nmalloc(size_t howmuch)
{
    void *r;

    /* Panic save? */

    if (!(r = malloc(howmuch)))
	c_die("CMatrix: malloc: out of memory!");

    return r;
}

/* Initialize the global variables */
void var_init(void)
{
    short i, j;

    if (matrix != NULL)
	free(matrix);

    matrix = nmalloc(sizeof(cmatrix) * (LINES + 1));
    for (i = 0; i <= LINES; i++)
	matrix[i] = nmalloc(sizeof(cmatrix) * COLS);

    if (length != NULL)
	free(length);
    length = nmalloc(COLS * sizeof(char));

    if (spaces != NULL)
	free(spaces);
    spaces = nmalloc(COLS* sizeof(char));

    if (updates != NULL)
	free(updates);
    updates = nmalloc(COLS * sizeof(char));

    /* Make the matrix */
    for (i = 0; i <= LINES; i++)
	for (j = 0; j <= COLS - 1; j += 2)
	    {matrix[i][j].val = -1; matrix[i][j].bold = 0;};

    for (j = 0; j <= COLS - 1; j += 2) {
	/* Set up spaces[] array of how many spaces to skip */
	spaces[j] = rand() % LINES + 1;

	/* And length of the stream */
	length[j] = rand() % (LINES - 3) + 3;

	/* Sentinel value for creation of new objects */
	matrix[1][j].val = ' ';

	/* And set updates[] array for update speed. */
	updates[j] = rand() % 3 + 1;
    }

}



int main(int argc, char *argv[])
{
    int i, j = 0, // bold = 1, //
	y, z, random = 0, mcolor = COLOR_GREEN;
	short tempval = 0;
	char count = 0 , asynch = 1, screensaver = 0, firstcoldone = 0, oldstyle = 0;	//asynch now inverted so 0 is asynch
	struct keyboard_event_t e;

    static int optchr, keypress;

    /* Many thanks to morph- (morph@jmss.com) for this getopt patch */
    opterr = 0;
    while ((optchr = getopt(argc, argv, "ahosvc:")) != EOF) {
	switch (optchr) {
	case 's':
	    screensaver = 1;
	    break;
	case 'a':
	    asynch = 0;
	    break;
	case 'c':
	    if (!strcmp(optarg, "green"))
		mcolor = COLOR_GREEN;
	    else if (!strcmp(optarg, "red"))
		mcolor = COLOR_RED;
	    else if (!strcmp(optarg, "blue"))
		mcolor = COLOR_BLUE;
	    else if (!strcmp(optarg, "white"))
		mcolor = COLOR_WHITE;
	    else if (!strcmp(optarg, "yellow"))
		mcolor = COLOR_YELLOW;
	    else if (!strcmp(optarg, "cyan"))
		mcolor = COLOR_CYAN;
	    else if (!strcmp(optarg, "magenta"))
		mcolor = COLOR_MAGENTA;
	    else if (!strcmp(optarg, "black"))
		mcolor = COLOR_BLACK;
	    else {
		printf(" Invalid color selection\n Valid "
		       "colors are green, red, blue, "
		       "white, yellow, cyan and magenta.\n");
		exit(1);
	    }
	    break;
	case 'h':
	case '?':
	    usage(argv);
	    exit(0);
	case 'o':
	    oldstyle = 1;
	    break;
	case 'v':
	    version();
	    exit(0);
	}
    }

    vdp_mode( 132 ); //132 is 80x30, 16 colour version with double buffering. Can cause issues. 132 is for 64x48
    vdp_clear_screen();
	vdp_get_scr_dims( true );
    vdp_logical_scr_dims( false );
	kbuf_init(16);
	vdp_cursor_enable( false );
	init_colours(mcolor);	//change the colour table for fast lookup/swap
    // Let's set up the font! font8x8_misaki[96][8]
    for (i=32;i<129;i++) {
//    for (i=128;i<256;i++) {
    	if (!( ((i == '&') && (oldstyle)) || ((i == '|') && (oldstyle))  )) vdp_redefine_character ( i,
    		font8x8_misaki[i-32][0] ,
    		font8x8_misaki[i-32][1] ,
    		font8x8_misaki[i-32][2] ,
    		font8x8_misaki[i-32][3] ,
    		font8x8_misaki[i-32][4] ,
    		font8x8_misaki[i-32][5] ,
    		font8x8_misaki[i-32][6] ,
    		font8x8_misaki[i-32][7] );
/*    		font8x8_misaki[i-128][0] ,
    		font8x8_misaki[i-128][1] ,
    		font8x8_misaki[i-128][2] ,
    		font8x8_misaki[i-128][3] ,
    		font8x8_misaki[i-128][4] ,
    		font8x8_misaki[i-128][5] ,
    		font8x8_misaki[i-128][6] ,
    		font8x8_misaki[i-128][7] ); This version was to use a different part of the character set*/
    }

	vdp_redefine_character ( 253, 0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x10, 0x00);
	vdp_redefine_character ( 254, 0x10, 0x28, 0x10, 0x28, 0x54, 0x28, 0x10, 0x00);


    srand(time(NULL));

    /* Set up values for random number generation */
//	randnum = 93;
//	randmin = 33;
//	randnum = 127;
//	randmin = 128;
//	highnum = 123;
//	highnum = 250;

    var_init();

    while (1) {
	count++;
	if (count > 4)
	    count = 1;

	if (kbuf_poll_event(&e)) {
	    if (screensaver)
		finish();
	    else
		switch (e.ascii) {
		case 'q':
		case 0x1B:	//escape
		case 0x03:	//ctrl-C
		    finish();
		    break;
		case 'a':
		    asynch = 1 - asynch;
		    break;
//		case '0':	//don't really want black on black.
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			init_colours (e.ascii - 48);
		    break;
		}
	}
	for (j = 0; j <= COLS - 1; j += 2) {
	    if (count > updates[j] || asynch) { 	//asynch is inverted now

		/* I dont like old-style scrolling, yuck */
		if (oldstyle) {
		    for (i = LINES - 1; i >= 1; i--)
			matrix[i][j].val = matrix[i - 1][j].val;

		    random = rand() % (randnum + 8) + randmin;
//		    random = (int) (rand() & randnum) + randmin;

		    if (matrix[1][j].val == 254)
				matrix[0][j].val = 253;
//		    else if (matrix[1][j].val == ' '
//			     || matrix[1][j].val == -1) {
		    else if (matrix[1][j].val < ' ' + 1) {
				if (spaces[j] > 0) {
				    matrix[0][j].val = ' ';
				    spaces[j]--;
				} else {

			    /* Random number to determine whether head of next column
			       of chars has a white 'head' on it. */

			    if ((rand() % 3) == 1)
					matrix[0][j].val = 254;	//0;
			    else
					matrix[0][j].val = (short) rand() % randnum + randmin;

			    spaces[j] = rand() % LINES + 1;
				}
		    } else if (random > highnum && matrix[1][j].val != 1)
					matrix[0][j].val = ' ';
		    	else
					matrix[0][j].val =
				    	(short) rand() % randnum + randmin;
//			    (int) (rand() & randnum) + randmin;

		} else {	/* New style scrolling (default) */

			if (matrix[0][j].val == -1 && matrix[1][j].val == ' ') {
				if (spaces[j] > 0) spaces[j]--;
				else {
				length[j] = rand() % (LINES - 3) + 3;
				matrix[0][j].val = rand() % randnum + randmin;

				matrix[0][j].bold = (rand() & 1);

				spaces[j] = rand() % LINES + 1;
			    }
			}

		    i = 0;
		    y = 0;
		    firstcoldone = 0;
		    while (i <= LINES) {

			/* Skip over spaces */
//			while (i <= LINES && (matrix[i][j].val == ' ' ||
//			       matrix[i][j].val == -1))
			while (i <= LINES && (matrix[i][j].val <= ' '))
			    i++;

			if (i > LINES)
			    break;

			/* Go to the head of this column */
			z = i;
			y = 0;
//			while (i <= LINES && (matrix[i][j].val != ' ' &&
			while (i <= LINES && (matrix[i][j].val > ' ')) {
//				matrix[i][j].val != -1)) {
			    i++;
			    y++;
			}

			if (i > LINES) {
			    matrix[z][j].val = ' ';
			    matrix[LINES][j].bold = 0;
			    continue;
			}

			matrix[i][j].val =
			    (int) rand() % randnum + randmin;

			if (matrix[i - 1][j].bold) {
			    matrix[i - 1][j].bold = 0;
			    matrix[i][j].bold = 1;
			}

			/* If we're at the top of the column and it's reached its
			 * full length (about to start moving down), we do this
			 * to get it moving.  This is also how we keep segments not
			 * already growing from growing accidentally =>
			 */
			if (y > length[j] || firstcoldone) {
			    matrix[z][j].val = ' ';
			    matrix[0][j].val = -1;
			}
			firstcoldone = 1;
			i++;
		    }
		}
		}
	    /* Hack =P */
	    y = 1 - oldstyle;
	    z = LINES - oldstyle;
//	    vdp_cursor_tab( j, 0);	//let's just set the cursor location once

	    for (i = y; i <= z; i++) {
//		move(i - y, j);
	    vdp_cursor_tab( j, i-y);

	    tempval = matrix[i][j].val;
		if (tempval == 254 || matrix[i][j].bold) {	//was 0
			vdp_set_text_colour(4); putch(tempval);
		} else {
			if (tempval>0)
			{
				vdp_set_text_colour(2 + (tempval % 2)); putch(tempval); // we've redefined 253 to be the broken pipe
			}

			else {
				vdp_set_text_colour(1);
		    }
		}
	    }
	}
		vdp_swap();
//	napms(update * 10); //don't need to slow it down....
//	napms(update);

    }

    finish();

}

