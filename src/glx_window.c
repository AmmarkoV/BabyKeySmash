/** @file glx_window.c
 *  @brief Fullscreen (all monitors) kiosk-style GLX window for BabyKeySmash
 *         GL context creation adapted from
 *         opengl_depth_and_color_renderer/src/Library/System/glx3.c
 *  @author Ammar Qammaz (AmmarkoV)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include "glx_window.h"

#define NORMAL   "\033[0m"
#define RED      "\033[31m"

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

#define ESCAPE_HOLD_SECONDS_TO_QUIT 3.0

static Display   *display = 0;
static Window     win = 0;
static GLXContext ctx = 0;
static Colormap   cmap = 0;
static int        winWidth = 0;
static int        winHeight = 0;

static struct babyWindowCallbacks * cb = 0;

static double escHeldSince = 0.0;      /* 0.0 = escape not held */
static double lastScreenSaverPoke = 0.0;
static double lastRaise = 0.0;
static const char * exitReason = "still running";
static volatile int signalQuitRequested = 0;

static double getTimeSeconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000.0;
}

static void signalHandler(int sig)
{
  signalQuitRequested = 1;
}

static int ctxErrorOccurred = 0;
static int ctxErrorHandler( Display *dpy, XErrorEvent *ev )
{
    ctxErrorOccurred = 1;
    return 0;
}

// Helper to check for extension string presence, same as glx3.c
static int isExtensionSupported(const char *extList, const char *extension)
{
  const char *start;
  const char *where, *terminator;

  where = strchr(extension, ' ');
  if (where || *extension == '\0')
    return 0;

  for (start=extList;;) {
    where = strstr(start, extension);
    if (!where)
      break;
    terminator = where + strlen(extension);
    if ( where == start || *(where - 1) == ' ' )
      if ( *terminator == ' ' || *terminator == '\0' )
        return 1;
    start = terminator;
  }
  return 0;
}

static void hideCursor()
{
  //A 1x1 transparent pixmap turned into a cursor : the kid sees sprites , not a pointer
  static char noData[] = { 0,0,0,0,0,0,0,0 };
  XColor black;
  black.red = black.green = black.blue = 0;
  Pixmap bitmapNoData = XCreateBitmapFromData(display, win, noData, 8, 8);
  Cursor invisibleCursor = XCreatePixmapCursor(display, bitmapNoData, bitmapNoData, &black, &black, 0, 0);
  XDefineCursor(display, win, invisibleCursor);
  XFreeCursor(display, invisibleCursor);
  XFreePixmap(display, bitmapNoData);
}

