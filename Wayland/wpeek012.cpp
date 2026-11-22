/* wpeek012.cpp  |  Popups for a file viewer named lpeek  
       this file handles dialog popups and message boxes for lpeek.
       maind() can be called from anywhere in the parent wpeek011.cpp .
       Intended to be modal on the parent, but focus can move
         to other clients.
       Execution resumes in the DestroyNotify event back in
         wpeek011.cpp when this dialog closes.

    *needs*  wpeek010.h    | the header file for extern variables
             wpeek011.cpp  | the main file that calls this popup dialog

    *needs*  xdg-shell-protocol.cpp       | *see* README.md
             xdg-shell-client-protocol.h  |


    g++ -DCONF_DIR='""' -DSHARE_DIR='""' -I/usr/include/freetype2 xdg-shell-protocol.cpp wpeek011.cpp wpeek012.cpp -o lpeek -lwayland-client -lwayland-cursor -lfreetype
       Optionally, run with debug > >$ WAYLAND_DEBUG=1 ./lpeek
         or - - - -   prefix with the following for debug   - - - -
    g++ -fsanitize=address -fno-omit-frame-pointer -g -O1 . . .
    g++ -fsanitize=thread -g -O1 . . .
                                                           [261122]  */

#include"wpeek010.h"
using namespace std;

/* defined extern variables declared in lpeek010.h
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
        bool select_own, lftctl;
        atomic <bool> podup, popfocus;
        string strng, clp, hnd;
        atomic <unsigned> ky, kyst;
        atomic <unsigned> btn, btnst;
// end list of defined extern variables */

xdg_positioner *d_xdgposi;
xdg_surface *d_xdgsurf;
xdg_popup *d_xdgpop;
wl_shm_pool *d_pool;
wl_buffer *d_buff;
unsigned *d; // pixel array for dialog window
mutex d_mutex, mtxP;
vector <rect> drct;
int dbfW, dbfH, dYhst, fnW, d_size, dky;
int dvX, dvY, dvW, dvH, dvS, dvM, dvI;
int fov, num, Ldv, ndx, mxW, hlt, hltHist;
vector <int> ls;
bool hltA;
atomic <bool> pdraw, dclick, dkyrep;
vector <unsigned char> lf;
filesystem::path lpth;
string strD, hist;
unsigned dkys[] = {17, 31, 36, 37, 72, 73, 80, 81, 103, 104, 108, 109};


int str_places(std::string str) {
  int i = 0, j, k, m = 0;
  j = str.size();
  while (i < j) {
    if (str[i] < 128) k = 1;
    else if ((str[i] & 224) == 192) k = 2;
    else if ((str[i] & 240) == 224) k = 3;
    else if ((str[i] & 248) == 240) k = 4;
    else k = 1;
    if (str[i] > 31) m++;
    i += k;
  }
  return m;
}

std::string lower_case(std::string str) {
  int i, j, k;
  unsigned char rt[256];
  i = 0; j = str.size(); k = 0;
  while (i < j) {
    if (str[i] < 128) {
      if (!(str[i] == 46 && i == 0)) {
        if (str[i] > 64 && str[i] < 91) rt[k] = str[i] + 32;
        else rt[k] = str[i];
        k++;
      }
    }
    else { rt[k] = str[i]; k++; }
    i++;
  }
  std::string s(reinterpret_cast<const char*>(rt), k);
  return s;
}

void text_run(int X, int Y, int start, unsigned c) {
  int a, h, k, L, n, q, r;
  unsigned u;
  std::string str;
  if (lf[ls[start]] == 9) n = ls[start] + 1; else n = ls[start];
  h = ls[start + 1] - 1; q = h - n;
  a = q; r = 0; L = 0;
  if (q > mxW - 1) {
    str.assign(&lf[n], &lf[h]);
    q = str_places(str);
    if (q > mxW - 1) { a = (mxW - 4) / 2; r = q - mxW + 4 + a; }
  }
  while (n < h) {
    if (lf[n] < 128) { k = 1; u = lf[n]; }
    else if ((lf[n] & 224) == 192) {
      k = 2;
      u = ((lf[n] & 31) << 6) + (lf[n + 1] & 63);
    }
    else if ((lf[n] & 240) == 224) {
      k = 3;
      u = ((lf[n] & 15) << 12) + ((lf[n + 1] & 63) << 6)
              + (lf[n + 2] & 63);
    }
    else if ((lf[n] & 248) == 240) {
      k = 4;
      u = ((lf[n] & 7) << 18) + ((lf[n + 1] & 63) << 12)
              + ((lf[n + 2] & 63) << 6) + (lf[n + 3] & 63);
    }
    else { k = 1; u = 0; }
    if (L == a + 1) {
      draw_glyph(d, dbfW, X, Y + txtH * 0.6, 168, c);
      X += txtW;
      draw_glyph(d, dbfW, X, Y + txtH * 0.6, 168, c);
      X += txtW;
    }
    if (!(L > a && L < r)) {
      draw_glyph(d, dbfW, X, Y, u, c);
      X += txtW;
    }
    n += k; L++;
  }
}

