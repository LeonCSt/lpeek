/* lpeek010.h  | header file for lpeek011.cpp and lpeek012.cpp  [260918] */


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <filesystem>
#include <thread>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <atomic>


extern Display *dis;
extern GC gc;
extern Window win, sel_owner_win;
extern Atom WM_DELETE_WINDOW, utf8str;
extern Atom clpbrd, prop, trgts;
extern XShmSegmentInfo shminfo;
extern FT_Library lbrry;
extern FT_Face face;
extern unsigned *p;          
extern unsigned clrs[36];
extern int waW, waH;
extern int bufW, bufH;
extern int txtW, txtH, thick;
extern int ascent, darkmode;
extern int share, tmr;
extern std::atomic <int> mode;
extern unsigned short symbol[256];
extern unsigned char gct[256];
extern bool select_own;
extern std::string strng, clp, hnd;


void maind();

void refresh();

void draw_glyph(unsigned *m, int bfW, int X, int Y,
        unsigned short symbl, unsigned fg);
        /*  unsigned *m          | pointer to beginning of buffer pixels
            int bfW              | buffer width
            int X, Y             | start position
            unsigned short symbl | unicode character
            unsigned fg          | foreground color   */

void fill_rectangle(unsigned* m, int bfW, int X, int Y,
        int width, int height, unsigned fg);


