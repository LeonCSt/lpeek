/* wpeek011.cpp  | A binary file viewer    
 *    Dialog is launched by calling  maind()  from anywhere in this file.
    Execution resumes in  dialog_return()  when dialog is closed.

    *needs*  wpeek010.h    | the header file for extern variables
             wpeek012.cpp  | the file that handles the popup dialog

    *needs*  xdg-shell-protocol.cpp       | *see* README.md
             xdg-shell-client-protocol.h  |

    g++ -DCONF_DIR='""' -DSHARE_DIR='""' -I/usr/include/freetype2 xdg-shell-protocol.cpp wpeek011.cpp wpeek012.cpp -o lpeek -lwayland-client -lwayland-cursor -lfreetype
      Optionally, run with debug > >$ WAYLAND_DEBUG=1 ./lpeek
        or - - - -   prefix with the following for debug   - - - -
    g++ -fsanitize=address -fno-omit-frame-pointer -g -O1 . . .
    g++ -fsanitize=thread -g -O1 . . .
                                                       [261122] */


#include"wpeek010.h"
using namespace std;

// defining extern variables declared in lpeek010.h
wl_display *dis;
wl_compositor *comp;
wl_shm *shm;
xdg_wm_base *xdgwmb;
wl_seat *seat;
wl_surface *d_surf;
xdg_surface *xdgsurf;
wl_cursor_theme *crsr_theme;
wl_cursor *crsr;
FT_Library lbrry;
FT_Face face;
unsigned *p;          
unsigned clrs[36];
int monW, monH, bufW, bufH;
int txtW, txtH, thick;
int ascent, darkmode;
int share, tmr;
int kydelay, kyrate;
atomic <int> mode, wheel;
atomic <int> mX, mY;
unsigned short symbol[256];
unsigned char gct[256];
bool select_own, lftctl, clpbrd;
atomic <bool> podup, popfocus;
string strng, clp, hnd;
atomic <unsigned> ky, kyst;
atomic <unsigned> btn, btnst;
// end definition of extern variables

wl_output *out;
wl_registry *reg;
wl_surface *surf;
xdg_toplevel *xdgtop;
wl_shm_pool *pool;
wl_buffer *buff;
wl_surface *crsr_surf;
wl_buffer *crsr_buff;
wl_cursor_image *crsr_img;
wl_pointer *pntr;
wl_keyboard *kybrd;
wl_data_device_manager *datadman;
wl_data_device *datdvc;
wl_data_source *datsrc;
wl_data_offer *dta_offr, *hst_offr;
vector <rect> rct;
filesystem::path vfp, cfgp;
FILE *fbin, *fcfg;
mutex mtx;
int surfW, surfH, rky;
int maxW, maxH, maxTxtH, locMx;
int accelerate, szM;
int bline, xtX, xtY, stsz, Tsz; 
int wide, deep, colm, row, detHist;
int mYhist, vScrll, vSlide, vPos, vLeft, vWdth;
unsigned *t;
unsigned pntr_serial, kbrd_serial;
long fsz, PgStart, loc, locHist, vMax, vNdx;
bool evloop, click, save_xit, relaunch;
atomic <bool> redraw = true, kyrep = false;
unsigned char *rBuf, *setts;
unsigned kys[] = {17, 31, 35, 36, 37, 38, 49, 50, 72, 73, 75, 77,
                  80, 81, 103, 104, 105, 106, 108, 109};
unsigned kyu[] = {17, 31, 73, 81, 104, 109};

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