void paintd() {
  wl_surface_attach(d_surf, d_buff, 0, 0);
  for (int i = 0; i < drct.size(); i++) {
    wl_surface_damage(d_surf, drct[i].x, drct[i].y,
            drct[i].width, drct[i].height);
  }
  wl_surface_commit(d_surf);
  wl_display_flush(dis);
}

void highlight() {
  pdraw = false;
  int i, j, k = dbfW - txtW - 2 * thick;
  unsigned c;
  if (hltHist != -1 && hltHist != fov + 1) {
    j = hltHist * txtH + thick;
    fill_rectangle(d, dbfW, thick, j, k, txtH, clrs[17 + darkmode]);
    if (hltHist == fov) {
      i = j + ascent - thick;
      draw_glyph(d, dbfW, thick, i, 171, clrs[11 + darkmode]);
      draw_glyph(d, dbfW, thick + txtW, i, 171, clrs[11 + darkmode]);
    }
    else {
      i = dvI + hltHist;
      if (i == ndx) if (darkmode) c = clrs[19]; else c = clrs[0];
      else if (i < Ldv) c = clrs[11 + darkmode];
      else c = clrs[3 + darkmode];
      if (lf[ls[i]] == 9) c = clrs[5 + darkmode];
      text_run(thick, j + ascent, i, c);
    }
    drct.push_back({thick, j, k, txtH});
  }
  if (hlt != -1 && hlt != fov + 1) {
    j = hlt * txtH + thick;
    fill_rectangle(d, dbfW, thick, j, k, txtH, clrs[16 + darkmode]);
    if (hlt == fov) {
      i = j + ascent - thick;
      draw_glyph(d, dbfW, thick, i, 171, clrs[11 + darkmode]);
      draw_glyph(d, dbfW, thick + txtW, i, 171, clrs[11 + darkmode]);
    }
    else {
      i = dvI + hlt;
      if (i == ndx) if (darkmode) c = clrs[19]; else c = clrs[0];
      else if (i < Ldv) c = clrs[11 + darkmode];
      else c = clrs[3 + darkmode];
      if (lf[ls[i]] == 9) c = clrs[5 + darkmode];
      text_run(thick, j + ascent, i, c);
    }
    drct.push_back({thick, j, k, txtH});
    hltA = true;
  }
  hltHist = hlt;
  paintd();
}

void draw_dvscroll() {
  int i, j, k, m, n, q;
  unsigned c = clrs[1 + darkmode];
  k = dvW;
  dvY = (double)dvI / dvM * dvS + 0.5;
  if (mode == 1) dvY += thick;
  else if (mode > 99) dvY += txtH * 0.5;
  q = (dvY + thick) * dbfW + dvX;
  m = q - dbfW;
  n = q + (dvH - 2 * thick) * dbfW;
  for (i = 0; i < thick; i++) {
    for (j = 0; j < k; j++) {
      d[m] = c; d[n] = c;
      m++; n++;
    }
    k -= 2;
    m -= dbfW + k + 1; n += dbfW - k - 1;
  }
  if (dvH > 2 * thick) fill_rectangle(d, dbfW, dvX, dvY + thick,
          dvW, dvH - 2 * thick, c); 
}

