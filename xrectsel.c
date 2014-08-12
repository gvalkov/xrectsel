// Author:  Georgi Valkov 
// Version: 1.1
// License: New BSD License
// URL:     https://github.com/gvalkov/xrectsel

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>


#define die(code, ...) do {fprintf(stderr, __VA_ARGS__); exit(code); } while (0)
long long _strtonum(const char*, long long, long long, int, const char **);

char* usage =
    "Usage: xrectsel [-hfwsbg]\n"
    "\n"
    "Options:\n"
    "  -h, --help           show this help message and exit\n"
    "  -f, --format         output format (default: %%x %%y %%w %%h)\n"
    "  -g, --grab           grab the x11 server (may prevent tearing)\n"
    "  -w, --border-width   set border width (default: 1)\n"
    "  -s, --border-style   set border line style (default: solid)\n"
    "  -b, --border-color   set border color (default: white)\n"
    "\n"
    "Color Format:\n"
    "  hex: #7CFC00\n"
    "  rgb: 127,252,0\n"
    "  x11: Lawn Green\n"
    "\n"
    "Styles:\n"
    "  border-style: solid dash double-dash\n"
    "\n"
    "Format Placeholders:\n"
    "  %%x %%X: offset from left/right of screen\n"
    "  %%y %%Y: offset from top/bottom of screen\n"
    "  %%w %%h: selection width/height\n"
    "\n"
    "Examples:\n"
    "  xrectsel -w 3 -b \"Lawn Green\"\n"
    "  xrectsel -f \"%%wx%%h+%%x+%%y\\n\"\n"
    "  xrectsel | read x y width height\n";

typedef struct xrectopts {
    XColor border_color;
    int border_width;
    int border_style;
    int show_help;
    int grab_server;
    const char* format;
} xrectopts;

XColor getcolor_hex(const char* colorstr) {
    if (strlen(colorstr) > 7)
        die(1, "error: invalid hex color \"%s\"\n", colorstr);

    const char* errstr;
    long num = _strtonum(&colorstr[1], 0, 16777216, 16, &errstr);
    if (errstr)
        die(1, "error: invalid hex color \"%s\" - %s\n", optarg, errstr);

    XColor color = {
        .red   = ((num >> 16) & 0xFF) * 257,
        .green = ((num >> 8) & 0xFF ) * 257,
        .blue  = ((num) & 0xFF      ) * 257,
        .flags = DoRed | DoGreen | DoBlue
    };
    return color;
}

XColor getcolor_rgb(const char* colorstr) {
    char *tk;

    short n = 0;
    unsigned short c[3] = {0};
    const char* errstr;

    char* running = strdup(colorstr);
    while (n < 3 && (tk = strsep(&running, ", ")) != NULL) {
        c[n] = (short) _strtonum(tk, 0, 255, 10, &errstr);
        if (errstr)
            die(1, "error: invalid rgb component \"%s\" - %s\n", tk, errstr);
        n++;
    }
    free(running);

    XColor color = {
        .red   = c[0] * 257,
        .green = c[1] * 257,
        .blue  = c[2] * 257,
        .flags = DoRed | DoGreen | DoBlue
    };
    return color;
}

XColor getcolor(Display* disp, Colormap cm, const char* colorstr) {
    XColor color, closest;
    Status ret;

    // hex color
    if (colorstr[0] == '#')
        color = getcolor_hex(colorstr);
    // rgb color
    else if (strchr(colorstr, ','))
        color = getcolor_rgb(colorstr);
    // possibly an X11 color
    else
        ret = XLookupColor(disp, cm, colorstr, &color, &closest);

    if (ret == False)
       die(1, "error: unknown color \"%s\"\n", colorstr);

    // printf("rgb: %d, %d, %d\n", color.red, color.green, color.blue);
    return color;
}

