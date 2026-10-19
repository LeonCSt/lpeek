/*  lpeek011.cpp:  A file viewer
    g++ -I/usr/include/freetype2 lpeek011.cpp lpeek012.cpp -o lpeek -lX11 -lXext -lfreetype
       - - - - - -   prefix with the following for debug   - - - - - -
    g++ -fsanitize=address -fno-omit-frame-pointer -g -O1 . . . 
    g++ -fsanitize=thread -g -O1 . . .
                                                              [260918]  */

#include"lpeek010.h" 

// defining extern variables declared in lpeek010.h
Display *dis;
GC gc;
Window win, sel_owner_win;
Atom WM_DELETE_WINDOW, utf8str;
Atom clpbrd, prop, trgts;
XShmSegmentInfo shminfo;
FT_Library lbrry;
FT_Face face;
unsigned *p;          
unsigned clrs[36];
int waW, waH;
int bufW, bufH;
int txtW, txtH, thick;
int ascent, darkmode;
int share, tmr;
std::atomic <int> mode;
unsigned short symbol[256];
unsigned char gct[256];
bool select_own;
std::string strng, clp, hnd;
// end definition of extern variables
 
Pixmap pxmp, icn, msk;
std::vector <XRectangle> rct;
XSizeHints sh;
std::filesystem::path vfp, cfgp;
FILE *fbin, *fcfg;
Atom DRAW_PG_FINISHED;
int maxW, maxH, maxTxtH, locMx;
int cfgW, cfgH, accelerate;
int bline, xtX, xtY, stsz, Tsz; 
int wide, deep, colm, row, detHist;
int mYhist, vScrll, vSlide, vPos, vLeft, vWdth;
unsigned *t;
long fsz, PgStart, loc, locHist, vMax, vNdx;
bool loop, click, lftctl, save_xit;
std::atomic <bool> redraw = true;
unsigned char *rBuf, *setts;


void mkicon() {
  union pnts_t {
    short p[200] = {8, 8, 43, 8, 43, 11, 38, 12, 34, 16, 33, 21, 33, 94,
            34, 99, 38, 103, 43, 104, 78, 104, 83, 103, 87, 99, 88, 94,
            91, 94, 91, 119, 8, 119, 8, 116, 13, 115, 17, 111, 18, 106,
            18, 21, 17, 16, 13, 12, 8, 11, 8, 8, 48, 52, 53, 47,
            64, 43, 75, 39, 87, 34, 99, 27, 111, 19, 113, 24, 104, 32,
            98, 41, 96, 52, 96, 59, 98, 70, 104, 79, 113, 87, 111, 92,
            99, 84, 87, 77, 75, 72, 64, 68, 53, 64, 48, 59, 48, 52,
            113, 24, 117, 40, 119, 52, 119, 59, 117, 71, 113, 87, 104, 79,
            98, 70, 96, 59, 96, 52, 98, 41, 104, 32, 113, 24, 34, 51,
            36, 44, 40, 38, 51, 32, 64, 29, 74, 25, 87, 19, 99, 11,
            103, 8, 115, 8, 115, 15, 111, 19, 99, 27, 87, 34, 75, 39,
            65, 43, 53, 47, 48, 52, 48, 59, 53, 64, 65, 68, 75, 72,
            87, 77, 99, 84, 111, 92, 115, 95, 115, 104, 112, 107, 108, 107,
            99, 100, 87, 92, 74, 86, 64, 82, 51, 79, 40, 73, 36, 67,
            34, 60, 34, 51};
    XPoint xp[100];
  } pnts;
  icn = XCreatePixmap(dis, win, 128, 128, 24);
  XSetForeground(dis, gc, 0xff0044cc);
  XFillPolygon(dis, icn, gc, pnts.xp, 26, Nonconvex, CoordModeOrigin);
  XSetForeground(dis, gc, 0xffdedede);
  XFillPolygon(dis, icn, gc, &pnts.xp[26], 23, Nonconvex, CoordModeOrigin);
  XSetForeground(dis, gc, 0xff0044cc);
  XFillPolygon(dis, icn, gc, &pnts.xp[49], 13, Convex, CoordModeOrigin);
  XSetForeground(dis, gc, 0xff0044cc);
  XFillPolygon(dis, icn, gc, &pnts.xp[62], 38, Nonconvex, CoordModeOrigin);
  msk = XCreatePixmap(dis, win, 128, 128, 1);
  XGCValues tgcv;
  tgcv.function = GCFunction; tgcv.foreground = 1; tgcv.background = 0;
  GC tmpgc = XCreateGC(dis, msk, GCForeground | GCBackground, &tgcv);
  XFillPolygon(dis, msk, tmpgc, pnts.xp, 26, Nonconvex, CoordModeOrigin);
  XFillPolygon(dis, msk, tmpgc, &pnts.xp[26], 23, Nonconvex, CoordModeOrigin);
  XFillPolygon(dis, msk, tmpgc, &pnts.xp[49], 13, Convex, CoordModeOrigin);
  XFillPolygon(dis, msk, tmpgc, &pnts.xp[62], 38, Nonconvex, CoordModeOrigin);
  XFreeGC(dis, tmpgc);
}

int find_string(unsigned char *b, std::string s, int toEOF) {
  int i, j, k, m, n;
  bool f, cf = false;
  m = s.size();
  n = s[0];
  for (i = 0; i < toEOF; i++) {
    if (b[i] == 35) cf = true;
    else if (cf) {
      if (b[i] == 10) cf = false; 
    }
    else if (b[i] == n) {
      f = true; k = i + 1;
      for (j = 1; j < m; j++) {
        if (b[k] != s[j]) f = false;
       	k++;
      }
      if (f) return k;
    }
  }
  return -1;
}