void draw_list() {
  pdraw = false;
  int i, j, k;
  unsigned c;
  j = (fov * txtH + thick) * dbfW;
  c = clrs[17 + darkmode];
  fill_rectangle(d, dbfW, thick, thick, dbfW - 2 * thick, fov * txtH, c);
  k = dvI + fov; j = ascent + thick;
  for (i = dvI; i < k; i++) {
    if (i == ndx) if (darkmode) c = clrs[19]; else c = clrs[0];
    else if (i < Ldv) c = clrs[11 + darkmode];
    else c = clrs[3 + darkmode];
    if (lf[ls[i]] == 9) c = clrs[5 + darkmode];
    text_run(thick, j, i, c);
    j += txtH;
  }
  if (dvM > 0) draw_dvscroll();
  drct.push_back({thick, thick, dbfW - 2 * thick, fov * txtH});
  hltA = false;
  paintd();
}

void listbox() {
  pdraw = false;
  int i, j;
  unsigned c;
  draw_rectangle(d, dbfW, 0, 0, dbfW, dbfH, clrs[11 + darkmode]);
  j = fov * txtH + thick;
  drct.push_back({0, j, dbfW, dbfH - j});
  fill_rectangle(d, dbfW, thick, j,
          dbfW - 2 * thick, txtH, clrs[17 + darkmode]);
  j += txtH;
  if (darkmode) c = clrs[0]; else c = clrs[18];
  fill_rectangle(d, dbfW, thick, j,
          dbfW - 2 * thick, dbfH - j - thick, c);
  j = (fov + 1) * txtH + ascent + thick;
  i = thick; j -= txtH + thick;
  draw_glyph(d, dbfW, i, j, 171, clrs[11 + darkmode]);
  i += txtW;
  draw_glyph(d, dbfW, i, j, 171, clrs[11 + darkmode]);
  if (darkmode) c = clrs[19]; else c = clrs[0];
  j = (fov + 1) * txtH + ascent + thick;
  if (ndx != -1) text_run(thick, j, ndx, c);
  if (num > 0) draw_list(); else paintd();
}

void list_metrics() {
  int g, i, j, k, n, q, u;
  bool f, h = false, d = false;
  std::string sm, st, lc, fn;
  std::error_code ec;
  int m[52];
  for (i = 0; i < 52; i++) m[i] = 0;
  strD = strng;
  i = strD.size() - 1;
  if (i != 0 && strD[i] == 47) strD.pop_back();
  lpth = strD.c_str();
  if (!std::filesystem::is_directory(lpth)) {
    fn = lpth.filename();
    lpth = lpth.parent_path();
    strD = lpth.c_str();
    h = true;
  }
  if (strD != hist) {
    hist = strD;
    lf.clear(); ls.clear();
    lf.resize(1024); ls.resize(256);
    num = 0; n = 0; fnW = 0; Ldv = 0;
    for (const std::filesystem::directory_entry& entry :
            std::filesystem::directory_iterator(lpth)) {
      if (entry.is_symlink()) continue;
      d = false;
      sm = entry.path().filename();
      j = sm.size(); f = false;
      if (entry.is_directory()) {
        std::filesystem::directory_iterator it(entry.path(), ec);
        if (ec) d = true;
        sm += "/"; j++; f = true;
      }
      else if(entry.is_regular_file()) {
        int rc = ::access(entry.path().c_str(), R_OK);
        if (rc != 0) d = true;
      }
      u = str_places(sm);
      if (u > fnW) fnW = u;
      lc = lower_case(sm);
      if (f) {
        u = lc[0] - 97;
        if (u < 0 || u > 25) u = 0;
        i = m[u]; k = Ldv; Ldv++;
        for (g = u + 1; g < 26; g++) m[g]++;
      }
      else {
        u = lc[0] - 71;
        if (u < 26 || u > 51) u = 26;
        i = Ldv + m[u]; k = num;
        for (g = u + 1; g < 52; g++) m[g]++;
      }
      for (i; i < k; i++) {
        if (lf[ls[i]] == 9) q = 1; else q = 0;
        st.assign(&lf[ls[i] + q], &lf[ls[i + 1] - 1]);
        st = lower_case(st);
        if (lc <= st) break;
      }
      j++;
      if (d) {sm = "\t" + sm; j++;}
      for (g = num; g >= i; g--) ls[g + 1] = ls[g] + j;
      for (g = n - 1; g >= ls[i]; g--) lf[g + j] = lf[g];
      u = ls[i] + j - 1; k = 0;
      for (g = ls[i]; g < u; g++) { lf[g] = sm[k]; k++; }
      lf[g] = 0; n += j; num++;
      if (n > lf.size() - 256) lf.resize(lf.size() + 1024);
      if (num > ls.size() - 2) ls.resize(ls.size() + 256);
    }
  }
  mxW = (bufW - share - 2 * thick) / txtW;
  i = mxW;
  if (fnW < i - 1) i = fnW + 1;
  if (i < 3) i = 3;
  dbfW = txtW * i;
  i = monW * monH / 2 / dbfW / txtH;
  k = (bufH - 6 * thick) / txtH;
  if (k > i - 1) k = i - 1;
  if (k > (num + 2)) k = num + 2;
  dbfH = txtH * k + 4 * thick;
  dvI = 0;
  if (num > 0) {
    fov = k - 2;
    dvM = num - fov;
    dvX = dbfW - txtW + 2 * thick;
    dvW = txtW - 4 * thick;
    if (dvM > 0) {
      dvH = (double)fov / (dvM + fov) * fov * txtH;
      if (dvH < 2 * thick) dvH = 2 * thick;
      dvS = fov * txtH - dvH;
    }
  } else { dvM = -1; fov = 0; }
  ndx = -1;
  if (h) {
    for (i = Ldv; i < num; i++) {
      st.assign(lf.begin() + ls[i], lf.begin() + ls[i + 1] - 1);
      if (st == fn) { ndx = i; break; }
    }
    if (dvM > 0) {
      dvI = ndx - int(fov / 2.0);
      if (dvI < 0) dvI = 0;
      if (dvI > dvM) dvI = dvM;
    }
  }
  hlt = -1; hltHist = -1;
}