void draw_rectangle(unsigned *m, int bfW, int X, int Y,
        int width, int height, unsigned fg) {
  int i, j, k, q, n;
  j = Y * bfW + X + width; k = (Y + height - 1) * bfW + X;
  for (i = Y * bfW + X; i < j; i++) {
    q = 0;
    for (n = 0; n < thick; n++) {
      m[i + q] = fg; m[k - q] = fg;
      q += bfW;
    }
    k++;
  }
  j = (Y + height - thick) * bfW + X;
  k = (Y + thick) * bfW + X + width - 1;
  for (i = (Y + thick) * bfW + X; i < j; i += bfW) {
    q = 0;
    for (n = 0; n < thick; n++) {
      m[i + q] = fg; m[k - q] = fg;
      q++;
    }
    k += bfW;
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
  bR = ((d & 0xff0000) >> 16); bG = ((d & 0xff00) >> 8);
  bB = (d & 0xff); fR = ((fg & 0xff0000) >> 16);
  fG = ((fg & 0xff00) >> 8); fB = (fg & 0xff);
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
  wl_surface_attach(surf, buff, 0, 0);
  for (int i = 0; i < rct.size(); i++) {
    wl_surface_damage(surf, rct[i].x, rct[i].y,
            rct[i].width, rct[i].height);
  }
  wl_surface_commit(surf);
  wl_display_flush(dis);
}

void launch_dialog() {
  xdg_toplevel_set_min_size(xdgtop, bufW, bufH);
  xdg_toplevel_set_max_size(xdgtop, bufW, bufH);
  thread t7(maind); t7.detach();
}

void set_cursor() {
  crsr_img = crsr->images[0];
  crsr_buff = wl_cursor_image_get_buffer(crsr_img);
  wl_pointer_set_cursor(pntr, pntr_serial, crsr_surf,
          crsr_img->hotspot_x, crsr_img->hotspot_y);
  wl_surface_attach(crsr_surf, crsr_buff, 0, 0);
  wl_surface_damage(crsr_surf, 0, 0, crsr_img->width, crsr_img->height);
  wl_surface_commit(crsr_surf);
}

void filesys_dialog() {
  redraw = false;
  int g, h, i = 0, j, k, m = 0, n, q, v;
  unsigned c, u;
  std::filesystem::path tp = strng.c_str();
  v = bufH - txtH - 4 * thick; j = bufH - v;
  fill_rectangle(p, bufW, 0, v, bufW, j, clrs[17 + darkmode]);
  rct.push_back({0, v, bufW, j});
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
    g = v + txtH * 1.4;
    draw_glyph(p, bufW, h, g, 168, c);
    h += txtW;
    draw_glyph(p, bufW, h, g, 168, c);
    h += txtW;
  }
  else { q = 0; share = n * txtW; }
  i = 0; m = 0; g = v + ascent;
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
  paint();
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

void scroll_page() {
  redraw = false;
  int i, j, k;
  fill_rectangle(p, bufW, 0, xtY - txtH, xtX, txtH, clrs[16 + darkmode]);
  if (locHist != -1) {
    j = bufH - txtH; k = txtW * detHist;
    fill_rectangle(p, bufW, 0, j, k, txtH, clrs[16 + darkmode]);
    rct.push_back({0, j, k, txtH});
    locHist = -1;
  }
  rct.push_back({vLeft, vPos, vWdth, vScrll});
  fill_rectangle(p, bufW, vLeft, vPos, vWdth, vScrll,
          clrs[16 + darkmode]);
  vscroll_draw();
  rct.push_back({vLeft, vPos, vWdth, vScrll});
  rct.push_back({0, 0, xtX, xtY});
  PgStart = vNdx * wide;
  std::thread t1(draw_page); t1.detach();
}

void clear_details() {
  redraw = false;
  int i, j, k, n;
  k = locHist;
  i = k % wide;
  j = k / wide;
  n = rBuf[locHist];
  if (i % 2 != j % 2) n += 256;
  i *= txtW; j *= txtH;
  place_tile(i, j, n);
  rct.push_back({i, j, txtW, txtH});
  locHist = -1;
  k = txtW * detHist;
  j = bufH - txtH;
  fill_rectangle(p, bufW, 0, j, k, txtH, clrs[16 + darkmode]);
  rct.push_back({0, j, k, txtH});
  paint();
}

void draw_details() {
  redraw = false;
  int i, j, k, c;
  long m, n;
  if (locHist != -1) {
    fill_rectangle(p, bufW, 0, bufH - txtH, txtW * detHist, txtH,
            clrs[16 + darkmode]);
    k = locHist;
    i = k % wide;
    j = k / wide;
    n = rBuf[locHist];
    if (i % 2 != j % 2) n += 256;
    i *= txtW; j *= txtH;
    place_tile(i, j, n);
    rct.push_back({i, j, txtW, txtH});
  }
  i = colm * txtW; j = row * txtH;
  draw_rectangle(p, bufW, i, j, txtW, txtH, clrs[darkmode]);
  rct.push_back({i, j, txtW, txtH});
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
  rct.push_back({0, bufH - txtH, txtW * i, txtH});
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
  rct.push_back({0, 0, bufW, bufH});
  thread t1(draw_page); t1.detach();
}

void dialog_return() {
  podup = false; popfocus = false;
  xdg_toplevel_set_min_size(xdgtop, maxTxtH * 12, maxTxtH * 8);
  xdg_toplevel_set_max_size(xdgtop, monW, monH);
  relaunch = false;
  if (mode == 1) {
    if (share == -1) mode = 0;
    else if (share == 1) {
      fclose(fbin);
      vfp = strng.c_str();
      fsz = std::filesystem::file_size(vfp);
      fbin = fopen(strng.c_str(), "rb");
      PgStart = 0; loc = 0;
      strng = vfp.filename();
      xdg_toplevel_set_title(xdgtop, strng.c_str());
      mode = 0;
    }
    else relaunch = true;
  }
  else if (mode == 100 || mode == 101 || mode == 201)
          mode = 0;
  else relaunch = true;
  zoom(false);
}

void top_config(void *data, xdg_toplevel *xdgtop, int width,
        int height, wl_array *states) {
  surfW = width; surfH = height;
}

void top_close(void *data, xdg_toplevel *xdgtop) {
  evloop = false;
}

void top_config_bounds(void *data, xdg_toplevel *xdgtop,
        int32_t width, int32_t height) {
  printf("received a top_config_bounds event\n");
}

void top_wm_capabils(void *data, xdg_toplevel *xdgtop,
        wl_array *caps) {
}

xdg_toplevel_listener top_lstnr = {
  .configure = top_config,
  .close = top_close,
  .configure_bounds = top_config_bounds,
  .wm_capabilities = top_wm_capabils,
};

void data_o_offer(void *data, wl_data_offer *data_offer,
        const char *mime_type) {
}

void data_o_source_actions(void *data, wl_data_offer *data_offer,
        unsigned source_actions) {
}

void data_o_action(void *data, wl_data_offer *data_offer,
        unsigned dnd_action) {
}

wl_data_offer_listener dat_offer_lstnr = {
  .offer = data_o_offer,
  .source_actions = data_o_source_actions,
  .action = data_o_action,
};

void dat_src_target(void *data, wl_data_source *data_source,
        const char *mime_type) {
}

void dat_src_send(void *data, wl_data_source *data_source,
        const char *mime_type, int32_t fd) {
  if (strcmp(mime_type, "UTF8_STRING") == 0) {
    write(fd, &clp[0], clp.size());
    close(fd);
  } else printf("Requesting a different mime_type -- %s\n", mime_type);
}

void dat_src_cancelled(void *data, wl_data_source *data_source) {
  select_own = false;
  wl_data_source_destroy(datsrc);
  datsrc = NULL;
}

wl_data_source_listener dat_src_lstnr = {
  .target = dat_src_target,
  .send = dat_src_send,
  .cancelled = dat_src_cancelled,
};

void receive_clpbrd() {
  redraw = false;
  int i;
  bool f = false;
  std::error_code ec;
  if (!select_own && (dta_offr != hst_offr)) {
                //Section: Setting up the pipe
    int fds[2];
    pipe(fds);
    wl_data_offer_receive(dta_offr, "text/plain", fds[1]);
    close(fds[1]);
    wl_display_roundtrip(dis);
                //Section: Getting the data
    unsigned char temp[4096];
    i = read(fds[0], temp, sizeof(temp));
    clp.assign(temp, &temp[i]);
    close(fds[0]);
    hst_offr = dta_offr;
                //Section: Testing validity
    if (clp.size() > 2000) f = true;
    else {
      if (std::filesystem::exists(clp)) {
        if (!std::filesystem::is_regular_file(clp) &&
                  !std::filesystem::is_directory(clp)) f = true;
      } else f = true;
    }
  }
  if (f) {
    hnd = strng; tmr = 1300; share = 3;
  }
  else if (std::filesystem::is_directory(clp)) {
    if (select_own) { share = 0; strng = clp; }
    else {
      std::filesystem::directory_iterator it(clp, ec);
      if (ec) {
        hnd = strng; tmr = 1300; share = 4;
      }
      else {
        if (clp[clp.size() - 1] != 47) clp.push_back(47);
        share = 0; strng = clp;
      }
    }
  }
  else if (std::filesystem::is_regular_file(clp)) {
    if (select_own) { share = 1; strng = clp; }
    else {
      int rc = ::access(clp.c_str(), R_OK);
      if (rc != 0) {
        hnd = strng; tmr = 1300; share = 5;
      }
      else { share = 1; strng = clp; }
    }
  }
  d_close(); thread t3(dialog_return); t3.detach();
}

void send_to_clpbrd() {
  redraw = false;
  if (select_own)
          wl_data_device_set_selection(datdvc, NULL, kbrd_serial);
  wl_display_roundtrip(dis);
  if (datsrc) { wl_data_source_destroy(datsrc); datsrc = NULL; }
  datsrc = wl_data_device_manager_create_data_source(datadman);
  wl_data_source_add_listener(datsrc, &dat_src_lstnr, NULL);
  wl_data_source_offer(datsrc, "UTF8_STRING");
  wl_data_device_set_selection(datdvc, datsrc, kbrd_serial);
  select_own = true;
  clp = strng;
  hnd = strng; tmr = 800; share = 6;
  d_close(); thread t3(dialog_return); t3.detach();
}

void datdev_data_offer(void *data, wl_data_device *data_device,
        wl_data_offer *id) {
  wl_data_offer_add_listener(id, &dat_offer_lstnr, NULL);
}

void datdev_selection(void *data, wl_data_device *data_device,
        wl_data_offer *id) {
  if (dta_offr) {
    wl_data_offer_destroy(dta_offr);
    dta_offr = NULL; hst_offr = NULL;
  }
  if (id) {
    dta_offr = id;
    clpbrd = true;
  } else {
    clpbrd = false;
  }
}

wl_data_device_listener dat_dev_lstnr = {
  .data_offer = datdev_data_offer,
  .selection = datdev_selection,
};


void kybrd_keymap(void *data, wl_keyboard *kybrd, unsigned format,
        int fd, unsigned size) {
}

void kybrd_enter(void *data, wl_keyboard *kybrd, unsigned serial,
        wl_surface *surf, wl_array *keys) {
  kbrd_serial = serial;
}

void kybrd_leave(void *data, wl_keyboard *kybrd, unsigned serial,
        wl_surface *surf) {
}

void kydnhandler() {
  lock_guard<std::mutex> lock(mtx);
  if (rky == 49 && txtH > 16) { txtH--; zoom(true); } // _N
  else if (rky == 50 && txtH < maxTxtH) { txtH++; zoom(true); } // _M
             //  _W   _PAGEUP  _KP9
  else if (rky == 17 || rky == 104 || rky == 73) {
    vNdx -= accelerate; if (vNdx < 0) vNdx = 0;
    scroll_page();
    accelerate *= 1.2;
    if (accelerate < 6) accelerate = 6;
  }
             //  _S  _PAGEDOWN  _KP3
  else if (rky == 31 || rky == 109 || rky == 81) {
    vNdx += accelerate; if (vNdx > vMax) vNdx = vMax;
    scroll_page();
    accelerate *= 1.2;
    if (accelerate < 6) accelerate = 6;
  }
             //  _LEFT  _KP4  _H
  else if ((rky == 105 || rky == 75 || rky == 35)
          && loc > 0) {
    loc--; colm = loc % wide; row = loc / wide;
    draw_details();
  }
             //  _RIGHT  _KP6  _L
  else if ((rky == 106 || rky == 77 || rky == 38)
          && loc < locMx) {
    loc++;
    if (loc + PgStart >= fsz) loc = fsz - PgStart - 1;
    colm = loc % wide; row = loc / wide;
    draw_details();
  }
             //  _UP  _KP8  _K
  else if (rky == 103 || rky == 72 || rky == 37) {
    if (row < 1 && vNdx > 0) { vNdx--; scroll_page(); }
    else if (loc >= wide) { loc -= wide ; colm = loc % wide;
            row = loc / wide; draw_details(); }
  }
             //  _DOWN  _KP2  _J
  else if (rky == 108 || rky == 80 || rky == 36) {
    if (row > deep - 2 && vNdx < vMax) { vNdx++; scroll_page(); }
    else if (loc <= locMx - wide && loc < fsz - PgStart - wide) {
      loc += wide ; colm = loc % wide; row = loc / wide;
      draw_details();
    }
  }
}

void kyrepeat() {
  kyrep = true;
  int i = 0;
  thread t8(kydnhandler); t8.detach();
  while (kyst == 1) {
    if (redraw && i == kydelay) {
            thread t8(kydnhandler); t8.detach(); break; }
    this_thread::sleep_for(chrono::milliseconds(kyrate));
    i++;
  }
  i = 0;
  while (kyst == 1) {
    this_thread::sleep_for(chrono::milliseconds(kyrate));
    i++;
    if (redraw && i % 2 == 0) { thread t8(kydnhandler); t8.detach(); }
  }
  kyrep = false;
}

void kybrd_key(void *data, wl_keyboard *kybrd, unsigned serial,
        unsigned time, unsigned key, unsigned state) {
  ky = key; kyst = state;
  if (key == 29) {
    if (state == 0) lftctl = false; else lftctl = true; //  _LEFTCTRL
  }
  if (podup) { thread t5(d_kbd_key); t5.detach(); return; }
  int i;
  bool f = false;
  lock_guard<std::mutex> lock(mtx);
  if (state == 0) {
    if (key == 16 || key == 1) { evloop = false; return; } // _Q  _ESC
    for (i = 0; i < 6; i++) if (key == kyu[i]) {
            accelerate = deep; return; }
    if (!redraw) return;
    if (key == 21) {
      if (darkmode) darkmode = 0; else darkmode = 18; // _Y
      tile_foundry();
      zoom(false);
    }
    else if (key == 23) { 
      if (wide < 40 || deep < 6) {
        mode = 100; tmr = 1200;    // _I
        strng = "Please enlarge window."; launch_dialog();
      }
      else {
        i = find_string(setts, "InfoMsg=", stsz) + 1;
        if (i == 0) {
          strng = "lpeek.conf|.lpeekrc is missing InfoMsg=";
          tmr = 2200; mode = 201; launch_dialog();
        }
        else {
          mode = 101;
          strng = getstr(&setts[i], stsz - i);
          launch_dialog();
        }
      }
    }
    else if (key == 33) {
      if (wide < 40 || deep < 6) {
        mode = 100; tmr = 1200; // _F
        strng = "Please enlarge window."; launch_dialog();
      }
      else { mode = 2; strng = vfp.c_str(); filesys_dialog(); }
    }
  }
  else {
    if (!redraw || mode != 0) return;
    for (i = 0; i < 20; i++) {
      if (key == kys[i]) { f = true; break; }
    }
    if (!kyrep && f) { rky = key; thread t7(kyrepeat); t7.detach(); }
  }
}

void kybrd_modifiers(void *data, wl_keyboard *kybrd, unsigned serial,
        unsigned mods_depressed, unsigned mods_latched,
        unsigned mods_locked, unsigned group) {
}

void kybrd_repeat(void *data, wl_keyboard *kybrd, int rate,
        int delay) {
  kyrate = 500 / rate;
  kydelay = delay / kyrate;
}

wl_keyboard_listener kybrd_lstnr = {
  .keymap = kybrd_keymap,
  .enter = kybrd_enter,
  .leave = kybrd_leave,
  .key = kybrd_key,
  .modifiers = kybrd_modifiers,
  .repeat_info = kybrd_repeat,
};

void pntr_enter(void *data, wl_pointer *pntr, unsigned serial,
        wl_surface *surface, wl_fixed_t surf_x, wl_fixed_t surf_y) {
  pntr_serial = serial;
  if (surface == d_surf) popfocus = true; else popfocus = false;
  crsr = wl_cursor_theme_get_cursor(crsr_theme, "left_ptr");
  set_cursor();
}

void pntr_leave(void *datia, wl_pointer *pntr, unsigned serial,
        wl_surface *surface) {
  if (surface == d_surf) {
    popfocus = false;
  }
}

void pntr_motion(void *data, wl_pointer *pntr, unsigned time,
        wl_fixed_t surf_x, wl_fixed_t surf_y) {
  int i, j, L;
  double d;
  bool tf;
  mX = wl_fixed_to_int(surf_x); mY = wl_fixed_to_int(surf_y);
  if (podup) { thread t2(d_pntr_motion); t2.detach(); return; }
  if (mode == 1) return;
  if (btnst) {
    if (vMax > 0 && mYhist != mY) {
      d = (mY - mYhist) / (double)vSlide;
      L = vNdx + d * (vMax + 1) + 0.5;
      if (L > vMax) L = vMax; 
      if (L < 0) L = 0;
      if (redraw && L != vNdx) {
        click = false; mYhist = mY; vNdx = L;
        thread t2(scroll_page); t2.detach();
      }
    }
  }
  else {
    if (!redraw) return;
    tf = false;
    if (mX > 0 && mX < xtX && mY > 0 && mY < xtY) {
      i = mX / txtW; j = mY / txtH;
      L = j * wide + i;
      if (L + PgStart < fsz) {
        if (L != locHist) {
          colm = i; row = j; loc = L;
          thread t5(draw_details); t5.detach();
        }
      }
      else if (locHist != -1) tf = true;
    }
    else if (locHist != -1) tf = true;
    if (tf) { thread t4(clear_details); t4.detach(); }
  }
}

void pntr_button(void *data, wl_pointer *pntr, unsigned serial,
        unsigned time, unsigned button, unsigned state) {
        // BTN_LEFT 272,  BTN_RIGHT 273,  BTN_MIDDLE 274
  btn = button; btnst = state;
  if (podup) { thread t4(d_pntr_button); t4.detach(); return; }
  if (state == 1) {
    if (button == 272) {
      click = true; mYhist = mY;
    }
    else if (button == 273) {
      if (!redraw) return;
      if (wide < 40 || deep < 6) {
        mode = 100; tmr = 1200; // _F
        strng = "Please enlarge window."; launch_dialog();
      }
      else { mode = 2; strng = vfp.c_str(); filesys_dialog(); }
    }
  }
  else {
    if (button == 272) {
      if (redraw && click) {
        if (mY < vPos) {
          vNdx -= deep; if (vNdx < 0) vNdx = 0;
          thread t2(scroll_page); t2.detach();
        }
        else if (mY > vPos + vScrll) {
          vNdx += deep; if (vNdx > vMax) vNdx = vMax;
          thread t2(scroll_page); t2.detach();
        }
      }
    }
  }
}

void pntr_axis(void *data, wl_pointer *pntr, unsigned time,
        unsigned axis, wl_fixed_t value) {
}

void pntr_frame(void *data, wl_pointer *pntr) {
}

void pntr_axis_source(void *data, wl_pointer *pntr,
        unsigned axis_source) {
}

//void pntr_axis_stop(void *data, wl_pointer *pntr, uint32_t time,
//        uint32_t axis) {
//}

//void pntr_axis_discrete(void *data, wl_pointer *pntr, uint32_t axis,
//        int32_t discrete) {
//}

void pntr_axis_value120(void *data, wl_pointer *pntr, unsigned axis,
        int value120) {
  wheel = value120 / 120;
  if (podup) { thread t9(d_pntr_wheel); t9.detach(); return; }
  if (!redraw || mode != 0) return;
  int i;
  if (lftctl) {
    i = txtH - wheel;
    if (i > maxTxtH) i = maxTxtH;
    if (i < 16) i = 16;
    if (txtH != i) { txtH = i; zoom(true); }
  }
  else {
    i = vNdx + wheel;
    if (i > vMax) i = vMax;
    if (i < 0) i = 0;
    if (vNdx != i) { vNdx = i; thread t2(scroll_page); t2.detach(); }
  }
}

void pntr_axis_rel_dir(void *data, wl_pointer *pntr, unsigned axis,
        unsigned direction) {
}

wl_pointer_listener pntr_lstnr = {
  .enter = pntr_enter,
  .leave = pntr_leave,
  .motion = pntr_motion,
  .button = pntr_button,
  .axis = pntr_axis,
  .frame = pntr_frame,
  .axis_source = pntr_axis_source,
//  .axis_stop = pntr_axis_stop,
//  .axis_discrete = pntr_axis_discrete,
  .axis_value120 = pntr_axis_value120,
  .axis_relative_direction = pntr_axis_rel_dir,
};

void buff_release(void *data, wl_buffer *wl_buffer) {
  rct.clear();
  redraw = true;
  if (mode == 2) {
    mode = 1; launch_dialog(); 
  }
  else if (relaunch) {
    if (mode == 1) {
      if (share == 0) { mode = 2; filesys_dialog(); }
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
    else if (mode > 199) { mode = 2; strng = hnd; filesys_dialog(); }
  }
}

wl_buffer_listener buff_listnr = {
  .release = buff_release,
};

void draw_resize() {
  redraw = false;
  bufW = surfW; bufH = surfH;
  if (buff) wl_buffer_destroy(buff); 
  buff = wl_shm_pool_create_buffer(pool, 0, bufW, bufH,
          bufW * 4, WL_SHM_FORMAT_XRGB8888);
  wl_buffer_add_listener(buff, &buff_listnr, NULL);
  bline = bufH - txtH + ascent;
  wide = (bufW - txtW) / txtW; deep = (bufH - txtH) / txtH;
  locMx = wide * deep - 1;
  if (loc > locMx) loc = locMx;
  if (loc > fsz - PgStart - 1) loc = fsz - PgStart - 1;
  accelerate = deep;
  xtX = wide * txtW; xtY = deep * txtH;
  zoom(false);
}

void xdgsurf_cnfgr(void *data, xdg_surface *xdgsurf, unsigned serial) {
  if (redraw && (surfW != bufW || surfH != bufH)) {
    thread t6(draw_resize); t6.detach();
    xdg_surface_ack_configure(xdgsurf, serial);
  }
}

xdg_surface_listener xdgsrf_lstnr = {
  .configure = xdgsurf_cnfgr,
};

void xdgwmb_ping(void *data, xdg_wm_base *xdgwmb, unsigned serial) {
  xdg_wm_base_pong(xdgwmb, serial);
}

xdg_wm_base_listener wmb_lstnr = {
  .ping = xdgwmb_ping,
};

void output_geometry(void* data, wl_output *wl_output,
        int x, int y, int physical_width,
        int physical_height, int subpixel, const char *make,
        const char *model, int transform) {
}

void output_mode(void *data, wl_output *wl_output,
        unsigned flags, int width, int height, int refresh) {
  monW = width; monH = height;
}

void output_done(void *data, wl_output *wl_output) {
}

void output_scale(void *data, wl_output *wl_output,
        int factor) {
}

void output_name(void *data, wl_output *wl_output,
        const char *name) {
}

void output_description(void *data,
        wl_output *wl_output, const char *description) {
}

wl_output_listener out_lstnr = {
  .geometry = output_geometry,
  .mode = output_mode,
  .done = output_done,
  .scale = output_scale,
  .name = output_name,
  .description = output_description,
};

void registry_global(void *data, wl_registry *regi,
        unsigned name, const char *interface, unsigned version) {
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    comp = (wl_compositor*)wl_registry_bind(
            regi, name, &wl_compositor_interface, version);
  }
  else if (strcmp(interface, wl_output_interface.name) == 0) {
    out = (wl_output*)wl_registry_bind(
            regi, name, &wl_output_interface, version);
  }
  else if (strcmp(interface, wl_shm_interface.name) == 0) {
    shm = (wl_shm*)wl_registry_bind(
            regi, name, &wl_shm_interface, version);
  }
  else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    xdgwmb = (xdg_wm_base*)wl_registry_bind(
            regi, name, &xdg_wm_base_interface, version);
  }
  else if (strcmp(interface, wl_seat_interface.name) == 0) {
    seat = (wl_seat*)wl_registry_bind(
            regi, name, &wl_seat_interface, version);
  }
  else if (strcmp(interface,
          wl_data_device_manager_interface.name) == 0) {
    datadman = (wl_data_device_manager*)wl_registry_bind(
            regi, name, &wl_data_device_manager_interface, version);
  }
}