std::string getstr(unsigned char *b, int toEOF) {
  int i = 0;
  while (b[i] != 34) {
    i++;
    if (b[i] < 9 || i > 8192 || i == toEOF) return "\0";
  }
  std::string s(reinterpret_cast<const char*>(b), i);
  return s;
}

int putstr(unsigned char *b, std::string s, int toEOF) {
  int diff, i = 0, j, k;
  while (b[i] != 34) {
    i++;
    if (b[i] < 32 || i > 1023 || i == toEOF) return -9999;
  }
  j = s.size();
  diff = j - i;
  if (diff < 0) {
    for (k = i; k < toEOF; k++) b[k + diff] = b[k];
    for (k = toEOF + diff; k < toEOF; k++) b[k] = 0;
  }
  else if (diff > 0) {
    for (k = toEOF - 1; k >= i; k--) b[k + diff] = b[k];
  }
    for (k = 0; k < j; k++) b[k] = static_cast<unsigned char>(s[k]);
  return diff;
}

long readval(unsigned char *b, int places, bool hex) {
  int i;
  long L = 0, m = 1;
  if (hex) {
    for (places; places > 0; places--) {
      i = b[places - 1];
      if (i > 57) i -= 39;
      L += (i - 48) * m;
      m *= 16;
    }
  } else {
    for (places; places > 0; places--) {
      L += (b[places - 1] - 48) * m;
      m *= 10;
    }
  }
  return L;
}

void writeval(unsigned char *b, long val, int places) {
  int i = 0;
  long m = 1;
  for (places; places > 1; places--) m *= 10;
  for (m; m > 0; m /= 10) {
    b[i] = val / m + 48;
    val %= m;
    i++;
  }
}

void draw_rectangle(int X, int Y, int width, int height, unsigned fg) {
  int i, j, k, m, n;
  j = Y * bufW + X + width; k = (Y + height - 1) * bufW + X;
  for (i = Y * bufW + X; i < j; i++) {
    m = 0;
    for (n = 0; n < thick; n++) {
      p[i + m] = fg; p[k - m] = fg;
      m += bufW;
    }
    k++;
  }
  j = (Y + height - thick) * bufW + X;
  k = (Y + thick) * bufW + X + width - 1;
  for (i = (Y + thick) * bufW + X; i < j; i += bufW) {
    m = 0;
    for (n = 0; n < thick; n++) {
      p[i + m] = fg; p[k - m] = fg;
      m++;
    }
    k += bufW;
  }
}

void fill_rectangle(unsigned* m, int bfW, int X, int Y,
        int width, int height, unsigned fg) {
  int i, j, k, n;
  n = Y * bfW + X;
  j = Y + height; k = X + width;
  for (Y; Y < j; Y++) {
    for (i = X; i < k; i++) {
      m[n] = fg;
      n++;
    }
  n += bfW - width;
  }
}

void vscroll_draw() {
  int i, j, k, m, n, q;
  unsigned c = clrs[1 + darkmode];
  k =  vWdth;
  vPos = (double)vNdx / vMax * vSlide + 0.5;
  q = (vPos + thick) * bufW + vLeft;
  m = q - bufW;
  n = q + (vScrll - 2 * thick) * bufW;
  for (i = 0; i < thick; i++) {
    for (j = 0; j < k; j++) {
      p[m] = c; p[n] = c;
      m++; n++;
    }
    k -= 2;
    m -= bufW + k + 1; n += bufW - k - 1;
  }
  if (vScrll > 2 * thick) fill_rectangle(p, bufW, vLeft, vPos + thick,
          vWdth, vScrll - 2 * thick, c); 
}

void place_tile(int X, int Y, int tile) {
  int i, j, k;
  k = tile * Tsz;
  j = Y * bufW + X;
  for (i = 0; i < Tsz; i++) {
    p[j] = t[k];
    j++; k++;
    if (i % txtW == txtW - 1) j += bufW - txtW;
  }
}

void draw_glyph(unsigned *m, int bfW, int X, int Y,
        unsigned short symbl, unsigned fg) {
  if (FT_Load_Char(face, symbl, FT_LOAD_RENDER)) return;
  FT_GlyphSlot slot = face->glyph;
  if (slot->bitmap.width == 0) return;
  if (FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL)) return;
  unsigned char fR, fG, fB, bR, bG, bB;
  unsigned char *g = &slot->bitmap.buffer[0];
  unsigned char o;
  unsigned d;
  int h, i, j, k = 0, n;
  double r;
  h = slot->bitmap.width;
  X += slot->bitmap_left;
  Y -= slot->bitmap_top;
  i = int(Y + slot->bitmap.rows * 0.5) * bfW + X + h * 0.5;
  d = m[i];
  bR = ((d & 0xff0000) >> 16); bG = ((d & 0xff00) >> 8); bB = (d & 0xff);
  fR = ((fg & 0xff0000) >> 16); fG = ((fg & 0xff00) >> 8); fB = (fg & 0xff);
  j = Y + slot->bitmap.rows;
  n = Y * bfW + X;
  for (Y; Y < j; Y++) {
    for (i = 0; i < h; i++) {
      if (g[k] == 255) {
        m[n] = fg;
      }
      else if (g[k] > 0) {
        d = 0xff000000;
        r = gct[g[k]] / 255.0;
        o = r * fR + (1 - r) * bR + 0.5;
        d += o << 16;
        o = r * fG + (1 - r) * bG + 0.5;
        d += o << 8;
        o = r * fB + (1 - r) * bB + 0.5;
        d += o;
        m[n] = d;
      }
      k++; n++;
    }
    n += bfW - h;
  }
}

