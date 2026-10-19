/* lpeek012.cpp  |  A file viewer  
       this file handles dialog popups and message boxes for lpeek.
       maind() can be called from anywhere in the parent lpeek011.cpp .
       Intended to be modal on the parent, but focus can move
         to other clients.
       Execution resumes in the DestroyNotify event back in
         lpeek011.cpp when this dialog closes.
       g++ -I/usr/include/freetype2 lpeek011.cpp lpeek012.cpp -o lpeek -lX11 -lXext -lfreetype
       - - - - - -   prefix with the following for debug   - - - - - -
    g++ -fsanitize=address -fno-omit-frame-pointer -g -O1 . . . 
    g++ -fsanitize=thread -g -O1 . . .

                                                              [260918]  */

#include"lpeek010.h"

/* defined extern variables declared in lpeek010.h
Display *dis;
GC gc;
Window win, sel_owner_win;
Atom WM_DELETE_WINDOW, uft8str;
Atom clpbrd, prop, trgts;
XShmSegmentInfo shminfo;
FT_Library lbrry;
FT_Face face;
unsigned *p;          
unsigned clrs[36];
int waW, waH;
int mode, bufW, bufH;
int txtW, txtH, thick;
int ascent, darkmode;
int share, tmr;
std::atomic <int> mode;
unsigned short symbol[256];
unsigned char gct[256];
bool select_own;
std::string strng, clp, hnd;
   end list of defined extern variables  */

Window dlg;
Pixmap dxmp;
std::vector <XRectangle> drct;
int dbfW, dbfH, dYhst, fnW;
int dvX, dvY, dvW, dvH, dvS, dvM, dvI;
int fov, num, Ldv, ndx, mxW, hlt, hltHist;
int operation; // 0 is none,  1 is gettargets(), 2 is getclipboard()
std::vector <int> ls;
bool paintflag, pointerdialog, dclick, lfdctl, hltA;
std::atomic <bool> dredraw, eventloop;
unsigned *dp;
std::vector <unsigned char> lf;
std::filesystem::path lpth;
std::string strD, hist;


void gettargets() {
  Atom type, *targets;
  int di;
  unsigned long natoms, dul;
  unsigned char *atom_ret = NULL;
  bool f = false;
  XGetWindowProperty(dis, dlg, prop, 0,
          1024 * sizeof (Atom), False, XA_ATOM,
          &type, &di, &natoms, &dul, &atom_ret);
  targets = (Atom *)atom_ret;
  operation = 3;
  for (int i = 0; i < natoms; i++) {
    if ( targets[i] == utf8str) f = true;
  }
  if (f) XConvertSelection(dis, clpbrd, utf8str, prop, dlg, CurrentTime);
  else XConvertSelection(dis, clpbrd, XA_STRING, prop, dlg, CurrentTime);
  XFree(atom_ret);
}

void getclipboard() {
  Atom actual_type;
  unsigned char *bytes;
  int actual_format;
  unsigned long nitems, bytes_after;
  bool f = false;
  std::error_code ec;
  std::filesystem::path tmp;
  if (select_own) nitems = clp.size();
  else {
    XGetWindowProperty(dis, dlg, prop, 0, LONG_MAX / 4,
            False, AnyPropertyType, &actual_type, &actual_format,
            &nitems, &bytes_after, &bytes);
  }
  if (nitems > 2000) f = true;
  else {
    if (!select_own) {
      clp.assign(bytes, &bytes[nitems]);
      XDeleteProperty(dis, dlg, prop);
      XFree(bytes);
    }
    tmp = clp;
    if (std::filesystem::exists(tmp)) {
      if (!std::filesystem::is_regular_file(tmp) &&
                !std::filesystem::is_directory(tmp)) f = true;
    } else f = true;
  }
  if (f) {
    hnd = strng; tmr = 1300; share = 3; eventloop = false;
  }
  else {
    if (std::filesystem::is_directory(tmp)) {
      if (select_own) { share = 0; strng = clp; eventloop = false; }
      else {
        std::filesystem::directory_iterator it(tmp, ec);
        if (ec) {
          hnd = strng; tmr = 1300; share = 4; eventloop = false;
        }
        else {
          if (clp[clp.size() - 1] != 47) clp.push_back(47);
          share = 0; strng = clp; eventloop = false;
        }
      }
    }
    else if (std::filesystem::is_regular_file(tmp)) {
      if (select_own) { share = 1; strng = clp; eventloop = false; }
      else {
        int rc = ::access(tmp.c_str(), R_OK);
        if (rc != 0) {
          hnd = strng; tmr = 1300; share = 5; eventloop = false;
        }
        else { share = 1; strng = clp; eventloop = false; }
      }
    }
  }
}

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
      draw_glyph(dp, dbfW, X, Y + txtH * 0.6, 168, c);
      X += txtW;
      draw_glyph(dp, dbfW, X, Y + txtH * 0.6, 168, c);
      X += txtW;
    }
    if (!(L > a && L < r)) {
      draw_glyph(dp, dbfW, X, Y, u, c);
      X += txtW;
    }
    n += k; L++;
  }
}

