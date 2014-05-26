#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>


int main(int argc, char** argv) {
    Display* disp = XOpenDisplay(":0");
    Screen*  scr  = ScreenOfDisplay(disp, DefaultScreen(disp));

    int depth   = DefaultDepth(disp, XScreenNumberOfScreen(scr));
    Colormap cm = DefaultColormap(disp, XScreenNumberOfScreen(scr));
    Visual* vis = DefaultVisual(disp, XScreenNumberOfScreen(scr));
    Window root = RootWindow(disp, XScreenNumberOfScreen(scr));

    int count = 0, done = 0, ret = 0;

    int xfd = ConnectionNumber(disp);
    int fdsize = xfd + 1;

    int rx = 0, ry = 0, rw = 0, rh = 0, btn_pressed = 0;
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

    GC gc = XCreateGC(
        disp, root,
        GCFunction|GCForeground|GCBackground|GCSubwindowMode,
        &gcval);

    ret = XGrabPointer(
        disp, root, False,
        ButtonMotionMask | ButtonPressMask | ButtonReleaseMask,
        GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);

    if (ret != GrabSuccess)
        fprintf(stderr, "couldn't grab pointer:");

    ret = XGrabKeyboard(
        disp, root, False, GrabModeAsync, GrabModeAsync,
        CurrentTime);

    if (ret != GrabSuccess)
        fprintf(stderr, "couldn't grab keyboard:");

    XEvent ev;
    fd_set fdset;
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
                        XChangeActivePointerGrab(disp, ButtonMotionMask | ButtonReleaseMask,
                                                 cursor_nw, CurrentTime);
                    else if (rect_w < 0 && rect_h > 0)
                        XChangeActivePointerGrab(disp, ButtonMotionMask | ButtonReleaseMask,
                                                 cursor_sw, CurrentTime);
                    else if (rect_w > 0 && rect_h < 0)
                        XChangeActivePointerGrab(disp, ButtonMotionMask | ButtonReleaseMask,
                                                 cursor_ne, CurrentTime);
                    else if (rect_w > 0 && rect_h > 0)
                        XChangeActivePointerGrab(disp, ButtonMotionMask | ButtonReleaseMask,
                                                 cursor_se, CurrentTime);

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
                fprintf(stderr, "Key was pressed, aborting selection\n");
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

        // now block some
        FD_ZERO(&fdset);
        FD_SET(xfd, &fdset);
        errno = 0;
        count = select(fdsize, &fdset, NULL, NULL, NULL);
        if ((count < 0)
            && ((errno == ENOMEM) || (errno == EINVAL) || (errno == EBADF)))
            fprintf(stderr, "Connection to X display lost");
    }

    if (rect_w) {
        printf("%d %d %d %d\n", rect_x, rect_y, rect_w, rect_h);
        XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
        XFlush(disp);
    }

    XUngrabPointer(disp, CurrentTime);
    XUngrabKeyboard(disp, CurrentTime);
    XFreeCursor(disp, cursor);
    XFreeGC(disp, gc);
    XSync(disp, True);

    return 0;
}