void paint() {
  for (int i = 0; i < rct.size(); i++) {
    XCopyArea(dis, pxmp, win, gc, rct[i].x, rct[i].y,
            rct[i].width, rct[i].height, rct[i].x, rct[i].y);
  }
  XFlush(dis);
  rct.clear();
  redraw = true;
}

void launch_dialog() {
  sh.min_width = bufW; sh.min_height = bufH;
  sh.max_width = bufW; sh.max_height = bufH;
  XSetWMNormalHints(dis, win, &sh);
  maind();
}

void filesys_dialog() {
  redraw = false;
  int g, h, i = 0, j, k, m = 0, n, q;
  unsigned c, u;
  XRectangle r;
  std::filesystem::path tp = strng.c_str();
  j = txtH + 4 * thick;
  r.x = 0; r.y = bufH - j; r.width = bufW; r.height = j;
  fill_rectangle(p, bufW, r.x, r.y, r.width, r.height, clrs[17 + darkmode]);
  j = strng.size();
  while (i < j) {
    if (strng[i] < 128) k = 1;
    else if ((strng[i] & 224) == 192) k = 2;
    else if ((strng[i] & 240) == 224) k = 3;
    else if ((strng[i] & 248) == 240) k = 4;
    else k = 1;
    if (strng[i] == 47 && i < j - 1) n = m + 1;
    i += k; m++;
  }
  if (std::filesystem::is_directory(tp)) n = m;
  if (darkmode) c = clrs[19]; else c = clrs[0];
  h = 0;
  if (n > wide - 20) {
    q = n - (wide - 20) + 2; share = (wide - 20) * txtW;
    g = r.y + txtH * 1.4;
    draw_glyph(p, bufW, h, g, 168, c);
    h += txtW;
    draw_glyph(p, bufW, h, g, 168, c);
    h += txtW;
  }
  else { q = 0; share = n * txtW; }
  i = 0; m = 0; g = r.y + ascent;
  while (i < j) {
    if (strng[i] < 128) { k = 1; u = strng[i]; }
    else if ((strng[i] & 224) == 192) {
      k = 2;
      u = ((strng[i] & 31) << 6) + (strng[i + 1] & 63);
    }
    else if ((strng[i] & 240) == 224) {
      k = 3;
      u = ((strng[i] & 15) << 12) + ((strng[i + 1] & 63) << 6)
              + (strng[i + 2] & 63);
    }
    else if ((strng[i] & 248) == 240) {
      k = 4;
      u = ((strng[i] & 7) << 18) + ((strng[i + 1] & 63) << 12)
              + ((strng[i + 2] & 63) << 6) + (strng[i + 3] & 63);
    }
    else { k = 1; u = 0; }
    if (m >= q) {
      draw_glyph(p, bufW, h, g, u, c);
      h += txtW;
    }
    i += k; m++;
    if (m == n) break;
  }
  rct.push_back(r);
  paint();
  launch_dialog();
}

void draw_page() {
  int i, j, k, m, n, q;
  long L;
  bool xit = false;
  i = wide * deep;
  L = fsz - PgStart;
  if (i > L) i = L;
  fseek(fbin, PgStart, SEEK_SET);
  fread(rBuf, 1, i, fbin);
  L = PgStart;
  j = 0; n = 0;
  for (m = 0; m < deep; m++) {
    i = 0;
    for (k = 0; k < wide; k++) {
      q = rBuf[n];
      if (k % 2 != m % 2) q += 256;
      place_tile(i, j, q);
      i += txtW; L++; n++;
      if (L == fsz) { xit = true; break; }
    }
    if (xit) break;
    j += txtH;
  }
  paint();
  if (mode != 0) {
    XEvent ev{};
    ev.type = ClientMessage;
    ev.xclient.window = win;
    ev.xclient.message_type = DRAW_PG_FINISHED;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = 0;
    ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(dis, win, False, NoEventMask, &ev);
    XFlush(dis);
  }
}

void scroll_page() {
  redraw = false;
  XRectangle r;
  fill_rectangle(p, bufW, 0, xtY - txtH, xtX, txtH, clrs[16 + darkmode]);
  if (locHist != -1) {
    r.x = 0; r.y = bufH - txtH;
    r.width = txtW * detHist; r.height = txtH;
    fill_rectangle(p, bufW, r.x, r.y, r.width, r.height, clrs[16 + darkmode]);
    rct.push_back(r);
    locHist = -1;
  }
  r.x = vLeft; r.y = vPos; r.width = vWdth; r.height = vScrll;
  rct.push_back(r);
  fill_rectangle(p, bufW, vLeft, vPos, vWdth, vScrll, clrs[16 + darkmode]);
  vscroll_draw();
  r.x = vLeft; r.y = vPos; r.width = vWdth; r.height = vScrll;
  rct.push_back(r);
  r.x = 0; r.y = 0; r.width = xtX; r.height = xtY;
  rct.push_back(r);
  PgStart = vNdx * wide;
  std::thread t1(draw_page); t1.detach();
}

void clear_details() {
  redraw = false;
  XRectangle r;
  int i, j, k, n;
  k = locHist;
  i = k % wide;
  j = k / wide;
  r.x = i * txtW; r.y = j * txtH; r.width = txtW; r.height = txtH;
  n = rBuf[locHist];
  if (i % 2 != j % 2) n += 256;
  place_tile(r.x, r.y, n);
  rct.push_back(r);
  locHist = -1;
  r.x = 0; r.y = bufH - txtH;
  r.width = txtW * detHist; r.height = txtH;
  fill_rectangle(p, bufW, r.x, r.y, r.width, r.height, clrs[16 + darkmode]);
  rct.push_back(r);
  paint();
}