xrectopts parseopts(int argc, char** argv, Display* disp, Colormap cm) {
    char* short_opts = "hgw:b:s:f:";

    struct option long_opts[] = {
        {"help",   0, 0, 'h'},
        {"grab",   0, 0, 'g'},
        {"format", 1, 0, 'f'},
        {"border-width", 1, 0, 'w'},
        {"border-style", 1, 0, 's'},
        {"border-color", 1, 0, 'b'},
        {0, 0, 0, 0}
    };

    int idx = 0, c = 0;
    const char* errstr;

    // defaults
    xrectopts opts = {
        .border_style = LineSolid,
        .border_width = 1,
        .grab_server  = 0,
        .border_color = {
            .red = 65535, .green = 65535, .blue = 65535,
            .flags = DoRed | DoGreen | DoBlue
        },
        .format = "%x %y %w %h\n"
    };

    while ((c = getopt_long(argc, argv, short_opts, long_opts, &idx))) {
        if (c == -1)
            break;

        switch (c) {
        case 0:
            break;
        case 'h':
            die(0, usage);
            break;
        case 'g':
            opts.grab_server = 1;
            break;
        case 'w':
            opts.border_width = (int) _strtonum(optarg, 0, 10, 10, &errstr);
            if (errstr)
                die(1, "error: invalid border width \"%s\" - %s\n", optarg, errstr);
            break;
        case 's':
            if (strncmp(optarg, "solid", 6) == 0)
                opts.border_style = LineSolid;
            else if (strncmp(optarg, "dash", 4) == 0)
                opts.border_style = LineOnOffDash;
            else if (strncmp(optarg, "double-dash", 10) == 0)
                opts.border_style = LineDoubleDash;
            else
                die(1, "error: invalid line style \"%s\" - must be one of solid"
                       ", dash or double-dash\n", optarg);
            break;
        case 'b':
            opts.border_color = getcolor(disp, cm, optarg);
            break;
        case 'f':
            opts.format = optarg;
            break;
        default:
            exit(0);
            break;
        }
    }

    return opts;
}

void print_result(const char* fmt, int x, int y, int w, int h,
                  unsigned int lx, unsigned int ly) {
    flockfile(stdout);
    for (const char* s = fmt; *s != '\0'; ++s) {
        if (*s == '%') {
            switch (*++s) {
            case '%': putchar('%'); break;
            case 'x': printf("%d", x); break;
            case 'y': printf("%d", y); break;
            case 'w': printf("%d", w); break;
            case 'h': printf("%d", h); break;
            case 'X': printf("%u", lx); break;
            case 'Y': printf("%u", ly); break;
            }
        } else {
            putchar(*s);
        }
    }
    funlockfile(stdout);
}

