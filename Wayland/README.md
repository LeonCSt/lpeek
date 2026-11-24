## **Lpeek** Manual install instructions


Copy or clone all the following files to one directory:-

Makefile

wpeek010.h

wpeek011.cpp

wpeek012.cpp

xdg-shell-protocol.cpp

xdg-shell-client-protocol.h

lpeek.conf

lpeek_example.bin

FreeMono.otf


## Then run:-
 
sudo make install


## To uninstall, run:-

sudo make clean

sudo make uninstall


# Additional notes about Wayland version of Lpeek

As yet there is no icon for the Wayland version of Lpeek.

xdg-shell-protocols.cpp | Needs to reside in the working directory, for compile. Initially generated with the wayland-scanner, then modified to suit g++ compilation. Either copy from this repository or make your own, by: -

    In the working directory : -

  wayland-scanner private-code \ < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \ > xdg-shell-protocol.cpp


MODIFY xdg-shell-protocol.cpp TO SUIT g++ COMPILER AS FOLLOWS:-

    1) Delete the first 52 lines e.g. all includes and externs

    2) Replace with #include <wayland-util.h>

        and         #include "xdg-shell-client-protocol.h"

    3) Remove all WL_PRIVATE prepends (all 5 of them).

   ** note **    xdg-shell-client-protocol.h   is also needed in the working directory for #include "" :-

  wayland-scanner client-header \ < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \ > xdg-shell-client-protocol.h