void draw_details() {
  redraw = false;
  XRectangle r;
  int i, j, k, c;
  long m, n;
  if (locHist != -1) {
    fill_rectangle(p, bufW, 0, bufH - txtH, txtW * detHist, txtH,
            clrs[16 + darkmode]);
    k = locHist;
    i = k % wide;
    j = k / wide;
    r.x = i * txtW; r.y = j * txtH; r.width = txtW; r.height = txtH;
    n = rBuf[locHist];
    if (i % 2 != j % 2) n += 256;
    place_tile(r.x, r.y, n);
    rct.push_back(r);
  }
  r.x = colm * txtW; r.y = row * txtH; r.width = txtW; r.height = txtH;
  draw_rectangle(r.x, r.y, r.width, r.height, clrs[darkmode]);
  rct.push_back(r);
  if (darkmode) c = 28; else c = 11;
  draw_glyph(p, bufW, 0, bline, symbol[91], clrs[c]);
  i = 1; n = loc + PgStart;
  for (m = 10; m <= n; m *= 10) i++;
  m /= 10; j = 1;
  for (i; i > 0; i--) {
    k = n % m;
    draw_glyph(p, bufW, j * txtW, bline, symbol[n / m + 48], clrs[c]);
    j++; n = k; m/= 10;
  }
  draw_glyph(p, bufW, j * txtW, bline, symbol[93], clrs[c]);
  if (darkmode) c = 21; else c = 0;
  j++;
  draw_glyph(p, bufW, j * txtW, bline, symbol[61], clrs[c]);
  j++; i = 1;
  for (m = 10; m <= rBuf[loc]; m *= 10) i++;
  m /= 10; n = rBuf[loc];
  for (i; i > 0; i--) {
    k = n  % m;
    draw_glyph(p, bufW, j * txtW, bline, symbol[n / m + 48], clrs[c]);
    j++; n = k; m /= 10;
  }
  locHist = loc;
  if (detHist > j) i = detHist; else i = j;
  detHist = j;
  r.x = 0; r.y = bufH - txtH;
  r.width = txtW * i; r.height = txtH;
  rct.push_back(r);
  paint();
}

void tile_foundry() {
  int c, i, j, k, m, n;
  c = darkmode + 16; k = thick + 1;
  n = 0;
  for (i = 0; i < 512; i++) {
    fill_rectangle(p, bufW, 0, 0, txtW + 2 * k, txtH + 2 * k, clrs[c]);
    draw_glyph(p, bufW, k, ascent + k, symbol[i % 256],
            clrs[i % 16 + darkmode]);
    m = k * bufW + k; 
    for (j = 0; j < Tsz; j++) {
      t[n] = p[m];
      m++; n++;
      if (j % txtW == txtW - 1) m += bufW - txtW;
    }
    if (i % 256 == 255) c++;
  }
}

void zoom(bool fontsz) {
  redraw = false;
  long L;
  if (fontsz) {
    FT_Set_Pixel_Sizes(face, 0, txtH);
    FT_Load_Char(face, 9608, FT_LOAD_RENDER);
    ascent = face->glyph->bitmap_top;
    txtW = txtH * 0.60 + 0.5;
    thick = txtH / 16; Tsz = txtW * txtH;
    tile_foundry();
    bline = bufH - txtH + ascent;
    wide = (bufW - txtW) / txtW; deep = (bufH - txtH) / txtH;
    locMx = wide * deep - 1;
    if (loc > locMx) loc = locMx;
    if (loc > fsz - PgStart - 1) loc = fsz - PgStart - 1;
    accelerate = deep;
    xtX = wide * txtW; xtY = deep * txtH;
  }
  if (fsz % wide == 0) L = fsz + 1; else L = fsz;
  vMax = L / wide + 1 - deep;
  if (vMax > 0) {
    vScrll = (double)deep / (vMax + deep) * xtY;
    if (vScrll < 2 * thick) vScrll = 2 * thick;
    vSlide = xtY - vScrll;
  }
  else { vScrll = xtY; vPos = 0; }
  vLeft = bufW - txtW + 2 * thick;
  vWdth = txtW - 4 * thick;
  vNdx = PgStart / wide;
  if (vNdx > vMax) vNdx = vMax;
  if (vNdx < 0) vNdx = 0;
  PgStart = vNdx * wide;
  locHist = -1;
  fill_rectangle(p, bufW, xtX, 0, bufW - xtX, bufH, clrs[16 + darkmode]);
  if (vMax < 1) L = fsz / wide * txtH; else L = xtY - txtH;
  fill_rectangle(p, bufW, 0, L, xtX, bufH - L, clrs[16 + darkmode]);
  if (vMax > 0) vscroll_draw();
  rct.resize(1);
  rct[0].x = 0; rct[0].y = 0; rct[0].width = bufW; rct[0].height = bufH;
  std::thread t1(draw_page); t1.detach();
}

void dialog_return() {
    if (mode == 1) {
      if (share == -1) mode = 0;
      else if (share == 0) filesys_dialog();
      else if (share == 2) {
        strng = "Clipboard is empty"; mode = 200;
      }
      else if (share == 3) {
        strng = "Clipboard contents not a filepath"; mode = 202;
      }
      else if (share == 4) {
        strng = "Directory permission denied"; mode = 203;
      }
      else if (share == 5) {
        strng = "File permission denied"; mode = 204;
      }
      else if (share == 6) {
        strng = "FilePath Copied"; mode = 205;
      }
      if (mode > 199) launch_dialog();
    }
    else if (mode > 99 && mode < 200) mode = 0;
    else if (mode == 201 || mode == 205) mode = 0;
    else if (mode > 199) { mode = 1; strng = hnd; filesys_dialog(); }
}