void msgbox() {
  pdraw = false;
  int h = 0, i, j, k, m = 0, w;
  unsigned u;
  draw_rectangle(d, dbfW, 0, 0, dbfW, dbfH, clrs[11 + darkmode]);
  fill_rectangle(d, dbfW, thick, thick,
          dbfW - 2 * thick, dbfH - 2 * thick, clrs[17 + darkmode]);
  //j = dbfW * dbfH;
  //for (i = 0; i < j; i++) d[i] = clrs[17 + darkmode];
  h = dvI; m = txtH * 0.5 + ascent;
  i = ls[dvI]; j = dvI + fov;
  while (h < j) {
    w = txtW * 0.5;
    while (i < ls[h + 1]) {
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
      if (u > 32) draw_glyph(d, dbfW, w, m, u, clrs[4 + darkmode]);
      w += txtW;
      i += k;
    }
    h++; m += txtH;
  }
  if (dvM > 0) draw_dvscroll();
  drct.push_back({0, 0, dbfW, dbfH});
  paintd();
}

void msg_metrics() {
  int i, j, k, m = 0, s, w;
  ls.clear(); ls.resize(256); hist = "";
  w = (dbfW - txtW) / txtW;
  i = 0; j = strng.size(); num = 0;
  while (i < j) {
    if (strng[i] < 128) k = 1;
    else if ((strng[i] & 224) == 192) k = 2;
    else if ((strng[i] & 240) == 224) k = 3;
    else if ((strng[i] & 248) == 240) k = 4;
    else k = 1;
    if (strng[i] == 10) {
      ls[num + 1] = i + 1; s = i + 1;
      m = 0; num++;
    }
    else if (m == w) {
      if (strng[i] == 32) s = i + 1;
      else if (s == ls[num]) s = i;
      ls[num + 1] = s; num++;
      i = s - k; m = 0;
    }
    else if (strng[i] == 32) { s = i + 1; m++; }
    else if (strng[i] > 32) m++;
    i += k;
    if (i >= j) ls[num + 1] = j;
  }
  num++;
  dvM = num - fov;
  dvI = 0;
  if (dvM > 0) {
    dvH = (double)fov / (dvM + fov) * fov * txtH;
    if (dvH < 2 * thick) dvH = 2 * thick;
    dvS = fov * txtH - dvH;
  }
  dvX = dbfW - txtW + 2 * thick;
  dvW = txtW - 4 * thick;
}