void paintd() {
  for (int i = 0; i < drct.size(); i++)
          XCopyArea(dis, dxmp, dlg, gc, drct[i].x, drct[i].y,
          drct[i].width, drct[i].height, drct[i].x, drct[i].y);
  XFlush(dis);
  drct.clear();
  dredraw = true;
}

void highlight() {
  dredraw = false;
  int i;
  unsigned c;
  XRectangle r;
  if (hltHist != -1 && hltHist != fov + 1) {
    r.x = thick, r.y = hltHist * txtH + thick;
    r.width = dbfW - txtW; r.height = txtH;
    fill_rectangle(dp, dbfW, r.x, r.y, r.width, r.height,
            clrs[17 + darkmode]);
    if (hltHist == fov) {
      i = r.y + ascent - thick;
      draw_glyph(dp, dbfW, thick, i, 171, clrs[11 + darkmode]);
      draw_glyph(dp, dbfW, thick + txtW, i, 171, clrs[11 + darkmode]);
    }
    else {
      i = dvI + hltHist;
      if (i == ndx) if (darkmode) c = clrs[19]; else c = clrs[0];
      else if (i < Ldv) c = clrs[11 + darkmode];
      else c = clrs[3 + darkmode];
      if (lf[ls[i]] == 9) c = clrs[5 + darkmode];
      text_run(thick, r.y + ascent, i, c);
    }
    drct.push_back(r);
  }
  if (hlt != -1 && hlt != fov + 1) {
    r.x = thick, r.y = hlt * txtH + thick;
    r.width = dbfW - txtW; r.height = txtH;
    fill_rectangle(dp, dbfW, r.x, r.y, r.width, r.height,
            clrs[16 + darkmode]);
    if (hlt == fov) {
      i = r.y + ascent - thick;
      draw_glyph(dp, dbfW, thick, i, 171, clrs[11 + darkmode]);
      draw_glyph(dp, dbfW, thick + txtW, i, 171, clrs[11 + darkmode]);
    }
    else {
      i = dvI + hlt;
      if (i == ndx) if (darkmode) c = clrs[19]; else c = clrs[0];
      else if (i < Ldv) c = clrs[11 + darkmode];
      else c = clrs[3 + darkmode];
      if (lf[ls[i]] == 9) c = clrs[5 + darkmode];
      text_run(thick, r.y + ascent, i, c);
    }
    drct.push_back(r); hltA = true;
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
      dp[m] = c; dp[n] = c;
      m++; n++;
    }
    k -= 2;
    m -= dbfW + k + 1; n += dbfW - k - 1;
  }
  if (dvH > 2 * thick) fill_rectangle(dp, dbfW, dvX, dvY + thick,
          dvW, dvH - 2 * thick, c); 
}

void draw_list() {
  dredraw = false;
  int i, j, k;
  unsigned c;
  XRectangle r;
  j = (fov * txtH + thick) * dbfW;
  c = clrs[17 + darkmode];
  for (i = 0; i < j; i++) dp[i] = c;
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
  r.x = 0; r.y = 0; r.width = dbfW; r.height = fov * txtH + thick;
  drct.push_back(r); hltA = false;
  paintd();
}

void listbox() {
  dredraw = false;
  int i, j;
  unsigned c;
  j = fov  * txtH + thick;
  fill_rectangle(dp, dbfW, 0, j, dbfW, txtH, clrs[17 + darkmode]);
  j += txtH;
  if (darkmode) c = clrs[0]; else c = clrs[18];
  fill_rectangle(dp, dbfW, 0, j, dbfW, dbfH - j, c);
  j = (fov + 1) * txtH + ascent + thick;
  i = thick; j -= txtH + thick;
  draw_glyph(dp, dbfW, i, j, 171, clrs[11 + darkmode]);
  i += txtW;
  draw_glyph(dp, dbfW, i, j, 171, clrs[11 + darkmode]);
  if (darkmode) c = clrs[19]; else c = clrs[0];
  j = (fov + 1) * txtH + ascent + thick;
  if (ndx != -1) text_run(thick, j, ndx, c);
  drct.resize(1);
  drct[0].x = 0; drct[0].y = fov * txtH + thick;
  drct[0].width = dbfW; drct[0].height = dbfH - drct[0].y;
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
  i = waW * waH / 2 / dbfW / txtH;
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
  hlt = -1; hltHist = -1;}