void resize_draw() {
  bufW = cfgW; bufH = cfgH;
  if (pxmp) XFreePixmap(dis, pxmp);
  pxmp = XShmCreatePixmap(dis, win, shminfo.shmaddr, &shminfo,
          bufW, bufH, 24);
  bline = bufH - txtH + ascent;
  wide = (bufW - txtW) / txtW; deep = (bufH - txtH) / txtH;
  locMx = wide * deep - 1;
  if (loc > locMx) loc = locMx;
  if (loc > fsz - PgStart - 1) loc = fsz - PgStart - 1;
  accelerate = deep;
  xtX = wide * txtW; xtY = deep * txtH;
  zoom(false);
}

void init() {
  int i, j;
  bool f = false;
  std::string sl, sfn;
  dis = XOpenDisplay(0);
  rct.reserve(6);
  mode = 0;
                   //Section: Load settings
  save_xit = true; sl = ".lpeekrc";
  cfgp = std::filesystem::canonical("/proc/self/exe").parent_path()
          /".lpeekrc";
  if (!std::filesystem::exists(cfgp)) {
    cfgp = std::filesystem::path(std::getenv("HOME"))/".lpeekrc";
    if (!std::filesystem::exists(cfgp)) {
      printf(".lpeekrc is missing from your home dir\n");
      save_xit = false; sl = "lpeek.conf";
      cfgp = CONF_DIR"/lpeek.conf";
      if (!std::filesystem::exists(cfgp)) {
        printf("lpeek.conf is missing from %s\n", CONF_DIR); return;
      }
    }
  }
  stsz = std::filesystem::file_size(cfgp);
  fcfg = fopen(cfgp.c_str(), "rb");
  setts = new unsigned char[stsz + 1024];
  fread(setts, 1, stsz, fcfg);
  fclose(fcfg);
  i = find_string(setts, "MaxWdth=", stsz);
  if (i == -1) { printf("%s is missing MaxWdth=\n", sl.c_str()); return; }
  maxW = readval(&setts[i], 4, false);
  if (maxW < 300) maxW = 300;
  i = find_string(setts, "MaxHght=", stsz);
  if (i == -1) { printf("%s is missing MaxHght=\n", sl.c_str()); return; }
  maxH = readval(&setts[i], 4, false);
  if (maxH < 240) maxH = 240;
  i = find_string(setts, "MaxTxtHght=", stsz);
  if (i == -1) { printf("%s is missing MaxTxtHght=\n", sl.c_str()); return; }
  maxTxtH = readval(&setts[i], 2, false);
  if (maxTxtH < 16 || maxTxtH > 67) maxTxtH = 67;
                           //Sub Section: Set up memory usage
  waW = XDisplayWidth(dis, 0); waH = XDisplayHeight(dis, 0);
  if (waW > maxW) waW = maxW;
  if (waH > maxH) waH = maxH;
  shminfo.readOnly = False;
  shminfo.shmid = shmget(IPC_PRIVATE, waW * waH * 6, IPC_CREAT | 0777);
  shminfo.shmaddr = reinterpret_cast<char*>(shmat(shminfo.shmid, 0, 0));
  p = reinterpret_cast<unsigned*>(shminfo.shmaddr);
  XShmAttach(dis, &shminfo);
  j = maxTxtH * 0.6 + 0.5;
  t = new unsigned [maxTxtH * j * 512];
                           //Sub Section: Get global variables
  i = find_string(setts, "Width=", stsz);
  if (i == -1) { printf("%s is missing Width=\n", sl.c_str()); return; }
  cfgW = readval(&setts[i], 4, false);
  if (cfgW < 300) cfgW = 300;
  if (cfgW > maxW) cfgW = maxW;
  i = find_string(setts, "Height=", stsz);
  if (i == -1) { printf("%s is missing Height=\n", sl.c_str()); return; }
  cfgH = readval(&setts[i], 4, false);
  if (cfgH < 240) cfgH = 240;
  if (cfgH > maxH) cfgH = maxH;
  i = find_string(setts, "TextHeight=", stsz);
  if (i == -1) { printf("%s is missing TextHeight=\n", sl.c_str()); return; }
  txtH = readval(&setts[i], 2, false);
  if (txtH < 16) txtH = 16;
  if (txtH > maxTxtH) txtH = maxTxtH;
  i = find_string(setts, "DarkMode=", stsz);
  if (i == -1) { printf("%s is missing DarkMode=\n", sl.c_str()); return; }
  darkmode = readval(&setts[i], 2, false);
  if (!(darkmode == 0 || darkmode == 18)) darkmode = 18;
  i = find_string(setts, "PgStart=", stsz);
  if (i == -1) { printf("%s is missing PgStart=\n", sl.c_str()); return; }
  PgStart = readval(&setts[i], 14, false);
                           //Sub Section: Get color table
  j = find_string(setts, "Colors=", stsz);
  if (j == -1) { printf("%s is missing Colors=\n", sl.c_str()); return; }
  i = 0;
  while (i < 36) {
    clrs[i] = 0xff000000 + readval(&setts[j], 6, true);
    i++; j += 7;
  }
                           //Sub Section: Get symbol list
  j = find_string(setts, "Symbols=", stsz);
  if (j == -1) { printf("%s is missing Symbols=\n", sl.c_str()); return; }
  i = 0;
  while (i < 256) {
    symbol[i] =  readval(&setts[j], 4, true);
    i++; j += 5;
  }
                           //Sub Section: Prep font
  if (FT_Init_FreeType(&lbrry)) {
          printf("FreeType library did not load.\n"); return; }
  i = find_string(setts, "FontPath=", stsz) + 1;
  if (i == 0) { printf("%s is missing Fontpath=\n", sl.c_str()); return; }
  sfn = getstr(&setts[i], stsz - i);
  if (sfn == "\0") {
          printf("%s: Fontpath is corrupt.\n", sl.c_str()); return; }
  if (FT_New_Face(lbrry, sfn.c_str(), 0, &face)) {
    printf("Font failed. Trying app dir ...\n");
    std::filesystem::path temp;
    temp = std::filesystem::canonical("/proc/self/exe").parent_path()
            /"FreeMono.otf";
    if (!std::filesystem::exists(temp)) {
      printf("No FreeMono.otf in app dir. Trying share dir ...\n");
      temp = SHARE_DIR "/FreeMono.otf";
      if (!std::filesystem::exists(temp)) {
        printf(" FreeMono.otf is missing from %s\n", SHARE_DIR); return;
      }
    }
    if (FT_New_Face(lbrry, temp.c_str(), 0, &face)) {
            printf("Font failed. ???\n"); return; }
  }
  FT_Set_Pixel_Sizes(face, 0, txtH);
  FT_Load_Char(face, 9608, FT_LOAD_RENDER);
  ascent = face->glyph->bitmap_top;
  txtW = txtH * 0.60 + 0.5;
  thick = txtH / 16; Tsz = txtW * txtH;
                           //Sub Section: Read buffer and file size
  i = waW / 10 + 1; j = waH / 16 + 1;
  rBuf = new unsigned char[i * j];
  i = find_string(setts, "ViewFile=", stsz) + 1;
  if (i == 0) { printf("%s is missing ViewFile=\n", sl.c_str()); return; }
  sfn = getstr(&setts[i], stsz - i);
  if (sfn == "\0")
          { printf("%s: File path is corrupt.\n", sl.c_str()); return; }
  vfp = sfn;
  if (std::filesystem::exists(vfp)) {
    if (!std::filesystem::is_regular_file(vfp)) f = true;
    else {
      i = ::access(vfp.c_str(), R_OK);
      if (i != 0) f = true;
    }
  }
  else f = true;
  if (f) {
    printf("File is missing or denied. Reverting to default.\n");
    vfp = SHARE_DIR "/lpeek_example.bin";
    if (!std::filesystem::exists(vfp)) {
      printf("lpeek_example.bin is missing from %s\n", SHARE_DIR);
      vfp = std::filesystem::canonical("/proc/self/exe").parent_path()
              / "lpeek_example.bin"; 
      if (!std::filesystem::exists(vfp)) {
        printf("lpeek_example.bin is missing from app dir\n"); return;
      }
    }
  }
  sfn = vfp;
  fsz = std::filesystem::file_size(vfp);
  fbin = fopen(vfp.c_str(), "rb");
                   //Section: Set up gamma correction table
  double c, d;
  unsigned char cv[] = {5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20,
                        23, 27, 32, 39}; //curve
  j = 0;
  d = 255.5;
  for (i = 255; i > -1; i--) {
    gct[i] = d;
    if (i % 16 == 15) {
       c = cv[j] / 16.0;
       j++;
    }
    d -= c;
  }
  bufW = 200; bufH = 300;
  tile_foundry();
                   //Section: Prep Window
  gc = XDefaultGC(dis, DefaultScreen(dis));
  int attriMask = CWBackPixel | CWWinGravity | CWEventMask;
  XSetWindowAttributes winAttr;
  winAttr.background_pixel = 0xff000033;
  winAttr.win_gravity = CenterGravity;
  winAttr.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask |
          ButtonReleaseMask | PointerMotionMask | ExposureMask |
          StructureNotifyMask;
  int left = (waW - cfgW) / 2;
  int top = (waH - cfgH) / 2;
  win = XCreateWindow(dis, XDefaultRootWindow(dis), left, top,
          cfgW, cfgH, 1, DefaultDepth(dis,0), InputOutput,
          DefaultVisual(dis, 0), attriMask, &winAttr);
  WM_DELETE_WINDOW = XInternAtom(dis, "WM_DELETE_WINDOW", False);
  DRAW_PG_FINISHED = XInternAtom(dis, "DRAW_PG_FINISHED", False);
  XSetWMProtocols(dis, win, &WM_DELETE_WINDOW, 1);
  XWMHints hnts;
  sh.flags = PPosition | PSize | PMinSize | PMaxSize | PWinGravity;
  sh.x = left; sh.y = top; sh.width = cfgW; sh.height = cfgH;
  sh.max_width = waW; sh.max_height = waH; 
  sh.min_width = maxTxtH * 5.4; sh.min_height = maxTxtH * 4.5;
  sh.win_gravity = CenterGravity;
  hnts.flags = 37; hnts.input = true;
  mkicon();
  hnts.icon_pixmap = icn; hnts.icon_mask = msk;
  XStoreName(dis, win, sfn.c_str());
  sfn = vfp.filename();
  XSetIconName(dis, win, sfn.c_str());
  XSetWMNormalHints(dis, win, &sh);
  XSetWMHints(dis, win, &hnts);
                   //Section: Prep selections
  sel_owner_win = XCreateSimpleWindow(dis, XDefaultRootWindow(dis),
          -10, -10, 1, 1, 0, 0, 0);
  clpbrd = XInternAtom(dis, "CLIPBOARD", False);
  utf8str = XInternAtom(dis, "UTF8_STRING", False);
  trgts = XInternAtom(dis, "TARGETS", False);
  prop = XInternAtom(dis, "XSEL_DATA", False);
  XMapWindow(dis, win);
  loop = true;
}