void d_close() {
  lock_guard<mutex> lock(d_mutex);
  if (d_xdgpop) { xdg_popup_destroy(d_xdgpop); d_xdgpop = NULL; }
  if (d_xdgsurf) { xdg_surface_destroy(d_xdgsurf); d_xdgsurf = NULL; }
  if (d_surf) { wl_surface_destroy(d_surf); d_surf = NULL; }
  if (d_buff) { wl_buffer_destroy(d_buff); d_buff = NULL; }
  if (d_pool) { wl_shm_pool_destroy(d_pool); d_pool = NULL; }
  if (d) { munmap(d, d_size); d = NULL; }
  if (d_xdgposi) { xdg_positioner_destroy(d_xdgposi); d_xdgposi = NULL;}
}

void timer() {
  std::this_thread::sleep_for(std::chrono::milliseconds(tmr));
  d_close(); thread t3(dialog_return); t3.detach();
}

void d_pop_config(void *data, xdg_popup *xdg_popup, int x, int y,
        int width, int height) {
}

void d_pop_done(void *data, xdg_popup *xdgpop) {
  printf("d_pop_done\n");
  if (mode == 1) share = -1;
  d_close(); thread t3(dialog_return); t3.detach();
}

void d_pop_reposi(void *data, xdg_popup *xdgpop, unsigned token) {
}

xdg_popup_listener d_xdgpop_lstnr = {
  .configure = d_pop_config,
  .popup_done = d_pop_done,
  .repositioned = d_pop_reposi,
};

void d_kydn_hdl() {
  int i;
  lock_guard<mutex> lock(mtxP);
               //  _W   _PAGEUP  _KP9
  if ((dky == 17 || dky == 104 || dky == 73) && dvM > 0) {
    i = dvI - fov; if (i < 0) i = 0;
    if (i != dvI) {
      dvI = i;
      if (mode == 1) { std::thread dt1(draw_list); dt1.detach(); }
      else if (mode == 101) msgbox();
    }
  }
               //  _S  _PAGEDOWN  _KP3
  else if ((dky == 31 || dky == 109 || dky == 81) && dvM > 0) {
    i = dvI + fov; if (i > dvM) i = dvM;
    if (i != dvI) {
      dvI = i;
      if (mode == 1) { std::thread dt1(draw_list); dt1.detach(); }
      else if (mode == 101) msgbox();
    }
  }
               //  _UP  _KP8  _K
  else if (dky == 103 || dky == 72 || dky == 37) {
    if (mode == 1) {
      if (hlt == -1) {
              hlt = fov; std::thread dt2(highlight); dt2.detach(); }
      else if (hlt == 0 && dvM > 0 && dvI > 0) {
              dvI--; std::thread dt1(draw_list); dt1.detach(); } 
      else if (hlt > 0) {
              hlt--; std::thread dt2(highlight); dt2.detach(); }
    }
    else if (mode == 101 && dvM > 0 && dvI > 0) {
            dvI--; msgbox(); }
  }
               //  _DOWN  _KP2  _J
  else if (dky == 108 || dky == 80 || dky == 36) {
    if (mode == 1) {
      if (hlt == fov + 1) {
              hlt = fov; std::thread dt2(highlight); dt2.detach(); }
      else if (hlt == fov && dvM > 0 && dvI < dvM) {
              dvI++; std::thread dt1(draw_list); dt1.detach(); } 
      else if (hlt < fov) {
              hlt++; std::thread dt2(highlight); dt2.detach(); }
    }
    else if (mode == 101 && dvM > 0 && dvI < dvM) {
            dvI++; msgbox(); }
  }
}

void d_key_repeat() {
  dkyrep = true;
  int i = 0;
  thread t8(d_kydn_hdl); t8.detach();
  while (kyst == 1) {
    if (pdraw && i == kydelay) { thread t8(d_kydn_hdl); t8.detach(); break; }
    this_thread::sleep_for(chrono::milliseconds(kyrate));
    i++;
  }
  i = 0;
  while (kyst == 1) {
    this_thread::sleep_for(chrono::milliseconds(kyrate));
    i++;
    if (pdraw && i % 2 == 0) { thread t8(d_kydn_hdl); t8.detach(); }
  }
  dkyrep = false;
}