static int grabInput()
{
  //Grabbing keyboard + pointer means the window manager / other clients never
  //see Alt+F4 , Alt+Tab , Super etc.  Retry a bit since another grab may be
  //momentarily active (e.g. the launching terminal)
  int i;
  int keyboardOk = 0 , pointerOk = 0;
  for (i=0; i<50; i++)
  {
    if (!keyboardOk)
      keyboardOk = ( XGrabKeyboard(display, win, True, GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess );
    if (!pointerOk)
      pointerOk  = ( XGrabPointer(display, win, True,
                                  PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
                                  GrabModeAsync, GrabModeAsync, win, None, CurrentTime) == GrabSuccess );
    if (keyboardOk && pointerOk) break;
    usleep(100*1000);
  }
  if (!keyboardOk) fprintf(stderr,RED "Could not grab keyboard , Alt+F4 etc. may leak to the window manager\n" NORMAL);
  if (!pointerOk)  fprintf(stderr,RED "Could not grab pointer\n" NORMAL);
  return (keyboardOk && pointerOk);
}

int babywin_open(struct babyWindowCallbacks * callbacks)
{
  cb = callbacks;

  signal(SIGINT ,signalHandler);
  signal(SIGTERM,signalHandler);

  display = XOpenDisplay(NULL);
  if (!display)
  {
    fprintf(stderr,"Failed to open X display\n");
    return 0;
  }

  int screen = DefaultScreen(display);
  //The X virtual screen spans all monitors ( XRandR merges them ) so a window
  //of this size extends to every visible monitor
  winWidth  = DisplayWidth(display,screen);
  winHeight = DisplayHeight(display,screen);
  fprintf(stderr,"Virtual screen covering all monitors is %ux%u\n",winWidth,winHeight);

  // Get a matching FB config , same attribs as glx3.c
  static int visual_attribs[] =
    {
      GLX_X_RENDERABLE    , True,
      GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
      GLX_RENDER_TYPE     , GLX_RGBA_BIT,
      GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
      GLX_RED_SIZE        , 8,
      GLX_GREEN_SIZE      , 8,
      GLX_BLUE_SIZE       , 8,
      GLX_ALPHA_SIZE      , 8,
      GLX_DEPTH_SIZE      , 24,
      GLX_STENCIL_SIZE    , 8,
      GLX_DOUBLEBUFFER    , True,
      None
    };

  int glx_major, glx_minor;
  if ( !glXQueryVersion( display, &glx_major, &glx_minor ) ||
       ( ( glx_major == 1 ) && ( glx_minor < 3 ) ) || ( glx_major < 1 ) )
  {
    fprintf(stderr,"Invalid GLX version\n");
    return 0;
  }

  int fbcount;
  GLXFBConfig* fbc = glXChooseFBConfig(display, screen, visual_attribs, &fbcount);
  if (!fbc)
  {
    fprintf(stderr,"Failed to retrieve a framebuffer config\n");
    return 0;
  }
  GLXFBConfig bestFbc = fbc[0];
  XFree( fbc );

  XVisualInfo *vi = glXGetVisualFromFBConfig( display, bestFbc );

  XSetWindowAttributes swa;
  swa.colormap = cmap = XCreateColormap( display, RootWindow( display, vi->screen ), vi->visual, AllocNone );
  swa.background_pixmap = None;
  swa.border_pixel      = 0;
  swa.event_mask        = KeyPressMask | KeyReleaseMask | PointerMotionMask |
                          ButtonPressMask | ButtonReleaseMask | ExposureMask | StructureNotifyMask;
  //override_redirect makes the window unmanaged : the window manager can not
  //close / minimize / alt-tab it , and it can span all monitors freely
  swa.override_redirect = True;

  win = XCreateWindow( display, RootWindow( display, vi->screen ),
                       0, 0, winWidth, winHeight, 0, vi->depth, InputOutput,
                       vi->visual,
                       CWBorderPixel|CWColormap|CWEventMask|CWOverrideRedirect, &swa );
  if ( !win )
  {
    fprintf(stderr,"Failed to create window\n");
    return 0;
  }

  XFree( vi );

  XStoreName( display, win, "BabyKeySmash" );
  XMapRaised( display, win );
  XSync(display,False);
  XSetInputFocus(display, win, RevertToParent, CurrentTime);

  //With detectable autorepeat , holding a key yields Press,Press,... and one
  //final Release , which makes the "hold Escape 3 seconds to quit" timer work
  XkbSetDetectableAutoRepeat(display, True, NULL);

  hideCursor();
  grabInput();

  // ---- GL 3.0 context creation , same flow as glx3.c ----
  const char *glxExts = glXQueryExtensionsString( display, screen );
  glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
           glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

  ctxErrorOccurred = 0;
  int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

  if ( !isExtensionSupported( glxExts, "GLX_ARB_create_context" ) || !glXCreateContextAttribsARB )
  {
    fprintf(stderr,"glXCreateContextAttribsARB() not found ... using old-style GLX context\n");
    ctx = glXCreateNewContext( display, bestFbc, GLX_RGBA_TYPE, 0, True );
  }
  else
  {
    int context_attribs[] =
      {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        None
      };
    ctx = glXCreateContextAttribsARB( display, bestFbc, 0, True, context_attribs );
    XSync( display, False );
    if ( ctxErrorOccurred || !ctx )
    {
      context_attribs[1] = 1;
      context_attribs[3] = 0;
      ctxErrorOccurred = 0;
      fprintf(stderr,"Failed to create GL 3.0 context ... using old-style GLX context\n");
      ctx = glXCreateContextAttribsARB( display, bestFbc, 0, True, context_attribs );
    }
  }

  XSync( display, False );
  XSetErrorHandler( oldHandler );

  if ( ctxErrorOccurred || !ctx )
  {
    fprintf(stderr,"Failed to create an OpenGL context\n");
    return 0;
  }

  glXMakeCurrent( display, win, ctx );
  fprintf(stderr,"BabyKeySmash GLX context ready..\n");
  return 1;
}

int babywin_width()  { return winWidth;  }
int babywin_height() { return winHeight; }

int babywin_processEvents()
{
  XEvent event;
  while (XPending(display))
  {
    XNextEvent(display, &event);
    switch (event.type)
    {
      case KeyPress:
      {
        KeySym keysym;
        char buffer[8];
        XLookupString((XKeyEvent *)&event,buffer,sizeof(buffer),&keysym,NULL);

        if (keysym==XK_Escape)
        {
          if (escHeldSince==0.0) { escHeldSince = getTimeSeconds(); }
        }
        else
        {
          //Backup parent combo : Ctrl+Shift+Q
          if ( ( (keysym==XK_q) || (keysym==XK_Q) ) &&
               (event.xkey.state & ControlMask) && (event.xkey.state & ShiftMask) )
                 { exitReason = "Ctrl+Shift+Q pressed"; return 0; }
          if (cb && cb->onKey) { cb->onKey(keysym); }
        }
        break;
      }
      case KeyRelease:
      {
        KeySym keysym = XkbKeycodeToKeysym(display, event.xkey.keycode, 0, 0);
        if (keysym==XK_Escape) { escHeldSince = 0.0; }
        break;
      }
      case MotionNotify:
        if (cb && cb->onMouseMove) { cb->onMouseMove(event.xmotion.x,event.xmotion.y); }
        break;
      case ButtonRelease:
      case ButtonPress:
        if (cb && cb->onButton)
          { cb->onButton(event.xbutton.button,(event.type==ButtonPress),event.xbutton.x,event.xbutton.y); }
        break;
      case Expose:
      case ConfigureNotify:
        break;
    }
  }

  double now = getTimeSeconds();

  if ( (escHeldSince!=0.0) && (now-escHeldSince > ESCAPE_HOLD_SECONDS_TO_QUIT) )
    {
      fprintf(stderr,"Escape held for %0.1f seconds , quitting..\n",ESCAPE_HOLD_SECONDS_TO_QUIT);
      exitReason = "Escape button held for 3 seconds";
      return 0;
    }

  if (signalQuitRequested)
    {
      fprintf(stderr,"Quit requested by signal..\n");
      exitReason = "terminated by signal ( SIGINT / SIGTERM )";
      return 0;
    }

  if (now-lastScreenSaverPoke > 50.0)
  {
    XResetScreenSaver(display);
    lastScreenSaverPoke = now;
  }

  //Desktop environments restack their windows above unmanaged
  //override-redirect windows , so keep raising ours back on top
  if (now-lastRaise > 1.0)
  {
    XRaiseWindow(display,win);
    lastRaise = now;
  }

  return 1;
}

const char * babywin_exitReason()
{
  return exitReason;
}

int babywin_swap()
{
  glXSwapBuffers(display, win);
  return 1;
}

int babywin_close()
{
  if (!display) { return 0; }
  XUngrabKeyboard(display,CurrentTime);
  XUngrabPointer(display,CurrentTime);
  glXMakeCurrent( display, 0, 0 );
  if (ctx) { glXDestroyContext( display, ctx ); }
  if (win) { XDestroyWindow( display, win ); }
  if (cmap) { XFreeColormap( display, cmap ); }
  XCloseDisplay( display );
  display=0;
  return 1;
}