void save_setts() {
  int i, j;
  std::string ts, sl;
  i = find_string(setts, "Width=", stsz);
  if (i == -1) { printf(".lpeekrc is corrupt. Not saved.\n"); return; }
  writeval(&setts[i], bufW, 4);
  i = find_string(setts, "Height=", stsz);
  if (i == -1) { printf(".lpeekrc is corrupt. Not saved.\n"); return; }
  writeval(&setts[i], bufH, 4);
  i = find_string(setts, "TextHeight=", stsz);
  if (i == -1) { printf(".lpeekrc is corrupt. Not saved.\n"); return; }
  writeval(&setts[i], txtH, 2);
  i = find_string(setts, "DarkMode=", stsz);
  if (i == -1) { printf(".lpeekrc is corrupt. Not saved.\n"); return; }
  writeval(&setts[i], darkmode, 2);
  i = find_string(setts, "PgStart=", stsz);
  if (i == -1) { printf(".lpeekrc is corrupt. Not saved.\n"); return; }
  writeval(&setts[i], PgStart, 14);
  i = find_string(setts, "ViewFile=", stsz) + 1;
  if (i == 0) { printf(".lpeekrc is corrupt. Not saved.\n"); return; }
  ts = getstr(&setts[i], stsz - i);
  if (ts == "\0") { printf(".lpeekrc: Filepath is corrupt.\n"); return; }
  sl = vfp.c_str();
  if (ts != sl) {
    j = putstr(&setts[i], sl, stsz - i);
    if (j == -9999) { printf(".lpeekrc is corrupt. Not saved.\n"); return; }
    stsz += j;
  }
  fcfg = fopen(cfgp.c_str(), "wb");
  fwrite(setts, 1, stsz, fcfg);
  fclose(fcfg);
}