void d_kbd_key() {
  int i, j;
  bool f = false;
  lock_guard<mutex> lock(mtxP);
  if (kyst == 1) {
    if (!pdraw) return;
    if (mode != 1 && mode != 101) return;
    for (i = 0; i < 12; i++) {
      if (ky == dkys[i]) { f = true; break; }
    }
    if (!dkyrep && f) {
      dky = ky; thread t7(d_key_repeat); t7.detach(); return;
    }
    if (mode != 1) return;
                 //  _ENTER 28, _KPENTER 96, _SPACE 57
    if ((ky == 28 || ky == 96 || ky == 57) && hltA) {
      if (hlt < fov && lf[ls[hlt + dvI]] != 9) {
        if (strD != "/") strD.push_back(47);
        j = ls[hlt + dvI + 1] - 1;
        for (i = ls[hlt + dvI]; i < j; i++) strD.push_back(lf[i]);
        if (hlt + dvI < Ldv) share = 0; else share = 1;
        strng = strD;
        d_close(); thread t3(dialog_return); t3.detach();
      }
      else if (hlt == fov && strD.size() > 1) {
        lpth = lpth.parent_path();
        strD = lpth.c_str();
        if (strD != "/") strD.push_back(47);
        share = 0; strng = strD;
        d_close(); thread t3(dialog_return); t3.detach();
      }
    }
                 //  _BACKSPACE 14,  _LEFT 105,  _KP4 75
    else if ((ky == 14 || ky == 105 || ky == 75) && strD.size() > 1) {
      lpth = lpth.parent_path();
      strD = lpth.c_str();
      if (strD != "/") strD.push_back(47);
      share = 0; strng = strD;
      d_close(); thread t3(dialog_return); t3.detach();
    }
                 // leftCTL   _C 46,    _V 47,
    else if (lftctl && ky == 47) {
      if (!clpbrd) {
        hnd = strng; tmr = 1200; share = 2;
        d_close(); thread t3(dialog_return); t3.detach();
      }
      else receive_clpbrd();
    }
    else if (lftctl && ky == 46) send_to_clpbrd();
  }
  else {
    if (ky == 16 || ky == 1) {
      if (mode == 1) share = -1;
      d_close(); thread t3(dialog_return); t3.detach();  // _Q   _ESC
    }
  } 
}

void d_pntr_motion() {
  int i;
  double d;
  lock_guard<mutex> lock(mtxP);
  if (!pdraw) return;
  if (dclick) {
    i = mY - dYhst;
    if (i > 3 || i < -3) dclick = false;
  }
  if (popfocus) {
    if (mode == 1) {
      if (btnst) {
        if (dvM > 0 && !dclick) {
          d = (mY - dYhst) / (double)dvS;
          i = dvI + d * (dvM + 1) + 0.5;
          if (i > dvM) i = dvM;
          if (i < 0) i = 0;
          if (i != dvI) {
            dYhst = mY; dvI = i;
            std::thread dt1(draw_list); dt1.detach();
          }
        }
      }
      else {
        i = (mY - thick) / txtH;
        if (i > fov) i = -1;
        if (pdraw && hlt != i) {
                hlt = i; std::thread dt2(highlight); dt2.detach(); }
      }
    }
    else if (mode == 101 && btnst) {
      if (dvM > 0 && dYhst != mY) {
        d = (mY - dYhst) / (double)dvS;
        i = dvI + d * (dvM + 1) + 0.5;
        if (i > dvM) i = dvM;
        if (i < 0) i = 0;
        if (i != dvI) {
          dYhst = mY; dvI = i;
          std::thread dt1(msgbox); dt1.detach();
        }
      }
    }
  }
}

void d_pntr_button() {
  int i, j;
  lock_guard<mutex> lock(mtxP);
  if (!pdraw) return;
  if (btnst == 1) {
    if (btn == 272) {
      dclick = true; dYhst = mY;
      if (!popfocus) {
        if (mode == 1) share = -1;
        d_close(); thread t3(dialog_return); t3.detach(); return;
      }
    }
  }
  else {
    if (btn == 272) {
      if (mode == 1 && dclick && pdraw) {
        if (hlt > -1 && hlt < fov && lf[ls[hlt + dvI]] != 9) {
          if (strD != "/") strD.push_back(47);
          j = ls[hlt + dvI + 1] - 1;
          for (i = ls[hlt + dvI]; i < j; i++) strD.push_back(lf[i]);
          if (hlt + dvI < Ldv) share = 0; else share = 1;
          strng = strD;
          d_close(); thread t3(dialog_return); t3.detach();
        }
        else if(hlt == fov && strD.size() > 1) {
          lpth = lpth.parent_path();
          strD = lpth.c_str();
          if (strD != "/") strD.push_back(47);
          share = 0; strng = strD;
          d_close(); thread t3(dialog_return); t3.detach();
        }
      }
    }
  }
}

