/* wpeek010.h  | LPeek header for wpeek011.cpp and wpeek012.cpp  [261114]
    */

#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <filesystem>
#include <thread>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include "xdg-shell-client-protocol.h"


extern wl_display *dis;
extern wl_compositor *comp;
extern wl_shm *shm;
extern xdg_wm_base *xdgwmb;
extern wl_seat *seat;
extern wl_surface *d_surf;
extern xdg_surface *xdgsurf;
extern wl_cursor_theme *crsr_theme;
extern wl_cursor *crsr;
extern FT_Library lbrry;
extern FT_Face face;
extern unsigned *p;          
extern unsigned clrs[36];
extern int monW, monH, bufW, bufH;
extern int txtW, txtH, thick;
extern int ascent, darkmode;
extern int share, tmr;
extern int kydelay, kyrate;
extern std::atomic <int> mode, wheel;
extern std::atomic <int> mX, mY;
extern unsigned short symbol[256];
extern unsigned char gct[256];
extern bool select_own, lftctl, clpbrd;
extern std::atomic <bool> podup, popfocus;
extern std::string strng, clp, hnd;
extern std::atomic <unsigned> ky, kyst;
extern std::atomic <unsigned> btn, btnst;

struct rect {
    int x, y, width, height;
    rect() : x(0), y(0), width(0), height(0) {}
    rect(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
};

void maind();

void d_close();

void dialog_return();

void set_cursor();

void d_kbd_key();

void d_pntr_motion();

void d_pntr_button();

void d_pntr_wheel();

void receive_clpbrd();

void send_to_clpbrd();

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

void draw_rectangle(unsigned* m, int bfW, int X, int Y,
        int width, int height, unsigned fg);

