/** @file glx_window.h
 *  @brief Fullscreen (all monitors) kiosk-style GLX window for BabyKeySmash
 *         Adapted from opengl_depth_and_color_renderer/src/Library/System/glx3.c
 *  @author Ammar Qammaz (AmmarkoV)
 */

#ifndef GLX_WINDOW_H_INCLUDED
#define GLX_WINDOW_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct babyWindowCallbacks
{
  void (*onKey)(unsigned long keysym);            /* called on every KeyPress            */
  void (*onMouseMove)(int x,int y);               /* called on pointer motion            */
  void (*onButton)(int button,int isDown,int x,int y); /* mouse buttons                  */
};

/* Opens an override-redirect window covering the whole X virtual screen
   (all monitors), grabs keyboard+pointer so Alt+F4 / Alt+Tab etc. never
   reach the window manager, hides the cursor and creates a GL 3.0 context.
   retval 1=Success 0=Failure */
int babywin_open(struct babyWindowCallbacks * callbacks);

int babywin_width();
int babywin_height();

/* Pump X events, feed callbacks, keep screensaver away.
   Returns 0 when the parent exit combo fired (Escape held >3s , Ctrl+Shift+Q
   or SIGINT/SIGTERM) , 1 otherwise */
int babywin_processEvents();

int babywin_swap();
int babywin_close();

#ifdef __cplusplus
}
#endif

#endif