void registry_global_remove(void *data,
        wl_registry *reg, unsigned name) {
  printf("Global Object listed as '%u' is removed.\n", name);
}

wl_registry_listener rgstry_lstnr = {
  .global = registry_global,
  .global_remove = registry_global_remove,
};

void init() {
  int i, j;
  bool f = false;
  string sl, sfn;
  rct.reserve(6);
  mode = 0;
                   //Section: Load settings
  save_xit = true; sl = ".lpeekrc";
  cfgp = filesystem::canonical("/proc/self/exe").parent_path()
          /".lpeekrc";
  if (!filesystem::exists(cfgp)) {
    cfgp = filesystem::path(std::getenv("HOME"))/".lpeekrc";
    if (!filesystem::exists(cfgp)) {
      printf(".lpeekrc is missing from your home dir\n");
      save_xit = false; sl = "lpeek.conf";
      cfgp = CONF_DIR"/lpeek.conf";
      if (!filesystem::exists(cfgp)) {
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
  if (i == -1) { printf("%s is missing MaxWdth=\n", sl.c_str()); return;}
  maxW = readval(&setts[i], 4, false);
  if (maxW < 300) maxW = 300;
  i = find_string(setts, "MaxHght=", stsz);
  if (i == -1) { printf("%s is missing MaxHght=\n", sl.c_str()); return;}
  maxH = readval(&setts[i], 4, false);
  if (maxH < 240) maxH = 240;
  i = find_string(setts, "MaxTxtHght=", stsz);
  if (i == -1) { printf("%s is missing MaxTxtHght=\n", sl.c_str());
          return; }
  maxTxtH = readval(&setts[i], 2, false);
  if (maxTxtH < 16 || maxTxtH > 67) maxTxtH = 67;
                           //Sub Section: Set up memory usage
  int fd = syscall(SYS_memfd_create, "shm-leon", 0);  //<sys/syscall.h>
  szM = monW * monH * 4;                         //<unistd.h>
  ftruncate(fd, szM);
  p = (unsigned*) mmap(NULL, szM, PROT_READ | PROT_WRITE, MAP_SHARED,
          fd, 0);                                     //<sys/mman.h>
  pool = wl_shm_create_pool(shm, fd, szM);
  close(fd);
  j = maxTxtH * 0.6 + 0.5;
  t = new unsigned [maxTxtH * j * 512];
                           //Sub Section: Get global variables
  i = find_string(setts, "Width=", stsz);
  if (i == -1) { printf("%s is missing Width=\n", sl.c_str()); return; }
  surfW = readval(&setts[i], 4, false);
  if (surfW < 300) surfW = 300;
  if (surfW > maxW) surfW = maxW;
  i = find_string(setts, "Height=", stsz);
  if (i == -1) { printf("%s is missing Height=\n", sl.c_str()); return; }
  surfH = readval(&setts[i], 4, false);
  if (surfH < 240) surfH = 240;
  if (surfH > maxH) surfH = maxH;
  i = find_string(setts, "TextHeight=", stsz);
  if (i == -1) { printf("%s is missing TextHeight=\n", sl.c_str());
          return; }
  txtH = readval(&setts[i], 2, false);
  if (txtH < 16) txtH = 16;
  if (txtH > maxTxtH) txtH = maxTxtH;
  i = find_string(setts, "DarkMode=", stsz);
  if (i == -1) { printf("%s is missing DarkMode=\n", sl.c_str());
          return; }
  darkmode = readval(&setts[i], 2, false);
  if (!(darkmode == 0 || darkmode == 18)) darkmode = 18;
  i = find_string(setts, "PgStart=", stsz);
  if (i == -1) { printf("%s is missing PgStart=\n", sl.c_str()); return;}
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
  if (j == -1) { printf("%s is missing Symbols=\n", sl.c_str()); return;}
  i = 0;
  while (i < 256) {
    symbol[i] =  readval(&setts[j], 4, true);
    i++; j += 5;
  }
                           //Sub Section: Prep font
  if (FT_Init_FreeType(&lbrry)) {
          printf("FreeType library did not load.\n"); return; }
  i = find_string(setts, "FontPath=", stsz) + 1;
  if (i == 0) { printf("%s is missing Fontpath=\n", sl.c_str()); return;}
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
  i = monW / 10 + 1; j = monH / 16 + 1;
  rBuf = new unsigned char[i * j];
  i = find_string(setts, "ViewFile=", stsz) + 1;
  if (i == 0) { printf("%s is missing ViewFile=\n", sl.c_str()); return;}
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
  sfn = vfp.filename();
  fsz = std::filesystem::file_size(vfp);
  fbin = fopen(vfp.c_str(), "rb");
  xdg_toplevel_set_max_size(xdgtop, monW, monH);
  xdg_toplevel_set_min_size(xdgtop, maxTxtH * 12, maxTxtH * 8);
  xdg_toplevel_set_title(xdgtop, sfn.c_str());
                   //Section: Make gamma correction table
  j = 0;
  double c, d;
  unsigned char cv[] = {5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20,
                        23, 27, 32, 39}; //curve
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
  evloop = true;
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
    if (j == -9999) {
            printf(".lpeekrc is corrupt. Not saved.\n"); return; }
    stsz += j;
  }
  fcfg = fopen(cfgp.c_str(), "wb");
  fwrite(setts, 1, stsz, fcfg);
  fclose(fcfg);
}

void cleanup() {
  this_thread::sleep_for(chrono::milliseconds(30));
  if (podup) { printf("closing podup\n"); d_close(); }
  if (save_xit) save_setts();
  if (t) delete[] t;
  if (rBuf) delete[] rBuf;
  if (setts) delete[] setts;
  if (fbin) fclose(fbin);
  if (hst_offr) wl_data_offer_destroy(hst_offr);
  if (datsrc) wl_data_source_destroy(datsrc);
  if (datdvc) wl_data_device_destroy(datdvc);
  if (kybrd) wl_keyboard_destroy(kybrd);
  if (pntr) wl_pointer_destroy(pntr);
  if (seat) wl_seat_destroy(seat);
  if (crsr_theme) wl_cursor_theme_destroy(crsr_theme);
  if (crsr_surf) wl_surface_destroy(crsr_surf);
  if (xdgtop) xdg_toplevel_destroy(xdgtop);
  if (xdgsurf) xdg_surface_destroy(xdgsurf);
  if (surf) wl_surface_destroy(surf);
  if (buff) wl_buffer_destroy(buff);
  if (xdgwmb) xdg_wm_base_destroy(xdgwmb);
  if (shm) wl_shm_destroy(shm);
  if (comp) wl_compositor_destroy(comp);
  if (out) wl_output_destroy(out);
  if (reg) wl_registry_destroy(reg);
  if (pool) wl_shm_pool_destroy(pool);
  if (p) munmap(p, szM);
  if (face) FT_Done_Face(face);
  if (lbrry) FT_Done_FreeType(lbrry);
  wl_display_disconnect(dis);
}

int main() {
  dis = wl_display_connect(NULL);
  reg = wl_display_get_registry(dis);
  wl_registry_add_listener(reg, &rgstry_lstnr, NULL);
  wl_display_roundtrip(dis);
                //Section: Prep Window
  wl_output_add_listener(out, &out_lstnr, NULL);
  surf = wl_compositor_create_surface(comp);
  xdg_wm_base_add_listener(xdgwmb, &wmb_lstnr, NULL);
  xdgsurf = xdg_wm_base_get_xdg_surface(xdgwmb, surf);
  xdg_surface_add_listener(xdgsurf, &xdgsrf_lstnr, NULL);
  xdgtop = xdg_surface_get_toplevel(xdgsurf);
  xdg_toplevel_add_listener(xdgtop, &top_lstnr, NULL);
  wl_display_roundtrip(dis);
  init();
  if (!evloop) { cleanup(); return 1; }
  wl_surface_commit(surf);
  pntr = wl_seat_get_pointer(seat);
  wl_pointer_add_listener(pntr, &pntr_lstnr, NULL);
  kybrd = wl_seat_get_keyboard(seat);
  wl_keyboard_add_listener(kybrd, &kybrd_lstnr, NULL);
  datdvc = wl_data_device_manager_get_data_device(datadman, seat);
  wl_data_device_add_listener(datdvc, &dat_dev_lstnr, NULL);
                //Section: Prep Cursors
  crsr_theme = wl_cursor_theme_load(nullptr, 24, shm);
  crsr_surf = wl_compositor_create_surface(comp);
                //Section: Show window
  while (wl_display_dispatch(dis) != -1) {
    if (!evloop) break;
  }
  cleanup();
  return 0;
}