void msgbox() {
  dredraw = false;
  int h = 0, i, j, k, m = 0, w;
  unsigned u;
  j = dbfW * dbfH;
  for (i = 0; i < j; i++) dp[i] = clrs[17 + darkmode];
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
      if (u > 32) draw_glyph(dp, dbfW, w, m, u, clrs[4 + darkmode]);
      w += txtW;
      i += k;
    }
    h++; m += txtH;
  }
  if (dvM > 0) draw_dvscroll();
  drct.resize(1);
  drct[0].x = 0; drct[0].y = 0;
  drct[0].width = dbfW; drct[0].height = dbfH;
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

void close_down() {
  if (dxmp) XFreePixmap(dis, dxmp);
  if (dlg) XDestroyWindow(dis, dlg);
}

void timer() {
  std::this_thread::sleep_for(std::chrono::milliseconds(tmr));
  XEvent ev{};
  ev.type = ClientMessage;
  ev.xclient.window = win;
  ev.xclient.message_type = X_PROTOCOL;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = WM_DELETE_WINDOW;
  ev.xclient.data.l[1] = CurrentTime;
  XSendEvent(dis, win, False, NoEventMask, &ev);
  XFlush(dis);
}

void initd() {
  int i, j;
  switch (mode) {
    case 1:
      list_metrics();
      i = share; j = bufH - dbfH - thick;
      break;
    case 100:
      {
      dbfW = txtW * 8; dbfH = txtH * 4;
      fov = 3;
      msg_metrics();
      std::thread dt3(timer); dt3.detach();
      }
      break;
    case 101:
      dbfW = txtW * 40.5;
      j = waW * waH / 2 / dbfW / txtH;
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
  dlg = XCreateSimpleWindow(dis, win, i, j, dbfW, dbfH,
          1, 0xff88aaaa, 0xff221922);
  XSelectInput(dis, dlg, KeyPressMask | KeyReleaseMask | ButtonPressMask |
          ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
          PointerMotionMask | ExposureMask | StructureNotifyMask);
  XSetWMProtocols(dis, dlg, &WM_DELETE_WINDOW, 1);
                   //Section: Prep buffer
  j = bufW * bufH;
  dxmp = XShmCreatePixmap(dis, dlg, &shminfo.shmaddr[j * 4],
          &shminfo, dbfW, dbfH, 24);
  dp = &p[j];
  XMapWindow(dis, dlg);
  eventloop = true;
}

void maind() {
  initd();
  if (!eventloop) return;
  int i, j;
  int dmy;
  double d;
  KeySym keyd;
  char textd;
  bool dmsdn = false, f = false;
  pointerdialog = false;
  XSelectionRequestEvent *dsev;
  XSelectionEvent dssev;
  XEvent evntd;
  while(eventloop) {
    paintflag = false;
    XNextEvent(dis, &evntd);
    switch(evntd.type) {
      case KeyPress:
        XLookupString(&evntd.xkey, &textd, 1, &keyd, 0);
        if (keyd == XK_Control_L) lfdctl = true;
	if (!dredraw) break;
        if ((keyd == XK_w || keyd == XK_Prior || keyd == XK_KP_Prior)
                && dvM > 0) {
          i = dvI - fov; if (i < 0) i = 0;
	  if (i != dvI) {
            dvI = i;
            if (mode == 1) { std::thread dt1(draw_list); dt1.detach(); }
            else if (mode > 99 && mode < 200)
                    { std::thread dt1(msgbox); dt1.detach(); }
          }
	}
        else if ((keyd == XK_s || keyd == XK_Next || keyd == XK_KP_Next)
                && dvM > 0) {
          i = dvI + fov; if (i > dvM) i = dvM;
          if (i != dvI) {
          dvI = i;
            if (mode == 1) { std::thread dt1(draw_list); dt1.detach(); }
	    else if (mode > 99 && mode < 200)
                    { std::thread dt1(msgbox); dt1.detach(); }
          }
	}
	else if (keyd == XK_Up || keyd == XK_KP_Up || keyd == XK_k) {
          if (mode == 1) {
	    if (hlt == -1) {
                    hlt = fov; std::thread dt2(highlight); dt2.detach(); }
	    else if (hlt == 0 && dvM > 0 && dvI > 0) {
                    dvI--; std::thread dt1(draw_list); dt1.detach(); } 
 	    else if (hlt > 0) {
                    hlt--; std::thread dt2(highlight); dt2.detach(); }
	  }
	  else if (mode > 99 && mode < 200 && dvM > 0 && dvI > 0) {
                  dvI--; std::thread dt1(msgbox); dt1.detach(); }
	}
	else if (keyd == XK_Down || keyd == XK_KP_Down || keyd == XK_j) {
          if (mode == 1) {
	    if (hlt == fov + 1) {
                    hlt = fov; std::thread dt2(highlight); dt2.detach(); }
	    else if (hlt == fov && dvM > 0 && dvI < dvM) {
                    dvI++; std::thread dt1(draw_list); dt1.detach(); } 
 	    else if (hlt < fov) {
                    hlt++; std::thread dt2(highlight); dt2.detach(); }
	  }
	  else if (mode > 99 && mode < 200 && dvM > 0 && dvI < dvM) {
                  dvI++; std::thread dt1(msgbox); dt1.detach(); }
	}
	else if ((keyd == XK_space || keyd == XK_Return
                || keyd == XK_KP_Enter) && mode == 1 && hltA) {
          if (hlt < fov && lf[ls[hlt + dvI]] != 9) {
            if (strD != "/") strD.push_back(47);
            j = ls[hlt + dvI + 1] - 1;
            for (i = ls[hlt + dvI]; i < j; i++) strD.push_back(lf[i]);
            if (hlt + dvI < Ldv) share = 0; else share = 1;
            strng = strD;
            eventloop = false;
	  }
          else if(hlt == fov && strD.size() > 1) {
            lpth = lpth.parent_path();
	    strD = lpth.c_str();
	    if (strD != "/") strD.push_back(47);
	    share = 0; strng = strD;
	    eventloop = false;
      	  }
        }
	else if ((keyd == XK_BackSpace || keyd == XK_Left
                || keyd == XK_KP_Left) && mode == 1 && strD.size() > 1) {
          lpth = lpth.parent_path();
          strD = lpth.c_str();
          if (strD != "/") strD.push_back(47);
          share = 0; strng = strD;
          eventloop = false;
	}
        break;
      case KeyRelease:
        XLookupString(&evntd.xkey, &textd, 1, &keyd, 0);
        if (keyd == XK_Control_L) lfdctl = false;
	else if (keyd == XK_Escape || keyd == XK_q) {
          if (mode == 1) share = -1;
          eventloop = false;
	}
        else if (mode == 1 && dredraw && lfdctl && keyd == XK_v) {
          if (select_own) getclipboard();
	  else {
            operation = 2;
            XConvertSelection(dis, clpbrd, trgts, prop, dlg, CurrentTime);
	  }
        }
        else if (mode == 1 && lfdctl && keyd == XK_c) {
          operation = 1;
          XConvertSelection(dis, clpbrd, trgts, prop,
                  sel_owner_win, CurrentTime);
        }
        break;
      case ButtonPress:
        if (evntd.xbutton.button == 1) {
          dmsdn = true; dclick = true; dYhst = dmy;
        }
	if (evntd.xbutton.button == 4) {
	  if (mode == 1 && dredraw && dvI > 0) {
                  dvI--; std::thread dt1(draw_list); dt1.detach(); }
	  if (mode > 99 && mode < 200 && dredraw && dvI > 0) {
                  dvI--; std::thread dt1(msgbox); dt1.detach(); }
	}
	if (evntd.xbutton.button == 5) {
	  if (mode == 1 && dredraw && dvI < dvM) {
                  dvI++; std::thread dt1(draw_list); dt1.detach(); }
	  if (mode > 99 && mode < 200 && dredraw && dvI < dvM) {
                  dvI++; std::thread dt1(msgbox); dt1.detach(); }
	}
        break;
      case ButtonRelease:
        if (evntd.xbutton.button == 1) {
          dmsdn = false;
	  if (!pointerdialog) {
            if (dclick) {
              if (mode == 1) share = -1;
	      eventloop = false;
            }
	  }
	  else {
            if (mode == 1 && dclick && dredraw) {
              if (hlt < fov && lf[ls[hlt + dvI]] != 9) {
                if (strD != "/") strD.push_back(47);
                j = ls[hlt + dvI + 1] - 1;
                for (i = ls[hlt + dvI]; i < j; i++) strD.push_back(lf[i]);
                if (hlt + dvI < Ldv) share = 0; else share = 1;
		strng = strD;
                eventloop = false;
	      }
	      else if(hlt == fov && strD.size() > 1) {
                lpth = lpth.parent_path();
	        strD = lpth.c_str();
	        if (strD != "/") strD.push_back(47);
	        share = 0; strng = strD;
	        eventloop = false;
	      }
	    }
          }
	}
	else if (evntd.xbutton.button == 2 && mode == 1 && dredraw) {
          if (select_own) getclipboard();
          else {
            operation = 2;
            XConvertSelection(dis, clpbrd, trgts, prop, dlg,
                    CurrentTime);
          }
        }
	break;
      case EnterNotify:
        pointerdialog = true;
        break;
      case LeaveNotify:
	if (evntd.xcrossing.mode == NotifyNormal) {
          pointerdialog = false;
          if (!dmsdn && mode == 1 && dredraw && hltHist != -1) {
                  hlt = -1; std::thread dt2(highlight); dt2.detach(); }
        }
        break;
      case MotionNotify:
        dmy = evntd.xbutton.y;
	if (dclick) {
          i = dmy - dYhst;
          if (i > 3 || i < -3) dclick = false;
        }
        if (pointerdialog) {
          if (mode == 1) {
            if (dmsdn) {
  	      if (dvM > 0 && !dclick) {
	        d = (dmy - dYhst) / (double)dvS;
	        i = dvI + d * (dvM + 1) + 0.5;
	        if (i > dvM) i = dvM;
	        if (i < 0) i = 0;
	        if (dredraw && i != dvI) {
	          dYhst = dmy; dvI = i;
		  std::thread dt1(draw_list); dt1.detach();
	        }
	      }
	    }
	    else {
              i = (dmy - thick) / txtH;
	      if (dredraw && hlt != i) {
                      hlt = i; std::thread dt2(highlight); dt2.detach(); }
	    }
	  }
	  else if (mode > 99 && mode < 200 && dmsdn) {
  	    if (dvM > 0 && dYhst != dmy) {
	      d = (dmy - dYhst) / (double)dvS;
	      i = dvI + d * (dvM + 1) + 0.5;
              if (i > dvM) i = dvM;
	      if (i < 0) i = 0;
	      if (dredraw && i != dvI) {
	        dYhst = dmy; dvI = i;
		std::thread dt1(msgbox); dt1.detach();
	      }
	    }
	  }
        }
        break;
      case Expose:
        if (f) refresh();
        if (!f && mode == 1) { listbox(); f = true; break; }
        if (!f && mode > 99) { msgbox(); f = true; break; }
        paintflag = true;
	f = true;
        break;
      case ClientMessage:
        if ((Atom) evntd.xclient.data.l[0] == WM_DELETE_WINDOW) {
          if (mode == 1) share = -1;
          eventloop = false;
        }
        break;
      case SelectionClear:
        select_own = false;
        clp = "";
        break;
      case SelectionNotify:
        if (evntd.xselection.selection == clpbrd) {
          if (evntd.xselection.property == None && operation == 2) {
                  hnd = strng; tmr = 1000; share = 2; eventloop = false; }
          else if (operation == 2) gettargets();
          else if (operation == 3 && dredraw) {getclipboard(); operation = 0;}
	  else if (operation == 1) {
	    if (!select_own) {
              XSetSelectionOwner(dis, clpbrd, sel_owner_win, CurrentTime);
	    }
	    if (XGetSelectionOwner(dis, clpbrd) == sel_owner_win) {
              select_own = true;
              clp = strng;
              tmr = 600; share = 6; eventloop = false;
            } else printf("XGetSelectionOwner failed.");
	  operation = 0;
	  }
	}
        break;
      case SelectionRequest:
        dsev = (XSelectionRequestEvent*)&evntd.xselectionrequest;
        if (dsev->target == trgts) {
          Atom mytargets[] = {utf8str, XA_STRING};
          XChangeProperty(dis, dsev->requestor, dsev->property,
                  XA_ATOM, 32, PropModeReplace,
		  reinterpret_cast<unsigned char*>(mytargets), 2);
        }
	else {
          XChangeProperty(dis, dsev->requestor, dsev->property,
                  utf8str, 8,  PropModeReplace,
                  (unsigned char *)clp.c_str(), clp.size());
        }
        dssev.property = dsev->property;
        dssev.target = dsev->target;
        dssev.type = SelectionNotify;
        dssev.requestor = dsev->requestor;
        dssev.selection = dsev->selection;
        dssev.time = dsev->time;
        XSendEvent(dis, dsev->requestor, True, NoEventMask,
                (XEvent *)&dssev);
        break;
    }
    if (paintflag) {
      drct.resize(1);
      drct[0].x = 0; drct[0].y = 0;
      drct[0].width = dbfW; drct[0].height = dbfH;
      paintd();
    }
  }
  close_down();
}