void finish() {
  if (sel_owner_win) XDestroyWindow(dis, sel_owner_win);
  if (t) delete[] t;
  if (rBuf) delete[] rBuf;
  if (setts) delete[] setts;
  if (pxmp) XFreePixmap(dis, pxmp);
  if (win) XDestroyWindow(dis, win);
  if (fbin) fclose(fbin);
  if (face) FT_Done_Face(face);
  if (lbrry) FT_Done_FreeType(lbrry);
  if (shminfo.shmid > -1) {
    XShmDetach(dis, &shminfo);
    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, 0);
  }
}

void refresh() {
  rct.resize(1);
  rct[0].x = 0; rct[0].y = 0; rct[0].width = bufW; rct[0].height = bufH;
  paint();
}

int main() {
  init();
  if (!loop) { finish(); return 1; }
  KeySym key;
  int i, j, mX, mY;
  double d;
  long L;
  char text;
  bool  pf, tf, msdwn = false;
  XSelectionRequestEvent *sev;
  XSelectionEvent ssev;
  XEvent evnt, nev;
  XConfigureEvent *xcfg;
  while(loop) {
    XNextEvent(dis, &evnt);
    pf = false;
    switch(evnt.type) {
      case KeyPress:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_Control_L) lftctl = true;
	if (!redraw || mode != 0) break;
	if (key == XK_w || key == XK_Prior || key == XK_KP_Prior) {
          vNdx -= accelerate; if (vNdx < 0) vNdx = 0;
          scroll_page();
	  accelerate *= 1.2;
	  if (accelerate < 6) accelerate = 6;
        }
        else if (key == XK_s || key == XK_Next || key == XK_KP_Next) {
          vNdx += accelerate; if (vNdx > vMax) vNdx = vMax;
          scroll_page();
	  accelerate *= 1.2;
	  if (accelerate < 6) accelerate = 6;
        }
        else if ((key == XK_Left || key == XK_KP_Left || key == XK_h)
                && loc > 0) {
          loc--; colm = loc % wide; row = loc / wide;
          draw_details();
	}
        else if ((key == XK_Right || key == XK_KP_Right || key == XK_l)
                && loc < locMx) {
          loc++;
	  if (loc + PgStart >= fsz) loc = fsz - PgStart - 1;
          colm = loc % wide; row = loc / wide;
          draw_details();
	}
	else if (key == XK_Up || key == XK_KP_Up || key == XK_k) {
	  if (row < 1 && vNdx > 0) { vNdx--; scroll_page(); }
	  else if (loc >= wide) { loc -= wide ; colm = loc % wide;
		  row = loc / wide; draw_details(); }
	}
	else if (key == XK_Down || key == XK_KP_Down || key == XK_j) {
	  if (row > deep - 2 && vNdx < vMax) { vNdx++; scroll_page(); }
	  else if (loc <= locMx - wide && loc < fsz - PgStart - wide) {
            loc += wide ; colm = loc % wide; row = loc / wide;
	    draw_details();
	  }
	}
	else if (key == XK_n && txtH > 16) { txtH--; zoom(true); }
	else if (key == XK_m && txtH < maxTxtH) { txtH++; zoom(true); }
        break;
      case KeyRelease:
        if (mode == 1) break;
        if (XEventsQueued(dis, QueuedAfterReading)) {
          XPeekEvent(dis, &nev);
          if (nev.type == KeyPress &&
                  nev.xkey.time == evnt.xkey.time &&
                  nev.xkey.keycode == evnt.xkey.keycode) break;
        }
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (!msdwn && (key == XK_Escape || key == XK_q)) loop = false;
        else if (!msdwn && redraw && key == XK_y) {
          if (darkmode) darkmode = 0; else darkmode = 18;
          tile_foundry();
          zoom(false);
        }
        else if (key == XK_Control_L) lftctl = false;
        else if (!msdwn && redraw && key == XK_i) {
          if (wide < 40 || deep < 6) {
            mode = 100; tmr = 800;
            strng = "Please enlarge window."; launch_dialog();
	  }
          else {
            i = find_string(setts, "InfoMsg=", stsz) + 1;
            if (i == 0) {
              strng = "lpeek.conf|.lpeekrc is missing InfoMsg=";
	      tmr = 1200; mode = 201; launch_dialog();
	    }
	    else {
	      mode = 101;
              strng = getstr(&setts[i], stsz - i);
              launch_dialog();
	    }
	  }
        }
	else if (key == XK_f && redraw) {
          if (wide < 40 || deep < 6) { mode = 100; tmr = 800;
                  strng = "Please enlarge window."; launch_dialog(); }
          else { mode = 1; strng = vfp.c_str(); filesys_dialog(); }
	}
	else if ((key == XK_w || key == XK_Prior || key == XK_KP_Prior)
                && mode == 0) accelerate = deep;
	else if ((key == XK_s || key == XK_Next || key == XK_KP_Next)
                && mode == 0) accelerate = deep;
        break;
      case ButtonPress:
        if (mode == 1) break;
        if (evnt.xbutton.button == 1) {
          click = true; msdwn = true; mYhist = mY;
        }
        else if (evnt.xbutton.button == 4) {
          if (lftctl) {
            if (redraw && txtH < maxTxtH) {
              txtH++;
              zoom(true);
            }
          }
          else {
            if (redraw && vNdx > 0) { vNdx--; scroll_page(); }
          }
        }
        else if (evnt.xbutton.button == 5) {
          if (lftctl) {
            if (redraw && txtH > 16) {
              txtH--;
              zoom(true);
            }
          }
          else {
            if (redraw && vNdx < vMax) { vNdx++; scroll_page(); }
          }
        }
        break;
      case ButtonRelease:
        if (mode == 1) break;
        if (evnt.xbutton.button == 1) {
          msdwn = false;
          if (redraw && click) {
            if (mY < vPos) {
              vNdx -= deep; if (vNdx < 0) vNdx = 0;
              scroll_page();
            }
            else if (mY > vPos + vScrll) {
              vNdx += deep; if (vNdx > vMax) vNdx = vMax;
              scroll_page();
            }
          }
        }
        else if (evnt.xbutton.button == 3 && redraw) {
          if (wide < 40 || deep < 6) { mode = 100; tmr = 800;
                  strng = "Please enlarge window."; launch_dialog(); }
          else { mode = 1; strng = vfp.c_str(); filesys_dialog(); }
        }
        break;
      case MotionNotify:
        if (mode == 1) break;
        mX = evnt.xbutton.x; mY = evnt.xbutton.y;
        if (msdwn) {
          if (vMax > 0 && mYhist != mY) {
            d = (mY - mYhist) / (double)vSlide;
            L = vNdx + d * (vMax + 1) + 0.5;
            if (L > vMax) L = vMax; 
            if (L < 0) L = 0;
            if (redraw && L != vNdx) {
              click = false; mYhist = mY; vNdx = L;
              scroll_page();
            }
          }
        } else {
          tf = false;
          if (mX > 0 && mX < xtX && mY > 0 && mY < xtY) {
            i = mX / txtW; j = mY / txtH;
            L = j * wide + i;
            if (L + PgStart < fsz) {
              if (redraw && L != locHist) {
                colm = i; row = j; loc = L;
                draw_details();
              }
            }
            else if (locHist != -1) tf = true;
          }
          else if (locHist != -1) tf = true;
          if (redraw && tf) clear_details();
        }
        break;
      case Expose:
        if (mode != 0) break;
        if (!redraw) break;
        if (cfgW != bufW || cfgH != bufH) resize_draw();
        else pf = true;
        break;
      case ConfigureNotify:
        xcfg = reinterpret_cast<XConfigureEvent*>(&evnt);
        cfgW = xcfg->width; cfgH = xcfg->height;
        break;
      case DestroyNotify:
        sh.max_width = waW; sh.max_height = waH; 
        sh.min_width = maxTxtH * 5.4; sh.min_height = maxTxtH * 4.5;
        XSetWMNormalHints(dis, win, &sh);
      	if (mode == 1 && share == 1) {
          fclose(fbin);
          vfp = strng.c_str();
          fsz = std::filesystem::file_size(vfp);
          fbin = fopen(strng.c_str(), "rb");
          PgStart = 0; loc = 0;
          XStoreName(dis, win, strng.c_str());
          strng = vfp.filename();
          XSetIconName(dis, win, strng.c_str());
          mode = 0;
        }
	zoom(false);
        break;
      case ClientMessage:
	if (evnt.xclient.message_type == DRAW_PG_FINISHED) {
          dialog_return();
        }
	else if ((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW)
                loop = false;
        break;
      case SelectionClear:
        select_own = false;
        clp = "";
        break;
      case SelectionRequest:
        sev = (XSelectionRequestEvent*)&evnt.xselectionrequest;
        if (sev->target == trgts) {
            Atom mytargets[] = { utf8str, XA_STRING };
            XChangeProperty(dis, sev->requestor, sev->property,
                    XA_ATOM, 32, PropModeReplace,
		    reinterpret_cast<unsigned char*>(mytargets), 4);
        }
	else {
        XChangeProperty(dis, sev->requestor, sev->property,
               utf8str, 8,  PropModeReplace,
               (unsigned char *)clp.c_str(), clp.size());
	}
        ssev.property = sev->property;
        ssev.target = sev->target;
        ssev.type = SelectionNotify;
        ssev.requestor = sev->requestor;
        ssev.selection = sev->selection;
        ssev.time = sev->time;
        XSendEvent(dis, sev->requestor, True, NoEventMask,
                (XEvent *)&ssev);
        break;
    }
    if (pf) refresh();
  }
  if (save_xit) save_setts();
  finish();
  return 0;
}