int main(int argc, char** argv) {
    Display* disp = XOpenDisplay(NULL);
    Screen*  scr  = ScreenOfDisplay(disp, DefaultScreen(disp));
    Colormap cm   = DefaultColormap(disp, XScreenNumberOfScreen(scr));
    Window root   = RootWindow(disp, XScreenNumberOfScreen(scr));

    xrectopts opts = parseopts(argc, argv, disp, cm);

    int xfd = ConnectionNumber(disp);
    int fdsize = xfd + 1;

    int count = 0, done = 0, ret = 0;
    int rx = 0, ry = 0, btn_pressed = 0;
    int rect_x = 0, rect_y = 0, rect_w = 0, rect_h = 0;

    Cursor cursor    = XCreateFontCursor(disp, XC_crosshair);
    Cursor cursor_nw = XCreateFontCursor(disp, XC_ul_angle);
    Cursor cursor_ne = XCreateFontCursor(disp, XC_ur_angle);
    Cursor cursor_se = XCreateFontCursor(disp, XC_lr_angle);
    Cursor cursor_sw = XCreateFontCursor(disp, XC_ll_angle);

    XGCValues gcval;
    gcval.foreground = XWhitePixel(disp, 0);
    gcval.function   = GXxor;
    gcval.background = XBlackPixel(disp, 0);
    gcval.plane_mask = gcval.background ^ gcval.foreground;
    gcval.subwindow_mode = IncludeInferiors;

    GC gc = XCreateGC(disp, root,
                      GCFunction|GCForeground|GCBackground|GCSubwindowMode,
                      &gcval);

    XAllocColor(disp, cm, &opts.border_color);

    XSetForeground(disp, gc, opts.border_color.pixel);
    XSetLineAttributes(disp, gc, opts.border_width, opts.border_style, CapButt, JoinMiter);

    ret = XGrabPointer(
        disp, root, False,
        ButtonMotionMask | ButtonPressMask | ButtonReleaseMask,
        GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);

    if (ret != GrabSuccess)
        die(1, "error: couldn't grab pointer\n");

    if (opts.grab_server) {
        if (XGrabServer(disp) != True)
            die(1, "error: couldn't grab server\n");
    }

    ret = XGrabKeyboard(disp, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
    if (ret != GrabSuccess)
        die(1, "error: couldn't grab keyboard\n");

    XEvent ev;
    fd_set fdset;
    int grabmask = ButtonMotionMask | ButtonReleaseMask;
    while (1) {
        while (!done && XPending(disp)) {
            XNextEvent(disp, &ev);

            switch (ev.type) {
            case MotionNotify:
                if (btn_pressed) {
                    // re-draw the last rect to clear it
                    if (rect_w)
                        XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);

                    rect_x = rx;
                    rect_y = ry;
                    rect_w = ev.xmotion.x - rect_x;
                    rect_h = ev.xmotion.y - rect_y;

                    // Change the cursor to show we're selecting a region
                    if (rect_w < 0 && rect_h < 0)
                        XChangeActivePointerGrab(disp, grabmask, cursor_nw, CurrentTime);
                    else if (rect_w < 0 && rect_h > 0)
                        XChangeActivePointerGrab(disp, grabmask, cursor_sw, CurrentTime);
                    else if (rect_w > 0 && rect_h < 0)
                        XChangeActivePointerGrab(disp, grabmask, cursor_ne, CurrentTime);
                    else if (rect_w > 0 && rect_h > 0)
                        XChangeActivePointerGrab(disp, grabmask, cursor_se, CurrentTime);

                    if (rect_w < 0) {
                        rect_x += rect_w;
                        rect_w = 0 - rect_w;
                    }
                    if (rect_h < 0) {
                        rect_y += rect_h;
                        rect_h = 0 - rect_h;
                    }

                    // draw rectangle
                    XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
                    XFlush(disp);
                }
                break;
            case ButtonRelease:
                done = 1;
                break;
            case ButtonPress:
                btn_pressed = 1;
                rx = ev.xbutton.x;
                ry = ev.xbutton.y;
                break;
            case KeyPress:
                fprintf(stderr, "key pressed, aborting selection\n");
                done = 2;
                break;
            case KeyRelease:
                /* ignore */
                break;
            default:
                break;
            }
        }
        if (done)
            break;

        // now block some (but why?)
        FD_ZERO(&fdset);
        FD_SET(xfd, &fdset);
        errno = 0;
        count = select(fdsize, &fdset, NULL, NULL, NULL);
        if ((count < 0) && ((errno == ENOMEM) || (errno == EINVAL) || (errno == EBADF)))
            fprintf(stderr, "connection to X display lost\n");
    }

    Window root_win;
    unsigned int root_w = 0, root_h = 0, root_b, root_d;
    int root_x = 0, root_y = 0;

    ret = XGetGeometry(disp, root, &root_win, &root_x, &root_y,
                       &root_w, &root_h, &root_b, &root_d);
    if (ret == False)
        die(1, "error: failed to get root window geometry\n");

    unsigned int lx = root_w - rect_x - rect_w; // offset from right of screen
    unsigned int ly = root_h - rect_y - rect_h; // offset from bottom of screen

    if (rect_w) {
        XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
        XFlush(disp);
    }

    print_result(opts.format, rect_x, rect_y, rect_w, rect_h, lx, ly);

    XUngrabPointer(disp, CurrentTime);
    XUngrabKeyboard(disp, CurrentTime);
    XFreeCursor(disp, cursor);
    XFreeGC(disp, gc);
    XSync(disp, True);
    if (opts.grab_server)
        XUngrabServer(disp);
    XCloseDisplay(disp);

    return 0;
}
