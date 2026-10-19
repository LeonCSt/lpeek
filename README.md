## **Lpeek** a GUI binary file viewer!

Quickly navigate through file system and view any regular file.

Use mouse or keyboard controls.

Suited to tiled or floating layout.

User has generous GUI options on element size, and window appearance.

So far, only an X11 version is available. Pending Wayland and Windows versions.

Manual install instruction provided. Plus the ability to carry a portable USB instance of the app.

Stand alone compilation without the need for themes or graphical support.

Fast response and scrolling but lightweight in size and cpu requirement.

Any comments, suggestions and criticisms welcome at https://github.com/LeonCSt/beginner_code/discussions

Regards ♡ ☺ ☮

## Information about Lpeek

  Action    |  Keyboard   |  Mouse
            |             |
Info        | I           |
Darkmode    | Y           |
FileSystem  | F           | RightClick
Scroll      | W/S PgUp/Dn | LeftDrag
Crawl       | K/J   Up/Dn | Wheel
Focus       | HJKL Arrows | Pointer
Select      | Enter Space | LeftClick
Zoom        | M  and  N   | Ctrl Wheel
Clipboard   | CtrlC CtrlV |
Back Dir    | Bckspc Left |
Exit        | Q  or  Esc  |

  FreeMono.otf is bundled with Lpeek. Most probably you already have that on your system. If so, in lpeek.conf, the FontPath can be set to an existing FreeMono.otf file (using sudo edit). Then you can delete the shipped FreeMono.otf which was placed in /usr/local/share/lpeek/ . You may try a different font file, but results are undefined.

  Copy lpeek.conf from /usr/local/etc/lpeek/ to your home directory, and rename to .lpeekrc . Change ownership so you own it.

  Lpeek re-opens to the same state as it was last closed. Lpeek reads all settings in .lpeekrc , but only writes to Width, Height, TextHeight, Darkmode, PgStart and ViewFile when closing. All the other settings can be edited and changes will persist.

  To copy the viewing filepath, just right click (to open filesystem dialog) then Ctrl C. To paste in a different filepath, just right click then Ctrl V.

  Lpeek can be optimized to use less buffer memory (for older systems), by altering the MaxWdht, MaxHght, and MaxTxtHght settings in lpeek.conf (and .lpeekrc). By default, buffer memory is set to the X display size - but is limited by these settings. Keep this in mind if you are using multiple screens, as the X display will encompass all of them, and may be extraordinarily large.

  When trouble-shooting, run /usr/local/bin/lpeek from a terminal and read the error messages. For elevated privileges, run with sudo from a terminal.

  For a portable app just copy the executable lpeek from /usr/local/bin/ , .lpeekrc from your home dir, lpeek_example.bin and FreeMono.otf from /usr/local/share/lpeek/ , all to the same directory on a USB. 

  Lpeek developer wishes to give credit and thanks to GNU-Fonts, the FreeType project, MIT-SHM, and the C++ group. Only English is presently supported. No AI vibe coding has been used. Lpeek is totally non invasive and simply views binary files.

  GNU License.

  lpeek developer: Leon Stanley.
  Australia 25th Sep 2026"