void d_pntr_wheel() {
  lock_guard<mutex> lock(mtxP);
  if (!pdraw) return;
  int i = dvI + wheel;
  if (i > dvM) i = dvM;
  if (i < 0) i = 0;
  if (i == dvI) return; 
  if (mode == 1) { dvI = i; draw_list(); }
  else if (mode == 101) { dvI = i; msgbox(); }
}

void d_buff_release(void *data, wl_buffer *wl_buffer) {
  drct.clear();
  pdraw = true;
}

wl_buffer_listener d_buff_listnr = {
  .release = d_buff_release,
};

void d_xdgsurf_cnfgr(void *data, xdg_surface *d_xdgsurf,
        unsigned serial) {
  xdg_surface_ack_configure(d_xdgsurf, serial);
}

xdg_surface_listener d_xdgsrf_lstnr = {
  .configure = d_xdgsurf_cnfgr,
};

void d_init() {
  int i, j;
  switch (mode) {
    case 1:
      list_metrics();
      i = share; j = bufH - dbfH - thick;
      break;
    case 100:
      dbfW = txtW * 8; dbfH = txtH * 4;
      fov = 3;
      msg_metrics();
      { std::thread dt3(timer); dt3.detach(); }
      break;
    case 101:
      dbfW = txtW * 40.5;
      j = monW * monH / 2 / dbfW / txtH;
      fov = (dbfW - txtH) / txtH;
      if (fov > j - 1) fov = j - 1;
      j = (bufH - txtH) / txtH;
      if (fov > j - 1) fov = j - 1;
      dbfH = txtH * fov;
      fov--; msg_metrics();
      break;
    case 200:
      dbfW = txtW * 20; dbfH = txtH * 2; break;
    case 201:
      dbfW = txtW * 40; dbfH = txtH * 2; break;
    case 202:
      dbfW = txtW * 34; dbfH = txtH * 2; break;
    case 203:
      dbfW = txtW * 28; dbfH = txtH * 2; break;
    case 204:
      dbfW = txtW * 23; dbfH = txtH * 2; break;
    case 205:
      dbfW = txtW * 16; dbfH = txtH * 2; break;
  }
  if (mode != 1) { i = (bufW - dbfW) / 2; j = (bufH - dbfH) / 2; }
  if (mode > 199) {
    fov = 1; msg_metrics();
    std::thread dt3(timer); dt3.detach();
  }
  d_xdgposi = xdg_wm_base_create_positioner(xdgwmb);
  xdg_positioner_set_size(d_xdgposi, dbfW, dbfH);
  xdg_positioner_set_anchor_rect(d_xdgposi, i, j, dbfW, dbfH);
}

void maind() {
  d_init();
  lock_guard<mutex> lock(d_mutex);
  d_surf = wl_compositor_create_surface(comp);
  d_xdgsurf = xdg_wm_base_get_xdg_surface(xdgwmb, d_surf);
  xdg_surface_add_listener(d_xdgsurf, &d_xdgsrf_lstnr, NULL);
  d_xdgpop = xdg_surface_get_popup(d_xdgsurf, xdgsurf, d_xdgposi);
  xdg_popup_add_listener(d_xdgpop, &d_xdgpop_lstnr, NULL);
  wl_surface_commit(d_surf);
                //Section: Prep Shared memory Buffer
  int d_fd = syscall(SYS_memfd_create, "shm-dial", 0); //<sys/syscall.h>
  d_size = dbfW * dbfH * 4;                                  //<unistd.h>
  ftruncate(d_fd, d_size);
  d = (unsigned*) mmap(NULL, d_size, PROT_READ | PROT_WRITE, MAP_SHARED,
          d_fd, 0);                                    //<sys/mman.h>
  d_pool = wl_shm_create_pool(shm, d_fd, d_size);
  close(d_fd);
  d_buff = wl_shm_pool_create_buffer(d_pool, 0, dbfW, dbfH,
          dbfW * 4, WL_SHM_FORMAT_XRGB8888);
  wl_buffer_add_listener(d_buff, &d_buff_listnr, NULL);
  drct.reserve(4);
  if (mode == 1) listbox();
  else if (mode > 99) msgbox();
  podup = true;
}


