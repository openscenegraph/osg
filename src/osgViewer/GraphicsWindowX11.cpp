/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

/* Note, elements of GraphicsWindowX11 have used Prodcer/RenderSurface_X11.cpp as both
 * a guide to use of X11/GLX and copying directly in the case of setBorder().
 * These elements are licensed under OSGPL as above, with Copyright (C) 2001-2004  Don Burns.
 */

// TODO:
// implement http://www.opengl.org/registry/specs/OML/glx_swap_method.txt

#include <osgViewer/api/X11/GraphicsWindowX11>
#include <osgViewer/api/X11/PixelBufferX11>

#include <osg/DeleteHandler>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/Xmd.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>

#include <X11/Xmd.h>        /* For CARD16 */

#ifdef OSGVIEWER_USE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif

#ifdef OSGVIEWER_USE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include <unistd.h>
#include <cstdlib>

using namespace osgViewer;

#ifdef OSG_USE_EGL
bool checkEGLError(const char* str)
{
    EGLint err = eglGetError();
    if (err != EGL_SUCCESS)
    {
        OSG_WARN<<"Warning: "<<str<<" EGL error "<<std::hex<<err<<std::dec<<std::endl;
        return true;
    }
    else
    {
        // OSG_WARN<<"EGL reports no errors: "<<str<<std::endl;
        return false;
    }
}
#endif

class X11KeyboardMap
{
    public:

        X11KeyboardMap()
        {
#if 0
            _extendKeymap[8               ] = osgGA::GUIEventAdapter::KEY_BackSpace;
            _extendKeymap[127             ] = osgGA::GUIEventAdapter::KEY_Delete;
            _extendKeymap[27              ] = osgGA::GUIEventAdapter::KEY_Escape;
            // _extendKeymap[13              ] = osgGA::GUIEventAdapter::KEY_Enter;
#endif
            _extendedKeymap[XK_Escape       ] = osgGA::GUIEventAdapter::KEY_Escape;
            _extendedKeymap[XK_F1           ] = osgGA::GUIEventAdapter::KEY_F1;
            _extendedKeymap[XK_F2           ] = osgGA::GUIEventAdapter::KEY_F2;
            _extendedKeymap[XK_F3           ] = osgGA::GUIEventAdapter::KEY_F3;
            _extendedKeymap[XK_F4           ] = osgGA::GUIEventAdapter::KEY_F4;
            _extendedKeymap[XK_F5           ] = osgGA::GUIEventAdapter::KEY_F5;
            _extendedKeymap[XK_F6           ] = osgGA::GUIEventAdapter::KEY_F6;
            _extendedKeymap[XK_F7           ] = osgGA::GUIEventAdapter::KEY_F7;
            _extendedKeymap[XK_F8           ] = osgGA::GUIEventAdapter::KEY_F8;
            _extendedKeymap[XK_F9           ] = osgGA::GUIEventAdapter::KEY_F9;
            _extendedKeymap[XK_F10          ] = osgGA::GUIEventAdapter::KEY_F10;
            _extendedKeymap[XK_F11          ] = osgGA::GUIEventAdapter::KEY_F11;
            _extendedKeymap[XK_F12          ] = osgGA::GUIEventAdapter::KEY_F12;
            _extendedKeymap[XK_quoteleft    ] = '`';
            _extendedKeymap[XK_minus        ] = '-';
            _extendedKeymap[XK_equal        ] = '=';
            _extendedKeymap[XK_BackSpace    ] = osgGA::GUIEventAdapter::KEY_BackSpace;
            _extendedKeymap[XK_Tab          ] = osgGA::GUIEventAdapter::KEY_Tab;
            _extendedKeymap[XK_bracketleft  ] = '[';
            _extendedKeymap[XK_bracketright ] = ']';
            _extendedKeymap[XK_backslash    ] = '\\';
            _extendedKeymap[XK_Caps_Lock    ] = osgGA::GUIEventAdapter::KEY_Caps_Lock;
            _extendedKeymap[XK_semicolon    ] = ';';
            _extendedKeymap[XK_apostrophe   ] = '\'';
            _extendedKeymap[XK_Return       ] = osgGA::GUIEventAdapter::KEY_Return;
            _extendedKeymap[XK_comma        ] = ',';
            _extendedKeymap[XK_period       ] = '.';
            _extendedKeymap[XK_slash        ] = '/';
            _extendedKeymap[XK_space        ] = ' ';
            _extendedKeymap[XK_Shift_L      ] = osgGA::GUIEventAdapter::KEY_Shift_L;
            _extendedKeymap[XK_Shift_R      ] = osgGA::GUIEventAdapter::KEY_Shift_R;
            _extendedKeymap[XK_Control_L    ] = osgGA::GUIEventAdapter::KEY_Control_L;
            _extendedKeymap[XK_Control_R    ] = osgGA::GUIEventAdapter::KEY_Control_R;
            _extendedKeymap[XK_Meta_L       ] = osgGA::GUIEventAdapter::KEY_Meta_L;
            _extendedKeymap[XK_Meta_R       ] = osgGA::GUIEventAdapter::KEY_Meta_R;
            _extendedKeymap[XK_Alt_L        ] = osgGA::GUIEventAdapter::KEY_Alt_L;
            _extendedKeymap[XK_Alt_R        ] = osgGA::GUIEventAdapter::KEY_Alt_R;
            _extendedKeymap[XK_Super_L      ] = osgGA::GUIEventAdapter::KEY_Super_L;
            _extendedKeymap[XK_Super_R      ] = osgGA::GUIEventAdapter::KEY_Super_R;
            _extendedKeymap[XK_Hyper_L      ] = osgGA::GUIEventAdapter::KEY_Hyper_L;
            _extendedKeymap[XK_Hyper_R      ] = osgGA::GUIEventAdapter::KEY_Hyper_R;
            _extendedKeymap[XK_Menu         ] = osgGA::GUIEventAdapter::KEY_Menu;
            _extendedKeymap[XK_Print        ] = osgGA::GUIEventAdapter::KEY_Print;
            _extendedKeymap[XK_Scroll_Lock  ] = osgGA::GUIEventAdapter::KEY_Scroll_Lock;
            _extendedKeymap[XK_Pause        ] = osgGA::GUIEventAdapter::KEY_Pause;
            _extendedKeymap[XK_Home         ] = osgGA::GUIEventAdapter::KEY_Home;
            _extendedKeymap[XK_Page_Up      ] = osgGA::GUIEventAdapter::KEY_Page_Up;
            _extendedKeymap[XK_End          ] = osgGA::GUIEventAdapter::KEY_End;
            _extendedKeymap[XK_Page_Down    ] = osgGA::GUIEventAdapter::KEY_Page_Down;
            _extendedKeymap[XK_Delete       ] = osgGA::GUIEventAdapter::KEY_Delete;
            _extendedKeymap[XK_Insert       ] = osgGA::GUIEventAdapter::KEY_Insert;
            _extendedKeymap[XK_Left         ] = osgGA::GUIEventAdapter::KEY_Left;
            _extendedKeymap[XK_Up           ] = osgGA::GUIEventAdapter::KEY_Up;
            _extendedKeymap[XK_Right        ] = osgGA::GUIEventAdapter::KEY_Right;
            _extendedKeymap[XK_Down         ] = osgGA::GUIEventAdapter::KEY_Down;
            _extendedKeymap[XK_Num_Lock     ] = osgGA::GUIEventAdapter::KEY_Num_Lock;
            _extendedKeymap[XK_KP_Divide    ] = osgGA::GUIEventAdapter::KEY_KP_Divide;
            _extendedKeymap[XK_KP_Multiply  ] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
            _extendedKeymap[XK_KP_Subtract  ] = osgGA::GUIEventAdapter::KEY_KP_Subtract;
            _extendedKeymap[XK_KP_Add       ] = osgGA::GUIEventAdapter::KEY_KP_Add;
            _extendedKeymap[XK_KP_Home      ] = osgGA::GUIEventAdapter::KEY_KP_Home;
            _extendedKeymap[XK_KP_Up        ] = osgGA::GUIEventAdapter::KEY_KP_Up;
            _extendedKeymap[XK_KP_Page_Up   ] = osgGA::GUIEventAdapter::KEY_KP_Page_Up;
            _extendedKeymap[XK_KP_Left      ] = osgGA::GUIEventAdapter::KEY_KP_Left;
            _extendedKeymap[XK_KP_Begin     ] = osgGA::GUIEventAdapter::KEY_KP_Begin;
            _extendedKeymap[XK_KP_Right     ] = osgGA::GUIEventAdapter::KEY_KP_Right;
            _extendedKeymap[XK_KP_End       ] = osgGA::GUIEventAdapter::KEY_KP_End;
            _extendedKeymap[XK_KP_Down      ] = osgGA::GUIEventAdapter::KEY_KP_Down;
            _extendedKeymap[XK_KP_Page_Down ] = osgGA::GUIEventAdapter::KEY_KP_Page_Down;
            _extendedKeymap[XK_KP_Insert    ] = osgGA::GUIEventAdapter::KEY_KP_Insert;
            _extendedKeymap[XK_KP_Delete    ] = osgGA::GUIEventAdapter::KEY_KP_Delete;
            _extendedKeymap[XK_KP_Enter     ] = osgGA::GUIEventAdapter::KEY_KP_Enter;

            _standardKeymap[XK_1            ] = '1';
            _standardKeymap[XK_2            ] = '2';
            _standardKeymap[XK_3            ] = '3';
            _standardKeymap[XK_4            ] = '4';
            _standardKeymap[XK_5            ] = '5';
            _standardKeymap[XK_6            ] = '6';
            _standardKeymap[XK_7            ] = '7';
            _standardKeymap[XK_8            ] = '8';
            _standardKeymap[XK_9            ] = '9';
            _standardKeymap[XK_0            ] = '0';
            _standardKeymap[XK_A            ] = 'A';
            _standardKeymap[XK_B            ] = 'B';
            _standardKeymap[XK_C            ] = 'C';
            _standardKeymap[XK_D            ] = 'D';
            _standardKeymap[XK_E            ] = 'E';
            _standardKeymap[XK_F            ] = 'F';
            _standardKeymap[XK_G            ] = 'G';
            _standardKeymap[XK_H            ] = 'H';
            _standardKeymap[XK_I            ] = 'I';
            _standardKeymap[XK_J            ] = 'J';
            _standardKeymap[XK_K            ] = 'K';
            _standardKeymap[XK_L            ] = 'L';
            _standardKeymap[XK_M            ] = 'M';
            _standardKeymap[XK_N            ] = 'N';
            _standardKeymap[XK_O            ] = 'O';
            _standardKeymap[XK_P            ] = 'P';
            _standardKeymap[XK_Q            ] = 'Q';
            _standardKeymap[XK_R            ] = 'R';
            _standardKeymap[XK_S            ] = 'S';
            _standardKeymap[XK_T            ] = 'T';
            _standardKeymap[XK_U            ] = 'U';
            _standardKeymap[XK_V            ] = 'V';
            _standardKeymap[XK_W            ] = 'W';
            _standardKeymap[XK_X            ] = 'X';
            _standardKeymap[XK_Y            ] = 'Y';
            _standardKeymap[XK_Z            ] = 'Z';
            _standardKeymap[XK_a            ] = 'a';
            _standardKeymap[XK_b            ] = 'b';
            _standardKeymap[XK_c            ] = 'c';
            _standardKeymap[XK_d            ] = 'd';
            _standardKeymap[XK_e            ] = 'e';
            _standardKeymap[XK_f            ] = 'f';
            _standardKeymap[XK_g            ] = 'g';
            _standardKeymap[XK_h            ] = 'h';
            _standardKeymap[XK_i            ] = 'i';
            _standardKeymap[XK_j            ] = 'j';
            _standardKeymap[XK_k            ] = 'k';
            _standardKeymap[XK_l            ] = 'l';
            _standardKeymap[XK_m            ] = 'm';
            _standardKeymap[XK_n            ] = 'n';
            _standardKeymap[XK_o            ] = 'o';
            _standardKeymap[XK_p            ] = 'p';
            _standardKeymap[XK_q            ] = 'q';
            _standardKeymap[XK_r            ] = 'r';
            _standardKeymap[XK_s            ] = 's';
            _standardKeymap[XK_t            ] = 't';
            _standardKeymap[XK_u            ] = 'u';
            _standardKeymap[XK_v            ] = 'v';
            _standardKeymap[XK_w            ] = 'w';
            _standardKeymap[XK_x            ] = 'x';
            _standardKeymap[XK_y            ] = 'y';
            _standardKeymap[XK_z            ] = 'z';
        }

        ~X11KeyboardMap() {}

        int remapKey(int key)
        {
            KeyMap::iterator itr = _extendedKeymap.find(key);
            if (itr != _extendedKeymap.end()) return itr->second;

            itr = _standardKeymap.find(key);
            if (itr != _standardKeymap.end()) return itr->second;

            return key;
        }

        bool remapExtendedKey(int& key)
        {
            KeyMap::iterator itr = _extendedKeymap.find(key);
            if (itr != _extendedKeymap.end())
            {
                key = itr->second;
                return true;
            }
            else return false;
        }

    protected:

        typedef std::map<int, int> KeyMap;
        KeyMap _extendedKeymap;
        KeyMap _standardKeymap;
};

static bool remapExtendedX11Key(int& key)
{
    static X11KeyboardMap s_x11KeyboardMap;
    return s_x11KeyboardMap.remapExtendedKey(key);
}

// Functions to handle key maps of type char[32] as contained in
// an XKeymapEvent or returned by XQueryKeymap().
static inline bool keyMapGetKey(const char* map, unsigned int key)
{
    return (map[(key & 0xff) / 8] & (1 << (key & 7))) != 0;
}

static inline void keyMapSetKey(char* map, unsigned int key)
{
    map[(key & 0xff) / 8] |= (1 << (key & 7));
}

static inline void keyMapClearKey(char* map, unsigned int key)
{
    map[(key & 0xff) / 8] &= ~(1 << (key & 7));
}

GraphicsWindowX11::~GraphicsWindowX11()
{
    close(true);
}

Display* GraphicsWindowX11::getDisplayToUse() const
{
    if (_threadOfLastMakeCurrent==0)
    {
        return _display;
    }

    if (OpenThreads::Thread::CurrentThreadId()==_threadOfLastMakeCurrent)
    {
        return _display;
    }
    else
    {
        return _eventDisplay;
    }
}

bool GraphicsWindowX11::createVisualInfo()
{
    if (_visualInfo)
    {
    #ifdef OSG_USE_EGL
        delete _visualInfo;
    #else
        XFree(_visualInfo);
    #endif
        _visualInfo = 0;
    }

    if( _window != 0 )
    {
        XWindowAttributes watt;
        XGetWindowAttributes( _display, _window, &watt );
        XVisualInfo temp;
        temp.visualid = XVisualIDFromVisual(watt.visual);
        int n;
        _visualInfo = XGetVisualInfo( _display, VisualIDMask, &temp, &n );
    }
    else
    {
    #ifdef OSG_USE_EGL

        _visualInfo = new XVisualInfo;
        int depth = DefaultDepth( _display, _traits->screenNum );
        if (XMatchVisualInfo( _display, _traits->screenNum, depth, TrueColor, _visualInfo )==0)
        {
            OSG_NOTICE<<"GraphicsWindowX11::createVisualInfo() failed."<<std::endl;
            return false;
        }

    #else

        typedef std::vector<int> Attributes;
        Attributes attributes;

        attributes.push_back(GLX_RENDER_TYPE); attributes.push_back(GLX_RGBA_BIT);

        if (_traits->doubleBuffer) { attributes.push_back(GLX_DOUBLEBUFFER); attributes.push_back(True); }
        if (_traits->quadBufferStereo) { attributes.push_back(GLX_STEREO); attributes.push_back(True); }

        attributes.push_back(GLX_RED_SIZE); attributes.push_back(_traits->red);
        attributes.push_back(GLX_GREEN_SIZE); attributes.push_back(_traits->green);
        attributes.push_back(GLX_BLUE_SIZE); attributes.push_back(_traits->blue);
        attributes.push_back(GLX_DEPTH_SIZE); attributes.push_back(_traits->depth);

        if (_traits->alpha) { attributes.push_back(GLX_ALPHA_SIZE); attributes.push_back(_traits->alpha); }
        if (_traits->stencil) { attributes.push_back(GLX_STENCIL_SIZE); attributes.push_back(_traits->stencil); }

        #if defined(GLX_SAMPLE_BUFFERS) && defined (GLX_SAMPLES)
            if (_traits->sampleBuffers) { attributes.push_back(GLX_SAMPLE_BUFFERS); attributes.push_back(_traits->sampleBuffers); }
            if (_traits->samples) { attributes.push_back(GLX_SAMPLES); attributes.push_back(_traits->samples); }
        #endif
        // TODO
        //  GLX_AUX_BUFFERS
        //  GLX_ACCUM_RED_SIZE
        //  GLX_ACCUM_GREEN_SIZE

        attributes.push_back(None);

        int numFbConfigs = 0;
        GLXFBConfig* fbConfigs = glXChooseFBConfig(_display, _traits->screenNum, attributes.data(), &numFbConfigs);
        if (numFbConfigs > 0)
        {
            _fbConfig = fbConfigs[0];
        }

        XFree(fbConfigs);

        _visualInfo = glXGetVisualFromFBConfig(_display, _fbConfig);
    #endif
    }

    return _visualInfo != 0;
}

bool GraphicsWindowX11::checkAndSendEventFullScreenIfNeeded(Display* display, int x, int y, int width, int height, bool windowDecoration)
{
    osg::GraphicsContext::WindowingSystemInterface *wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (wsi == NULL)
    {
        OSG_NOTICE << "Error, no WindowSystemInterface available, cannot toggle window fullscreen." << std::endl;
        return false;
    }

    unsigned int    screenWidth;
    unsigned int    screenHeight;

    wsi->getScreenResolution(*_traits, screenWidth, screenHeight);
    bool isFullScreen = x == 0 && y == 0 && width == (int)screenWidth && height == (int)screenHeight && !windowDecoration;

    if (isFullScreen) {
        resized(x, y, width, height);
        getEventQueue()->windowResize(x, y, width, height, getEventQueue()->getTime());
    }

    Atom netWMStateAtom = XInternAtom(display, "_NET_WM_STATE", True);
    Atom netWMStateFullscreenAtom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", True);

    OSG_INFO<<"GraphicsWindowX11::checkAndSendEventFullScreenIfNeeded()"<<std::endl;

    if (netWMStateAtom != None && netWMStateFullscreenAtom != None)
    {
        // set up full screen
        {
            XEvent xev;
            xev.xclient.type = ClientMessage;
            xev.xclient.serial = 0;
            xev.xclient.send_event = True;
            xev.xclient.window = _window;
            xev.xclient.message_type = netWMStateAtom;
            xev.xclient.format = 32;
            xev.xclient.data.l[0] = isFullScreen ? 1 : 0;
            xev.xclient.data.l[1] = netWMStateFullscreenAtom;
            xev.xclient.data.l[2] = 0;

            XSendEvent(display, RootWindow(display, DefaultScreen(display)),
                        False,  SubstructureRedirectMask | SubstructureNotifyMask, &xev);
        }

#ifdef OSGVIEWER_USE_XINERAMA

        // Span all monitors
        if( isFullScreen && XineramaIsActive( display ) )
        {
            int numMonitors;
            XineramaScreenInfo* xi = XineramaQueryScreens( display, &numMonitors );
            enum {
                TopMost,
                BottomMost,
                LeftMost,
                RightMost
            };
            uint32_t span[4] = { 0, 0, 0, 0 };
            int32_t minx =  2147483647; // INT_MAX
            int32_t maxx = -2147483648; // INT_MIN
            int32_t miny =  2147483647; // INT_MAX
            int32_t maxy = -2147483648; // INT_MIN

            for( int i = 0; i < numMonitors; i++ )
            {
                if( xi[i].x_org < minx )
                {
                    span[LeftMost] = xi[i].screen_number;
                    minx = xi[i].x_org;
                }

                if( xi[i].x_org > maxx )
                {
                    span[RightMost] = xi[i].screen_number;
                    maxx = xi[i].x_org;
                }

                if( xi[i].y_org < miny )
                {
                    span[TopMost] = xi[i].screen_number;
                    miny = xi[i].y_org;
                }

                if( xi[i].y_org > maxy )
                {
                    span[BottomMost] = xi[i].screen_number;
                    maxy = xi[i].y_org;
                }
            }
            XFree(xi);

            Atom fullmons = XInternAtom(display, "_NET_WM_FULLSCREEN_MONITORS", True);
            if( fullmons != None )
            {
                XEvent xev;
                xev.type = ClientMessage;
                xev.xclient.window = _window;
                xev.xclient.message_type = fullmons;
                xev.xclient.format = 32;
                xev.xclient.data.l[0] = span[TopMost];
                xev.xclient.data.l[1] = span[BottomMost];
                xev.xclient.data.l[2] = span[LeftMost];
                xev.xclient.data.l[3] = span[RightMost];
                xev.xclient.data.l[4] = 0;

                XSendEvent( display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
            }
        }
#endif
        return true;
    }

    return false;
}

#define MWM_HINTS_FUNCTIONS   (1L << 0)
#define MWM_HINTS_DECORATIONS (1L << 1)
#define MWM_HINTS_INPUT_MODE  (1L << 2)
#define MWM_HINTS_STATUS      (1L << 3)

#define MWM_DECOR_ALL         (1L<<0)
#define MWM_DECOR_BORDER      (1L<<1)
#define MWM_DECOR_RESIZEH     (1L<<2)
#define MWM_DECOR_TITLE       (1L<<3)
#define MWM_DECOR_MENU        (1L<<4)
#define MWM_DECOR_MINIMIZE    (1L<<5)
#define MWM_DECOR_MAXIMIZE    (1L<<6)

#define MWM_FUNC_ALL          (1L<<0)
#define MWM_FUNC_RESIZE       (1L<<1)
#define MWM_FUNC_MOVE         (1L<<2)
#define MWM_FUNC_MINIMIZE     (1L<<3)
#define MWM_FUNC_MAXIMIZE     (1L<<4)
#define MWM_FUNC_CLOSE        (1L<<5)

bool GraphicsWindowX11::setWindowDecorationImplementation(bool flag)
{
    Display* display = getDisplayToUse();

    XMapWindow(display, _window );

    checkAndSendEventFullScreenIfNeeded(display, _traits->x, _traits->y, _traits->width, _traits->height, flag);
    struct
    {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long          inputMode;
        unsigned long status;
    } wmHints;

    Atom atom;
    bool result = false;
    if( (atom = XInternAtom( display, "_MOTIF_WM_HINTS", 0 )) != None )
    {

        if (flag)
        {
            wmHints.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS;
            wmHints.functions = MWM_FUNC_ALL;
            wmHints.decorations = MWM_DECOR_ALL;
            wmHints.inputMode = 0;
            wmHints.status = 0;

            // if traits says not resize we want to set the functions to exlude MWM_FUNC_RESIZE,
            // but this bitmask needs to be set if the MWM_FUNC_ALL bit is already set in order to toggle it off.
            if (_traits.valid() && !_traits->supportsResize) wmHints.functions = wmHints.functions | MWM_FUNC_RESIZE;

        }
        else
        {
            wmHints.flags = MWM_HINTS_DECORATIONS;
            wmHints.functions = 0;
            wmHints.decorations = 0;
            wmHints.inputMode = 0;
            wmHints.status = 0;
        }

        XChangeProperty( display, _window, atom, atom, 32, PropModeReplace, (unsigned char *)&wmHints,  5 );
        result = true;
    }
    else
    {
        OSG_NOTICE<<"Error: GraphicsWindowX11::setWindowDecorationImplementation(" << flag << ") - couldn't change decorations." << std::endl;
        result = false;
    }

    XFlush(display);
    XSync(display,0);
    // add usleep here to give window manager a chance to handle the request, if
    // we don't add this sleep then any X11 calls right afterwards can produce
    // X11 errors.
    usleep(100000);
    return result;
}

bool GraphicsWindowX11::setWindowRectangleImplementation(int x, int y, int width, int height)
{
    if (!_initialized) return false;

    Display* display = getDisplayToUse();

    checkAndSendEventFullScreenIfNeeded(display, x, y, width, height, _traits->windowDecoration);

    XMoveResizeWindow(display, _window, x, y, width, height);

    XFlush(display);
    XSync(display, 0);

    // add usleep here to give window manager a chance to handle the request, if
    // we don't add this sleep then any X11 calls right afterwards can produce
    // X11 errors.
    usleep(100000);


    return true;
}

void GraphicsWindowX11::setWindowName(const std::string& name)
{
    if( _window == 0) return;

    //    char *slist[] = { name.c_str(), 0L };
    //    XTextProperty xtp;

    //    XStringListToTextProperty( slist, 1, &xtp );

    Display* display = getDisplayToUse();
    if( !display ) return;

    //    XSetWMName( display, _window, &xtp );
    XStoreName( display, _window, name.c_str() );
    XSetIconName( display, _window, name.c_str() );

    XFlush(display);
    XSync(display,0);

    _traits->windowName = name;
}

void GraphicsWindowX11::setCursor(MouseCursor mouseCursor)
{
    Cursor newCursor = getOrCreateCursor(mouseCursor);
    if (newCursor == _currentCursor) return;

    _currentCursor = newCursor;
    if (!_window) return;
    Display* display = getDisplayToUse();
    if (!display) return;
    XDefineCursor( display, _window, _currentCursor );
    XFlush(display);
    XSync(display, 0);

    _traits->useCursor = (_currentCursor != getOrCreateCursor(NoCursor));
}

Cursor GraphicsWindowX11::getOrCreateCursor(MouseCursor mouseCursor)
{
    std::map<MouseCursor,Cursor>::iterator i = _mouseCursorMap.find(mouseCursor);
    if (i != _mouseCursorMap.end()) return i->second;

    Display* display = getDisplayToUse();
    if (!display) return None;

    switch (mouseCursor) {
    case NoCursor:
    {
        // create an empty mouse cursor, note that it is safe to destroy the Pixmap just past cursor creation
        // since the resource in the x server is reference counted.
        char buff[2] = {0,0};
        XColor ncol = {0,0,0,0,DoRed|DoGreen|DoBlue,0};
        Pixmap pixmap = XCreateBitmapFromData( display, _parent, buff, 1, 1);
        _mouseCursorMap[mouseCursor] = XCreatePixmapCursor( display, pixmap, pixmap, &ncol, &ncol, 0, 0 );
        XFreePixmap(display, pixmap);
        // Important to have the pixmap and the buffer still available when the request is sent to the server ...
        XFlush(display);
        XSync(display, 0);
        break;
    }
    case RightArrowCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_left_ptr );
        break;
    case LeftArrowCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_top_left_arrow );
        break;
    case InfoCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_hand1 );
        break;
    case DestroyCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_pirate );
        break;
    case HelpCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_question_arrow );
        break;
    case CycleCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_exchange );
        break;
    case SprayCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_spraycan );
        break;
    case WaitCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_watch );
        break;
    case TextCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_xterm );
        break;
    case CrosshairCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_crosshair );
        break;
    case UpDownCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_sb_v_double_arrow );
        break;
    case LeftRightCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_sb_h_double_arrow );
        break;
    case TopSideCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_top_side );
        break;
    case BottomSideCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_bottom_side );
        break;
    case LeftSideCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_left_side );
        break;
    case RightSideCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_right_side );
        break;
    case TopLeftCorner:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_top_left_corner );
        break;
    case TopRightCorner:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_top_right_corner );
        break;
    case BottomRightCorner:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_bottom_right_corner );
        break;
    case BottomLeftCorner:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_bottom_left_corner );
        break;
    case HandCursor:
        _mouseCursorMap[mouseCursor] = XCreateFontCursor( display, XC_hand1 );
        break;

    case InheritCursor:
    default:
        _mouseCursorMap[mouseCursor] = None;
        break;
    };
    return _mouseCursorMap[mouseCursor];
}

void GraphicsWindowX11::init()
{
    if (_initialized) return;

    if (!_traits)
    {
        _valid = false;
        return;
    }

    // getEventQueue()->setCurrentEventState(osgGA::GUIEventAdapter::getAccumulatedEventState().get());

    WindowData* inheritedWindowData = dynamic_cast<WindowData*>(_traits->inheritedWindowData.get());
    Window windowHandle = inheritedWindowData ? inheritedWindowData->_window : 0;

    _ownsWindow = windowHandle == 0;

    _display = XOpenDisplay(_traits->displayName().c_str());

    if (!_display)
    {
        OSG_NOTICE<<"Error: Unable to open display \"" << XDisplayName(_traits->displayName().c_str()) << "\"."<<std::endl;
        _valid = false;
        return;
    }

    #ifdef OSG_USE_EGL

        _eglDisplay = eglGetDisplay((EGLNativeDisplayType)_display);

        EGLint eglMajorVersion, eglMinorVersion;
        if (!eglInitialize(_eglDisplay, &eglMajorVersion, &eglMinorVersion))
        {
            OSG_NOTICE<<"GraphicsWindowX11::init() - eglInitialize() failed."<<std::endl;

            XCloseDisplay( _display );
            _display = 0;
            _valid = false;
            return;
        }

        OSG_NOTICE<<"GraphicsWindowX11::init() - eglInitialize() succeeded eglMajorVersion="<<eglMajorVersion<<" iMinorVersion="<<eglMinorVersion<<std::endl;

   #else
        // Query for GLX extension
        int errorBase, eventBase;
        if( glXQueryExtension( _display, &errorBase, &eventBase)  == False )
        {
            OSG_NOTICE<<"Error: " << XDisplayName(_traits->displayName().c_str()) <<" has no GLX extension." << std::endl;

            XCloseDisplay( _display );
            _display = 0;
            _valid = false;
            return;
        }
    #endif

    // OSG_NOTICE<<"GLX extension, errorBase="<<errorBase<<" eventBase="<<eventBase<<std::endl;

    if (!createVisualInfo())
    {
        _traits->red /= 2;
        _traits->green /= 2;
        _traits->blue /= 2;
        _traits->alpha /= 2;
        _traits->depth /= 2;

        OSG_INFO<<"Relaxing traits"<<std::endl;

        if (!createVisualInfo())
        {
            OSG_NOTICE<<"Error: Not able to create requested visual." << std::endl;
            XCloseDisplay( _display );
            _display = 0;
            _valid = false;
            return;
        }
    }

    // get any shared GLX contexts
    GraphicsHandleX11* graphicsHandleX11 = dynamic_cast<GraphicsHandleX11*>(_traits->sharedContext.get());
    Context sharedContext = graphicsHandleX11 ? graphicsHandleX11->getContext() : 0;

    #ifdef OSG_USE_EGL

        _valid = _ownsWindow ? createWindow() : setWindow(windowHandle);

        if (!_valid)
        {
            XCloseDisplay( _display );
            _display = 0;
            return;
        }

        OSG_NOTICE<<"GraphicsWindowX11::init() - window created ="<<_valid<<std::endl;

        eglBindAPI(EGL_OPENGL_ES_API);

        EGLConfig eglConfig = 0;

        #if defined(OSG_GLES3_AVAILABLE)

            #ifndef EGL_OPENGL_ES3_BIT
                #define EGL_OPENGL_ES3_BIT 0x00000040
            #endif
            #define OSG_EGL_OPENGL_TARGET_BIT EGL_OPENGL_ES3_BIT

        #elif defined(OSG_GLES2_AVAILABLE)
            #define OSG_EGL_OPENGL_TARGET_BIT EGL_OPENGL_ES2_BIT
        #else
            #define OSG_EGL_OPENGL_TARGET_BIT EGL_OPENGL_ES_BIT
        #endif

        typedef std::vector<EGLint> Attributes;
        Attributes attributes;

        attributes.push_back(EGL_RED_SIZE); attributes.push_back(_traits->red);
        attributes.push_back(EGL_GREEN_SIZE); attributes.push_back(_traits->green);
        attributes.push_back(EGL_BLUE_SIZE); attributes.push_back(_traits->blue);
        attributes.push_back(EGL_DEPTH_SIZE); attributes.push_back(_traits->depth);

        if (_traits->alpha) { attributes.push_back(EGL_ALPHA_SIZE); attributes.push_back(_traits->alpha); }
        if (_traits->stencil) { attributes.push_back(EGL_STENCIL_SIZE); attributes.push_back(_traits->stencil); }

        if (_traits->sampleBuffers) { attributes.push_back(EGL_SAMPLE_BUFFERS); attributes.push_back(_traits->sampleBuffers); }
        if (_traits->samples) { attributes.push_back(EGL_SAMPLES); attributes.push_back(_traits->samples); }

        attributes.push_back(EGL_RENDERABLE_TYPE); attributes.push_back(OSG_EGL_OPENGL_TARGET_BIT);

        attributes.push_back(EGL_NONE);
        attributes.push_back(EGL_NONE);

        int numConfigs;
        if (!eglChooseConfig(_eglDisplay, &(attributes.front()), &eglConfig, 1, &numConfigs) || (numConfigs != 1))
        {
            OSG_NOTICE<<"GraphicsWindowX11::init() - eglChooseConfig() failed."<<std::endl;
            XCloseDisplay( _display );
            _valid = false;
            _display = 0;
            return;
        }


        _eglSurface = eglCreateWindowSurface(_eglDisplay, eglConfig, (EGLNativeWindowType)_window, NULL);
        if (_eglSurface == EGL_NO_SURFACE)
        {
            OSG_NOTICE<<"GraphicsWindowX11::init() - eglCreateWindowSurface(..) failed."<<std::endl;
            XCloseDisplay( _display );
            _valid = false;
            _display = 0;
            return;
        }

        #if defined(OSG_GLES1_AVAILABLE)
            EGLint* contextAttribs = 0;
        #else
            EGLint contextAttribs[] = {
                 EGL_CONTEXT_CLIENT_VERSION,
                2,
                EGL_NONE
            };
        #endif

        _context = eglCreateContext(_eglDisplay, eglConfig, sharedContext, contextAttribs);
        if (_context == EGL_NO_CONTEXT)
        {
            OSG_NOTICE<<"GraphicsWindowX11::init() - eglCreateContext(..) failed."<<std::endl;
            XCloseDisplay( _display );
            _valid = false;
            _display = 0;
            return;
        }

        _initialized = true;

        checkEGLError("after eglCreateContext()");

    #else

        typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

        const char* glx_str = glXQueryExtensionsString(_display, _traits->screenNum);
        std::string glx_string; if (glx_str) glx_string = glx_str;

        glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
        if (glx_string.find("GLX_ARB_create_context")!=std::string::npos)
        {
            glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
        }

        bool supportsProfiles = glx_string.find("GLX_ARB_create_context")!=std::string::npos;


        if (glXCreateContextAttribsARB)
        {
            OSG_INFO << "Attempting to create GL context:" << std::endl;
            OSG_INFO << " * version: " << _traits->glContextVersion << std::endl;
            OSG_INFO << " * context flags: " << _traits->glContextFlags << std::endl;
            OSG_INFO << " * profile: " << _traits->glContextProfileMask << std::endl;

            std::vector<int> contextAttributes;

            std::size_t pos = _traits->glContextVersion.find(".");
            int majorVersion = std::atoi(_traits->glContextVersion.substr(0, pos).c_str());
            int minorVersion = pos == std::string::npos ? 0 : std::atoi(_traits->glContextVersion.substr(pos + 1).c_str());

            contextAttributes.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
            contextAttributes.push_back(majorVersion);
            contextAttributes.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
            contextAttributes.push_back(minorVersion);

            // If asking for OpenGL 3.2 or newer we should also specify a profile
            float glVersion = static_cast<float>(majorVersion)+static_cast<float>(minorVersion)*0.1f;
            if (glVersion>=3.2 && supportsProfiles && _traits->glContextProfileMask != 0)
            {
                contextAttributes.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
                contextAttributes.push_back(_traits->glContextProfileMask);
            }
            if (_traits->glContextFlags != 0)
            {
                contextAttributes.push_back(GLX_CONTEXT_FLAGS_ARB);
                contextAttributes.push_back(_traits->glContextFlags);
            }
            contextAttributes.push_back(None);

            _context = glXCreateContextAttribsARB( _display, _fbConfig, sharedContext, True, contextAttributes.data() );
        }
        else
        {
            _context = glXCreateContext( _display, _visualInfo, sharedContext, True );
        }

        if (!_context)
        {
            OSG_NOTICE<<"Error: Unable to create OpenGL graphics context."<<std::endl;
            XCloseDisplay( _display );
            _display = 0;
            _valid = false;
            return;
        }

        _initialized = _ownsWindow ? createWindow() : setWindow(windowHandle);
        _valid = _initialized;

    #endif

    if (_valid == false)
    {
        if (_display)
        {
            XCloseDisplay( _display );
            _display = 0;
        }

        if (_eventDisplay)
        {
            XCloseDisplay( _eventDisplay );
            _eventDisplay = 0;
        }
    }

    getEventQueue()->syncWindowRectangleWithGraphicsContext();
}

bool GraphicsWindowX11::createWindow()
{
    unsigned int screen = _traits->screenNum;

    _eventDisplay = XOpenDisplay(_traits->displayName().c_str());

    _parent = RootWindow( _display, screen );

    XWindowAttributes watt;
    XGetWindowAttributes( _display, _parent, &watt );
    // unsigned int parentWindowHeight = watt.height;

    XSetWindowAttributes swatt;
    swatt.colormap = XCreateColormap( _display, _parent, _visualInfo->visual, AllocNone);
    //swatt.colormap = DefaultColormap( _dpy, 10 );
    swatt.background_pixel = 0;
    swatt.border_pixel = 0;
    swatt.event_mask =  0;
    unsigned long mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;

    if (_traits->overrideRedirect)
    {
        swatt.override_redirect = true;
        mask |= CWOverrideRedirect;

        OSG_INFO<<"Setting override redirect"<<std::endl;
    }


    osg::GraphicsContext::WindowingSystemInterface *wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (wsi == NULL) {
    OSG_NOTICE << "Error, no WindowSystemInterface available, cannot toggle window fullscreen." << std::endl;
    return false;
    }

    int x = _traits->x;
    int y = _traits->y;
    int width = _traits->width;
    int height = _traits->height;


    unsigned int screenWidth;
    unsigned int screenHeight;
    wsi->getScreenResolution(*_traits, screenWidth, screenHeight);

    bool doFullSceenWorkAround = false;
    bool isFullScreen = x == 0 && y == 0 && width == (int)screenWidth && height == (int)screenHeight && !_traits->windowDecoration;
    if (isFullScreen && !_traits->overrideRedirect)
    {
        // follows is hack to get around problems with toggling off full screen with modern X11 window
        // managers that try to be too clever when toggling off full screen and ignore the window size
        // calls made by the OSG when the initial window size is full screen.

        Atom netWMStateAtom = XInternAtom(_display, "_NET_WM_STATE", True);
        Atom netWMStateFullscreenAtom = XInternAtom(_display, "_NET_WM_STATE_FULLSCREEN", True);

        // we have a modern X11 server so assume we need the do the full screen hack.
        if (netWMStateAtom != None && netWMStateFullscreenAtom != None)
        {
            // artificially reduce the initial window size so that the windowing
            // system has a size to go back to when toggling off full screen,
            // we don't have to worry about the window being initially smaller as the
            // setWindowDecoration(..) implementation with enable full screen for us
            x = width/4;
            y = height/4;
            width /= 2;
            height /= 2;

            doFullSceenWorkAround = true;
        }
    }

    _window = XCreateWindow( _display, _parent,
                             x,
                             y,
                             width, height, 0,
                             _visualInfo->depth, InputOutput,
                             _visualInfo->visual, mask, &swatt );

    if (!_window)
    {
        OSG_NOTICE<<"Error: Unable to create Window."<<std::endl;
        _context = 0;
        return false;
    }


    // Give window a class so that user preferences can be saved in the resource database.
    XClassHint clH;
    clH.res_name = (char *)"OSG";
    clH.res_class = (char *)"osgViewer";
    XSetClassHint( _display, _window, &clH);

    // This positions the window at _windowX, _windowY
    XSizeHints sh;
    sh.flags = 0;
    sh.flags |= USSize;
    sh.flags &= 0x7;
    sh.flags |= USPosition;
    sh.flags &= 0xB;
    sh.x = _traits->x;
    sh.y = _traits->y;
    sh.width  = _traits->width;
    sh.height = _traits->height;
    XSetStandardProperties( _display, _window, _traits->windowName.c_str(), _traits->windowName.c_str(), None, 0, 0, &sh);

    setWindowDecoration(_traits->windowDecoration);


    useCursor(_traits->useCursor);

    _deleteWindow = XInternAtom (_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(_display, _window, &_deleteWindow, 1);


    XFlush( _display );
    XSync( _display, 0 );


    // get window geometry relative to root window/screen
    Window child_return;
    int windowX, windowY;
    XGetWindowAttributes( _display, _window, &watt );
    XTranslateCoordinates( _display, _window, watt.root, watt.x, watt.y, &windowX, &windowY, &child_return);

    if (_traits->x != windowX || _traits->y != windowY ||_traits->width != watt.width || _traits->height != watt.height)
    {

        if (doFullSceenWorkAround)
        {
            OSG_INFO<<"Full Screen failed, resizing manually"<<std::endl;
            XMoveResizeWindow(_display, _window, _traits->x, _traits->y, _traits->width, _traits->height);

            XFlush(_display);
            XSync(_display, 0);

            XGetWindowAttributes( _display, _window, &watt );
            XTranslateCoordinates( _display, _window, watt.root, watt.x, watt.y, &windowX, &windowY, &child_return);
        }

        resized( windowX, windowY, watt.width, watt.height );
    }

    //OSG_NOTICE<<"After sync apply.x = "<<watt.x<<" watt.y="<<watt.y<<" width="<<watt.width<<" height="<<watt.height<<std::endl;


    XSelectInput( _eventDisplay, _window, ExposureMask | StructureNotifyMask |
                                     KeyPressMask | KeyReleaseMask |
                                     PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
                                     KeymapStateMask | FocusChangeMask | EnterWindowMask );

    XFlush( _eventDisplay );
    XSync( _eventDisplay, 0 );
    rescanModifierMapping();

    return true;
}

bool GraphicsWindowX11::setWindow(Window window)
{
    if (_initialized)
    {
        OSG_NOTICE << "GraphicsWindowX11::setWindow() - Window already created; it cannot be changed";
        return false;
    }

    if (window==0)
    {
        OSG_NOTICE << "GraphicsWindowX11::setWindow() - Invalid window handle passed ";
        return false;
    }

    _window = window;
    if (_window==0)
    {
        OSG_NOTICE << "GraphicsWindowX11::setWindow() - Unable to retrieve native window handle";
        return false;
    }

    XWindowAttributes watt;
    XGetWindowAttributes( _display, _window, &watt );
    _traits->x = watt.x;
    _traits->y = watt.y;
    _traits->width = watt.width;
    _traits->height = watt.height;

    _parent = DefaultRootWindow( _display );

    //_traits->supportsResize = false;
    _traits->windowDecoration = false;

    if (_traits->windowName.size()) setWindowName(_traits->windowName);

    _eventDisplay = XOpenDisplay(_traits->displayName().c_str());

    XFlush( _eventDisplay );
    XSync( _eventDisplay, 0 );

    return true;
}

bool GraphicsWindowX11::realizeImplementation()
{
    if (_realized)
    {
        OSG_NOTICE<<"GraphicsWindowX11::realizeImplementation() Already realized"<<std::endl;
        return true;
    }

    if (!_initialized) init();

    if (!_initialized) return false;

    XMapWindow( _display, _window );

    getEventQueue()->syncWindowRectangleWithGraphicsContext();

    //    Window temp = _window;
//    XSetWMColormapWindows( _display, _window, &temp, 1);

    _realized = true;

    return true;
}

bool GraphicsWindowX11::makeCurrentImplementation()
{
    if (!_realized)
    {
        OSG_NOTICE<<"Warning: GraphicsWindow not realized, cannot do makeCurrent."<<std::endl;
        return false;
    }

    #ifdef OSG_USE_EGL
        bool result = eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _context)==EGL_TRUE;
        checkEGLError("after eglMakeCurrent()");
        return result;
    #else
        return glXMakeCurrent( _display, _window, _context )==True;
    #endif
}

bool GraphicsWindowX11::releaseContextImplementation()
{
    if (!_realized)
    {
        OSG_NOTICE<<"Warning: GraphicsWindow not realized, cannot do release context."<<std::endl;
        return false;
    }

    #ifdef OSG_USE_EGL
        bool result = eglMakeCurrent( _eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )==EGL_TRUE;
        checkEGLError("after eglMakeCurrent() release");
        return result;
    #else
        return glXMakeCurrent( _display, None, NULL )==True;
    #endif
}


void GraphicsWindowX11::closeImplementation()
{
    // OSG_NOTICE<<"Closing GraphicsWindowX11"<<std::endl;

    if (_eventDisplay)
    {
        XCloseDisplay( _eventDisplay );
        _eventDisplay = 0;
    }

    if (_display)
    {
        if (_context)
        {
        #ifdef OSG_USE_EGL
            eglDestroyContext( _eglDisplay, _context );
        #else
            glXDestroyContext( _display, _context );
        #endif
        }

        if (_window && _ownsWindow)
        {
            XDestroyWindow(_display, _window);
        }

        XFlush( _display );
        XSync( _display,0 );
    }

    _window = 0;
    _parent = 0;
    _context = 0;

    if (_visualInfo)
    {
        #ifdef OSG_USE_EGL
            delete _visualInfo;
        #else
            XFree(_visualInfo);
        #endif
        _visualInfo = 0;
    }


    if (_display)
    {
        XCloseDisplay( _display );
        _display = 0;
    }

    _initialized = false;
    _realized = false;
    _valid = false;
}

void GraphicsWindowX11::swapBuffersImplementation()
{
    if (!_realized) return;

    // OSG_NOTICE<<"swapBuffersImplementation "<<this<<" "<<OpenThreads::Thread::CurrentThread()<<std::endl;

    #ifdef OSG_USE_EGL
        eglSwapBuffers( _eglDisplay, _eglSurface );
        checkEGLError("after eglSwapBuffers()");
    #else
#if 0
        if (_traits.valid() && _traits->vsync) {

            unsigned int counter;
            glXGetVideoSyncSGI(&counter);
            glXWaitVideoSyncSGI(1, 0, &counter);
        }
#endif
        static bool init = false;
        if (_traits.valid() && _traits->swapGroupEnabled && !init) {
            setSwapGroup(_traits->swapGroupEnabled, _traits->swapGroup, _traits->swapBarrier);
            init = true;
        }

        glXSwapBuffers( _display, _window );
    #endif


    while( XPending(_display) )
    {
        XEvent ev;
        XNextEvent( _display, &ev );

        switch( ev.type )
        {
            case ClientMessage:
            {
                if (static_cast<Atom>(ev.xclient.data.l[0]) == _deleteWindow)
                {
                    OSG_INFO<<"DeleteWindow event received"<<std::endl;
                    getEventQueue()->closeWindow();
                }
            }
        }
    }
}

bool GraphicsWindowX11::checkEvents()
{
    if (!_realized) return false;

    Display* display = _eventDisplay;

    double baseTime = _timeOfLastCheckEvents;
    double eventTime = baseTime;
    double resizeTime = eventTime;
    _timeOfLastCheckEvents = getEventQueue()->getTime();
    if (baseTime>_timeOfLastCheckEvents) baseTime = _timeOfLastCheckEvents;


    // OSG_NOTICE<<"GraphicsWindowX11::checkEvents() : getEventQueue()->getCurrentEventState()->getGraphicsContext()="<<getEventQueue()->getCurrentEventState()->getGraphicsContext()<<std::endl;

    int windowX = _traits->x;
    int windowY = _traits->y;
    int windowWidth = _traits->width;
    int windowHeight = _traits->height;
    bool needNewWindowSize = false;

    Time firstEventTime = 0;

    // OSG_NOTICE<<"Check events"<<std::endl;
    while( XPending(display) )
    {
        XEvent ev;
        XNextEvent( display, &ev );

        switch( ev.type )
        {
            case ClientMessage:
            {
                OSG_INFO<<"ClientMessage event received"<<std::endl;
                if (static_cast<Atom>(ev.xclient.data.l[0]) == _deleteWindow)
                {
                    OSG_INFO<<"DeleteWindow event received"<<std::endl;
                    // FIXME only do if _ownsWindow ?
                    getEventQueue()->closeWindow(eventTime);
                }
                break;
            }
            case Expose :
                OSG_INFO<<"Expose x="<<ev.xexpose.x<<" y="<<ev.xexpose.y<<" width="<<ev.xexpose.width<<", height="<<ev.xexpose.height<<std::endl;
                requestRedraw();
                break;

            case GravityNotify :
                OSG_INFO<<"GravityNotify event received"<<std::endl;
                break;

            case ReparentNotify:
                OSG_INFO<<"ReparentNotify event received"<<std::endl;
                break;

            case DestroyNotify :
                OSG_INFO<<"DestroyNotify event received"<<std::endl;
                _realized =  false;
                _valid = false;
                break;

            case ConfigureNotify :
            {
                if (windowX != ev.xconfigure.x ||
                    windowY != ev.xconfigure.y ||
                    windowWidth != ev.xconfigure.width ||
                    windowHeight != ev.xconfigure.height)
                {
                    resizeTime = eventTime;

                    windowX = ev.xconfigure.x;
                    windowY = ev.xconfigure.y;
                    windowWidth = ev.xconfigure.width;
                    windowHeight = ev.xconfigure.height;
                }
                needNewWindowSize = true;

                break;
            }

            case MapNotify :
            {
                OSG_INFO<<"MapNotify"<<std::endl;
                XWindowAttributes watt;
                do
                    XGetWindowAttributes(display, _window, &watt );
                while( watt.map_state != IsViewable );

                if (windowWidth != watt.width || windowHeight != watt.height)
                {
                    resizeTime = eventTime;

                    windowWidth = watt.width;
                    windowHeight = watt.height;
                }
                needNewWindowSize = true;

                break;
            }

            case FocusIn :
                OSG_INFO<<"FocusIn event received"<<std::endl;
                flushKeyEvents();
                break;

            case UnmapNotify :
            case FocusOut :
            {
                OSG_INFO<<"FocusOut/UnmapNotify event received"<<std::endl;
                if (ev.type == FocusOut && ev.xfocus.mode != NotifyNormal) break;

                char modMap[32];
                getModifierMap(modMap);

                // release normal (non-modifier) keys
                for (unsigned int key = 8; key < 256; key++)
                {
                    bool isModifier = keyMapGetKey(modMap, key);
                    if (!isModifier) forceKey(key, eventTime, false);
                }
                needNewWindowSize = true;

                // release modifier keys
                for (unsigned int key = 8; key < 256; key++)
                {
                    bool isModifier = keyMapGetKey(modMap, key);
                    if (isModifier) forceKey(key, eventTime, false);
                }
                break;
            }

            case EnterNotify :
                OSG_INFO<<"EnterNotify event received"<<std::endl;
                _modifierState = ev.xcrossing.state;
                syncLocks();
                break;

            case KeymapNotify :
            {
                OSG_INFO<<"KeymapNotify event received"<<std::endl;

                // KeymapNotify is guaranteed to directly follow either a FocusIn or
                // an EnterNotify event. We are only interested in the FocusIn case.
                if (_lastEventType != FocusIn) break;

                char modMap[32];
                getModifierMap(modMap);
                syncLocks();

                char keyMap[32];
                XQueryKeymap(_eventDisplay, keyMap);

                // release normal (non-modifier) keys
                for (unsigned int key = 8; key < 256; key++)
                {
                    bool isModifier = keyMapGetKey(modMap, key);
                    if (isModifier) continue;
                    bool isPressed = keyMapGetKey(keyMap, key);
                    if (!isPressed) forceKey(key, eventTime, false);
                }
                needNewWindowSize = true;

                // press/release modifier keys
                for (unsigned int key = 8; key < 256; key++)
                {
                    bool isModifier = keyMapGetKey(modMap, key);
                    if (!isModifier) continue;
                    bool isPressed = keyMapGetKey(keyMap, key);
                    forceKey(key, eventTime, isPressed);
                }

                // press normal keys
                for (unsigned int key = 8; key < 256; key++)
                {
                    bool isModifier = keyMapGetKey(modMap, key);
                    if (isModifier) continue;
                    bool isPressed = keyMapGetKey(keyMap, key);
                    if (isPressed) forceKey(key, eventTime, true);
                }
                break;
            }

            case MappingNotify :
                OSG_INFO<<"MappingNotify event received"<<std::endl;
                if (ev.xmapping.request == MappingModifier) rescanModifierMapping();
                break;

            case MotionNotify :
            {
                if (firstEventTime==0) firstEventTime = ev.xmotion.time;
                Time relativeTime = ev.xmotion.time - firstEventTime;
                eventTime = baseTime + static_cast<double>(relativeTime)*0.001;

                int  wx, wy;
                Window win = 0L;
                if( ev.xmotion.same_screen )
                {
                    wx = ev.xmotion.x;
                    wy = ev.xmotion.y;
                }
                else
                {
                    // the mouse in on another screen so need to compute the
                    // coordinates of the mouse position relative to an absolute position
                    // then take away the position of the original window/screen to get
                    // the coordinates relative to the original position.
                    Window root;
                    int rx, ry;
                    unsigned int buttons;

                    int screenOrigin_x = 0;
                    int screenOrigin_y = 0;
                    int i;
                    for(i= 0; i < ScreenCount(display); i++ )
                    {
                        if( XQueryPointer( display, RootWindow(display, i),
                              &root, &win, &rx, &ry, &wx, &wy, &buttons) )
                        {
                            break;
                        }

                        screenOrigin_x += DisplayWidth(display, i);
                    }

                    for(i= 0; i < static_cast<int>(_traits->screenNum); i++ )
                    {
                        screenOrigin_x -= DisplayWidth(display, i);
                    }

                    int dest_x_return, dest_y_return;
                    Window child_return;
                    XTranslateCoordinates(display, _window, _parent, 0, 0, &dest_x_return, &dest_y_return, &child_return);

                    wx += (screenOrigin_x - dest_x_return);
                    wy += (screenOrigin_y - dest_y_return);
                }


                float mx = wx;
                float my = wy;
                transformMouseXY(mx, my);
                getEventQueue()->mouseMotion(mx, my, eventTime);

                // OSG_NOTICE<<"MotionNotify wx="<<wx<<" wy="<<wy<<" mx="<<mx<<" my="<<my<<std::endl;

                break;
            }

            case ButtonPress :
            {
                if (firstEventTime==0) firstEventTime = ev.xmotion.time;
                Time relativeTime = ev.xmotion.time - firstEventTime;
                eventTime = baseTime + static_cast<double>(relativeTime)*0.001;

                if( ev.xbutton.button == Button4 )
                {
                    getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP, eventTime);
                }
                else if( ev.xbutton.button == Button5)
                {
                    getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN, eventTime);
                }
                else
                {
                    float mx = ev.xbutton.x;
                    float my = ev.xmotion.y;
                    transformMouseXY(mx, my);
                    getEventQueue()->mouseButtonPress(mx, my, ev.xbutton.button, eventTime);
                }
                break;
            }

            case ButtonRelease :
            {
                if (firstEventTime==0) firstEventTime = ev.xmotion.time;
                Time relativeTime = ev.xmotion.time - firstEventTime;
                eventTime = baseTime + static_cast<double>(relativeTime)*0.001;

                if( ev.xbutton.button == Button4 )
                {
                    //ignore scroll release as X11 is generating button press and release pairs for scroll wheel motion and we only need only event for each scroll action
                    //getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP, eventTime);
                }
                else if( ev.xbutton.button == Button5)
                {
                    //ignore scroll release as X11 is generating button press and release pairs for scroll wheel motion and we only need only event for each scroll action
                    //getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN, eventTime);
                }
                else
                {
                    float mx = ev.xbutton.x;
                    float my = ev.xmotion.y;
                    transformMouseXY(mx, my);
                    getEventQueue()->mouseButtonRelease(mx, my, ev.xbutton.button, eventTime);
                }
                break;
            }

            case KeyPress:
            {
                if (firstEventTime==0) firstEventTime = ev.xmotion.time;
                Time relativeTime = ev.xmotion.time - firstEventTime;
                eventTime = baseTime + static_cast<double>(relativeTime)*0.001;

                _modifierState = ev.xkey.state;
                keyMapSetKey(_keyMap, ev.xkey.keycode);
                int keySymbol = 0;
                int unmodifiedKeySymbol = 0;
                adaptKey(ev.xkey, keySymbol, unmodifiedKeySymbol);

                getEventQueue()->keyPress(keySymbol, eventTime, unmodifiedKeySymbol);
                break;
            }

            case KeyRelease:
            {
                if (firstEventTime==0) firstEventTime = ev.xmotion.time;
                Time relativeTime = ev.xmotion.time - firstEventTime;
                eventTime = baseTime + static_cast<double>(relativeTime)*0.001;
#if 1
                // Check for following KeyPress events and see if
                // the pair are the result of auto-repeat. If so, drop
                // this one on the floor, to be consistent with
                // Windows and Mac ports. The idea comes from libSDL sources.
                XEvent nextev;
                if (XPending(display))
                {
                    XPeekEvent(display, &nextev);
                    if ((nextev.type == KeyPress)
                        && (nextev.xkey.keycode == ev.xkey.keycode)
                        && (nextev.xmotion.time - ev.xmotion.time < 2))
                    {
                        break;
                    }
                }
#endif
                _modifierState = ev.xkey.state;
                keyMapClearKey(_keyMap, ev.xkey.keycode);
                int keySymbol = 0;
                int unmodifiedKeySymbol = 0;
                adaptKey(ev.xkey, keySymbol, unmodifiedKeySymbol);

                getEventQueue()->keyRelease(keySymbol, eventTime, unmodifiedKeySymbol);
                break;
            }

            default:
                OSG_INFO<<"Other event "<<ev.type<<std::endl;
                break;

        }
        _lastEventType = ev.type;
    }


    // get window geometry relative to root window/screen
    if (needNewWindowSize)
    {
        XWindowAttributes watt;
        Window child_return;

        XGetWindowAttributes( display, _window, &watt );
        XTranslateCoordinates(display, _window, watt.root, watt.x, watt.y, &windowX, &windowY, &child_return);
    }

    // send window resize event if window position or size was changed
    if (windowX != _traits->x ||
        windowY != _traits->y ||
        windowWidth != _traits->width ||
        windowHeight != _traits->height)
    {

        resized(windowX, windowY, windowWidth, windowHeight);
        getEventQueue()->windowResize(windowX, windowY, windowWidth, windowHeight, resizeTime);

        // request window repaint if window size was changed
        if (windowWidth != _traits->width ||
            windowHeight != _traits->height)
        {
            requestRedraw();
        }
    }

    return !(getEventQueue()->empty());
}

void GraphicsWindowX11::grabFocus()
{
    Display* display = getDisplayToUse();

    XSetInputFocus( display, _window, RevertToNone, CurrentTime );
    XFlush(display);
    XSync(display,0);
}

void GraphicsWindowX11::grabFocusIfPointerInWindow()
{
    Window win, root;
    int wx, wy, rx, ry;
    unsigned int buttons;

    Display* display = getDisplayToUse();

    if( XQueryPointer( display, _window,
          &root, &win, &rx, &ry, &wx, &wy, &buttons))
    {
#if 0
        if (wx>=0 && wx<_traits->width &&
            wy>=0 && wy<_traits->height)
        {
            grabFocus();
        }
#else
        grabFocus();
#endif
    }
}

void GraphicsWindowX11::transformMouseXY(float& x, float& y)
{
    if (getEventQueue()->getUseFixedMouseInputRange())
    {
        osgGA::GUIEventAdapter* eventState = getEventQueue()->getCurrentEventState();
        x = eventState->getXmin() + (eventState->getXmax()-eventState->getXmin())*x/float(_traits->width);
        y = eventState->getYmin() + (eventState->getYmax()-eventState->getYmin())*y/float(_traits->height);
    }
}

#ifndef OSG_USE_EGL
void GraphicsWindowX11::setSwapGroup(bool on, GLuint group, GLuint barrier)
{
    if (_traits.valid())
    {
        _traits->swapGroupEnabled = on;
        _traits->swapGroup        = group;
        _traits->swapBarrier      = barrier;
    }

    typedef Bool (GL_APIENTRY *PFNGLXJOINSWAPGROUPNVPROC) (Display *dpy, GLXDrawable drawable, GLuint group);
    PFNGLXJOINSWAPGROUPNVPROC glXJoinSwapGroupNV = (PFNGLXJOINSWAPGROUPNVPROC)glXGetProcAddress( (GLubyte *)"glXJoinSwapGroupNV" );

    typedef Bool (GL_APIENTRY *PFNGLXBINDSWAPBARRIERNVPROC) (Display *dpy, GLuint group, GLuint barrier);
    PFNGLXBINDSWAPBARRIERNVPROC glXBindSwapBarrierNV = (PFNGLXBINDSWAPBARRIERNVPROC)glXGetProcAddress( (GLubyte *)"glXBindSwapBarrierNV" );

    if ((!glXJoinSwapGroupNV) || (!glXBindSwapBarrierNV))
    {
        OSG_INFO << "GraphicsWindowX11::setSwapGroup(bool, GLuint, GLuint) not supported" << std::endl;
        return;
    }

    int swapGroup = (on ? group : 0);
    Bool resultJoin = glXJoinSwapGroupNV(_display, _window, swapGroup);
    OSG_INFO << "GraphicsWindowX11::glXJoinSwapGroupNV (" << swapGroup
             << ") returned " << resultJoin << std::endl;

    int swapBarrier = (on ? barrier : 0);
    Bool resultBind = glXBindSwapBarrierNV(_display, swapGroup, swapBarrier);
    OSG_INFO << "GraphicsWindowX11::glXBindSwapBarrierNV (" << swapGroup << ", "
             << swapBarrier << ") returned " << resultBind << std::endl;
}
#endif

void GraphicsWindowX11::adaptKey(XKeyEvent& keyevent, int& keySymbol, int& unmodifiedKeySymbol)
{
    unsigned char buffer_return[32];
    int bytes_buffer = sizeof(buffer_return);
    KeySym keysym_return;

    int numChars = XLookupString(&keyevent, reinterpret_cast<char*>(buffer_return), bytes_buffer, &keysym_return, NULL);
    keySymbol = keysym_return;
    if (!remapExtendedX11Key(keySymbol) && (numChars==1))
    {
        keySymbol = buffer_return[0];
    }

    unmodifiedKeySymbol = XkbKeycodeToKeysym(keyevent.display, keyevent.keycode, 0, 0);
}

// Function to inject artificial key presses/releases.
void GraphicsWindowX11::forceKey(int key, double time, bool state)
{
    if (!(state ^ keyMapGetKey(_keyMap, key))) return; // already pressed/released

    XKeyEvent event;
    event.serial = 0;
    event.send_event = True;
    event.display = _eventDisplay;
    event.window = _window;
    event.subwindow = 0;
    event.time = 0;
    event.x = 0;
    event.y = 0;
    event.x_root = 0;
    event.y_root = 0;
    event.state = getModifierMask() | (_modifierState & (LockMask | _numLockMask));
    event.keycode = key;
    event.same_screen = True;

    int keySymbol = 0;
    int unmodifiedKeySymbol = 0;
    if (state)
    {
        event.type = KeyPress;
        adaptKey(event, keySymbol, unmodifiedKeySymbol);
        getEventQueue()->keyPress(keySymbol, time, unmodifiedKeySymbol);
        keyMapSetKey(_keyMap, key);
    }
    else
    {
        event.type = KeyRelease;
        adaptKey(event, keySymbol, unmodifiedKeySymbol);
        getEventQueue()->keyRelease(keySymbol, time, unmodifiedKeySymbol);
        keyMapClearKey(_keyMap, key);
    }
}

void GraphicsWindowX11::syncLocks()
{
    unsigned int mask = getEventQueue()->getCurrentEventState()->getModKeyMask();

    if (_modifierState & LockMask)
        mask |= osgGA::GUIEventAdapter::MODKEY_CAPS_LOCK;
    else
        mask &= ~osgGA::GUIEventAdapter::MODKEY_CAPS_LOCK;

    if (_modifierState & _numLockMask)
        mask |= osgGA::GUIEventAdapter::MODKEY_NUM_LOCK;
    else
        mask &= ~osgGA::GUIEventAdapter::MODKEY_NUM_LOCK;

    getEventQueue()->getCurrentEventState()->setModKeyMask(mask);
}

void GraphicsWindowX11::rescanModifierMapping()
{
    XModifierKeymap *mkm = XGetModifierMapping(_eventDisplay);
    KeyCode *m = mkm->modifiermap;
    KeyCode numlock = XKeysymToKeycode(_eventDisplay, XK_Num_Lock);
    _numLockMask = 0;
    for (int i = 0; i < mkm->max_keypermod * 8; i++, m++)
    {
        if (*m == numlock)
        {
            _numLockMask = 1 << (i / mkm->max_keypermod);
            break;
        }
    }
    XFree(mkm->modifiermap);
    XFree(mkm);
}

void GraphicsWindowX11::flushKeyEvents()
{
    XEvent e;
    while (XCheckMaskEvent(_eventDisplay, KeyPressMask|KeyReleaseMask, &e))
        continue;
}

// Returns char[32] keymap with bits for every modifier key set.
void GraphicsWindowX11::getModifierMap(char* keymap) const
{
    memset(keymap, 0, 32);
    XModifierKeymap *mkm = XGetModifierMapping(_eventDisplay);
    KeyCode *m = mkm->modifiermap;
    for (int i = 0; i < mkm->max_keypermod * 8; i++, m++)
    {
        if (*m) keyMapSetKey(keymap, *m);
    }
    XFree(mkm->modifiermap);
    XFree(mkm);
}

int GraphicsWindowX11::getModifierMask() const
{
    int mask = 0;
    XModifierKeymap *mkm = XGetModifierMapping(_eventDisplay);
    for (int i = 0; i < mkm->max_keypermod * 8; i++)
    {
        unsigned int key = mkm->modifiermap[i];
        if (key && keyMapGetKey(_keyMap, key))
        {
            mask |= 1 << (i / mkm->max_keypermod);
        }
    }
    XFree(mkm->modifiermap);
    XFree(mkm);
    return mask;
}

void GraphicsWindowX11::requestWarpPointer(float x,float y)
{
    if (!_realized)
    {
        OSG_INFO<<"GraphicsWindowX11::requestWarpPointer() - Window not realized; cannot warp pointer, screenNum="<< _traits->screenNum<<std::endl;
        return;
    }

    Display* display = _eventDisplay; // getDisplayToUse();

    XWarpPointer( display,
                  None,
                  _window,
                  0, 0, 0, 0,
                  static_cast<int>(x), static_cast<int>(y) );

    XFlush(display);
    XSync(display, 0);

    getEventQueue()->mouseWarped(x,y);
}

extern "C"
{

typedef int (*X11ErrorHandler)(Display*, XErrorEvent*);

int X11ErrorHandling(Display* display, XErrorEvent* event)
{
    OSG_NOTICE<<"Got an X11ErrorHandling call display="<<display<<" event="<<event<<std::endl;

    char buffer[256];
    XGetErrorText( display, event->error_code, buffer, 256);

    OSG_NOTICE << buffer << std::endl;
    OSG_NOTICE << "Major opcode: " << (int)event->request_code << std::endl;
    OSG_NOTICE << "Minor opcode: " << (int)event->minor_code << std::endl;
    OSG_NOTICE << "Error code: " << (int)event->error_code << std::endl;
    OSG_NOTICE << "Request serial: " << event->serial << std::endl;
    OSG_NOTICE << "Current serial: " << NextRequest( display ) - 1 << std::endl;

    switch( event->error_code )
    {
        case BadValue:
            OSG_NOTICE << "  Value: " << event->resourceid << std::endl;
            break;

        case BadAtom:
            OSG_NOTICE << "  AtomID: " << event->resourceid << std::endl;
            break;

        default:
            OSG_NOTICE << "  ResourceID: " << event->resourceid << std::endl;
            break;
    }
    return 0;
}

}

class X11WindowingSystemInterface : public osg::GraphicsContext::WindowingSystemInterface
{
#ifdef OSGVIEWER_USE_XRANDR
    // TODO: Investigate whether or not Robert thinks we should store/restore the original
    // resolution in the destructor; I'm not sure the other ones do this, and it may be the
    // responsibility of the user.
    bool _setScreen(const osg::GraphicsContext::ScreenIdentifier& si, unsigned int width, unsigned int height, unsigned int colorDepth, double rate) {
        if (colorDepth>0)
            OSG_NOTICE << "X11WindowingSystemInterface::_setScreen() is not fully implemented (missing depth)."<<std::endl;

        Display* display = XOpenDisplay(si.displayName().c_str());

        if(display)
        {
            XRRScreenConfiguration* sc = XRRGetScreenInfo(display, RootWindow(display, si.screenNum));

            if(!sc)
            {
                OSG_NOTICE << "Unable to create XRRScreenConfiguration on display \"" << XDisplayName(si.displayName().c_str()) << "\"."<<std::endl;
                return false;
            }

            int      numScreens = 0;
            int      numRates   = 0;
            Rotation currentRot = 0;
            bool     okay       = false;

            XRRConfigRotations(sc, &currentRot);

            // If the width or height are zero, use our defaults.
            if(!width || !height)
            {
                getScreenResolution(si, width, height);
            }

            // If this somehow fails, okay will still be false, no iteration will take place below,
            // and the sc pointer will still be freed later on.
            XRRScreenSize* ss = XRRConfigSizes(sc, &numScreens);

            for(int i = 0; i < numScreens; i++)
            {
                if(ss[i].width == static_cast<int>(width) && ss[i].height == static_cast<int>(height))
                {
                    short* rates     = XRRConfigRates(sc, i, &numRates);
                    bool   rateFound = false;

                    // Search for our rate in the list of acceptable rates given to us by Xrandr.
                    // If it's not found, rateFound will still be false and the call will never
                    // be made to XRRSetScreenConfigAndRate since the rate will be invalid.
                    for(int r = 0; r < numRates; r++)
                    {
                        if(rates[r] == static_cast<short>(rate))
                        {
                            rateFound = true;
                            break;
                        }
                    }

                    if(rate > 0.0f && !rateFound)
                    {
                        OSG_NOTICE << "Unable to find valid refresh rate " << rate << " on display \"" << XDisplayName(si.displayName().c_str()) << "\"."<<std::endl;
                    }
                    else if(XRRSetScreenConfigAndRate(display, sc, DefaultRootWindow(display), i, currentRot, static_cast<short>(rate), CurrentTime) != RRSetConfigSuccess)
                    {
                        OSG_NOTICE << "Unable to set resolution to " << width << "x" << height << " on display \"" << XDisplayName(si.displayName().c_str()) << "\"."<<std::endl;
                    }
                    else
                    {
                        okay = true;
                        break;
                    }
                }
            }

            XRRFreeScreenConfigInfo(sc);

            return okay;
        }
        else
        {
            OSG_NOTICE << "Unable to open display \"" << XDisplayName(si.displayName().c_str()) << "\"."<<std::endl;
            return false;
        }
    }
#endif

protected:
    bool _errorHandlerSet;


public:
    X11WindowingSystemInterface()
    {
        OSG_INFO<<"X11WindowingSystemInterface()"<<std::endl;


        // Install an X11 error handler, if the application has not already done so.

        // Set default handler, and get pointer to current handler.
        X11ErrorHandler currentHandler = XSetErrorHandler(NULL);

        // Set our handler, and get pointer to default handler.
        X11ErrorHandler defHandler = XSetErrorHandler(X11ErrorHandling);

        if ( currentHandler == defHandler )
        {
            // No application error handler, use ours.
            // OSG_INFO<<"Set osgViewer X11 error handler"<<std::endl;
            _errorHandlerSet = 1;
        }
        else
        {
            // Application error handler exists, leave it set.
            // OSG_INFO<<"Existing application X11 error handler set"<<std::endl;
            _errorHandlerSet = 0;
            XSetErrorHandler(currentHandler);
        }

#if 0
        if (XInitThreads() == 0)
        {
            OSG_NOTICE << "Error: XInitThreads() failed. Aborting." << std::endl;
            exit(1);
        }
        else
        {
            OSG_INFO << "X11WindowingSystemInterface, xInitThreads() multi-threaded X support initialized.\n";
        }
#endif

    }



    ~X11WindowingSystemInterface()
    {
        if (osg::Referenced::getDeleteHandler())
        {
            osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
            osg::Referenced::getDeleteHandler()->flushAll();
        }

        //OSG_NOTICE<<"~X11WindowingSystemInterface()"<<std::endl;

        // Unset our X11 error handler, providing the application has not replaced it.

        if ( _errorHandlerSet )
        {
            X11ErrorHandler currentHandler = XSetErrorHandler(NULL);
            if ( currentHandler == X11ErrorHandling )
            {
                // OSG_INFO<<"osgViewer X11 error handler removed"<<std::endl;
            }
            else
            {
                // Not our error handler, leave it set.
                // OSG_INFO<<"Application X11 error handler left"<<std::endl;
                XSetErrorHandler(currentHandler);
            }
        }
    }

    virtual unsigned int getNumScreens(const osg::GraphicsContext::ScreenIdentifier& si)
    {
        Display* display = XOpenDisplay(si.displayName().c_str());
        if(display)
        {
            unsigned int numScreens = ScreenCount(display);
            XCloseDisplay(display);

            return numScreens;
        }
        else
        {
            OSG_NOTICE << "A Unable to open display \"" << XDisplayName(si.displayName().c_str()) << "\""<<std::endl;
            return 0;
        }
    }

    bool supportsRandr(Display* display) const
    {
#ifdef OSGVIEWER_USE_XRANDR
        int event_basep;
        int error_basep;
        bool supports_randr = XRRQueryExtension( display, &event_basep, &error_basep );
        if( supports_randr )
        {
            int major, minor;
            XRRQueryVersion( display, &major, &minor );
            return ( major > 1 || ( major == 1 && minor >= 2 ) );
        }
#endif
        return false;
    }

    virtual void getScreenSettings(const osg::GraphicsContext::ScreenIdentifier& si, osg::GraphicsContext::ScreenSettings & resolution )
    {
        Display* display = XOpenDisplay(si.displayName().c_str());
        if(display)
        {
            resolution.width = DisplayWidth(display, si.screenNum);
            resolution.height = DisplayHeight(display, si.screenNum);
            resolution.colorDepth = DefaultDepth(display, si.screenNum);

            resolution.refreshRate = 0;            // Missing call. Need a X11 expert.


#ifdef OSGVIEWER_USE_XRANDR
           if (supportsRandr(display))
           {

               XRRScreenConfiguration* screenConfig = XRRGetScreenInfo ( display, RootWindow(display, si.screenNum) );

               resolution.refreshRate = XRRConfigCurrentRate ( screenConfig );

               XRRFreeScreenConfigInfo( screenConfig );
           }
#endif


            XCloseDisplay(display);
        }
        else
        {
            OSG_NOTICE << "Unable to open display \"" << XDisplayName(si.displayName().c_str()) << "\"."<<std::endl;
            resolution.width = 0;
            resolution.height = 0;
            resolution.colorDepth = 0;
            resolution.refreshRate = 0;
        }
    }

    virtual bool setScreenSettings(const osg::GraphicsContext::ScreenIdentifier& si, const osg::GraphicsContext::ScreenSettings & resolution )
    {
#ifdef OSGVIEWER_USE_XRANDR
        _setScreen(si, resolution.width, resolution.height, resolution.colorDepth, resolution.refreshRate);
#else
        OSG_NOTICE << "You must build osgViewer with Xrandr 1.2 or higher for setScreenSettings support!" << std::endl;
#endif
        return false;
    }

    virtual void enumerateScreenSettings(const osg::GraphicsContext::ScreenIdentifier& si, osg::GraphicsContext::ScreenSettingsList & resolutionList)
    {
        resolutionList.clear();

        Display* display = XOpenDisplay(si.displayName().c_str());
        if(display)
        {
#ifdef OSGVIEWER_USE_XRANDR
            int defaultDepth = DefaultDepth(display, si.screenNum);

            if (supportsRandr(display))
            {
                int nsizes = 0;
                XRRScreenSize * screenSizes = XRRSizes(display, si.screenNum, &nsizes);
                if (screenSizes && nsizes>0)
                {
                    for(int i=0; i<nsizes; ++i)
                    {
                        OSG_INFO<<"Screen size "<<screenSizes[i].width<<" "<<screenSizes[i].height<<" "<<screenSizes[i].mwidth<<" "<<screenSizes[i].mheight<<std::endl;

                        int nrates;
                        short * rates = XRRRates (display, si.screenNum, i, &nrates);
                        if (rates && nrates>0)
                        {
                            for(int j=0; j<nrates; ++j)
                            {
                                OSG_INFO<<"   rates "<<rates[j]<<std::endl;

                                resolutionList.push_back(osg::GraphicsContext::ScreenSettings(
                                    screenSizes[i].width,
                                    screenSizes[i].height,
                                    double(rates[j]),
                                    defaultDepth));
                            }
                        }
                        else
                        {
                            resolutionList.push_back(osg::GraphicsContext::ScreenSettings(
                                screenSizes[i].width,
                                screenSizes[i].height,
                                0.0,
                                defaultDepth));
                        }

                    }
                }
            }
#endif
            XCloseDisplay(display);
        }

        if (resolutionList.empty())
        {
            OSG_NOTICE << "X11WindowingSystemInterface::enumerateScreenSettings() not supported." << std::endl;
        }
    }

    virtual osg::GraphicsContext* createGraphicsContext(osg::GraphicsContext::Traits* traits)
    {
        if (traits->pbuffer)
        {
#if 1
            osg::ref_ptr<osgViewer::PixelBufferX11> pbuffer = new PixelBufferX11(traits);
            if (pbuffer->valid()) return pbuffer.release();
            else return 0;
#else
            osg::ref_ptr<osgViewer::GraphicsWindowX11> window = new GraphicsWindowX11(traits);
            if (window->valid()) return window.release();
            else return 0;
#endif
        }
        else
        {
            osg::ref_ptr<osgViewer::GraphicsWindowX11> window = new GraphicsWindowX11(traits);
            if (window->valid()) return window.release();
            else return 0;
        }
    }

};

REGISTER_WINDOWINGSYSTEMINTERFACE2(X11, X11WindowingSystemInterface, OSGVIEWER_EXPORT)

void GraphicsWindowX11::raiseWindow()
{
    Display* display = getDisplayToUse();
    if(!display) return;

    // get handles to window props of interest
    Atom stateAbove = XInternAtom(display, "_NET_WM_STATE_ABOVE", True);
    Atom stateAtom = XInternAtom(display, "_NET_WM_STATE", True);
    // check that atoms are supported
    if(stateAbove != None && stateAtom != None)
    {
        // fill an XEvent struct to send
        XEvent xev;
        xev.xclient.type = ClientMessage;
        xev.xclient.serial = 0;
        xev.xclient.send_event = True;
        xev.xclient.window = _window;
        xev.xclient.message_type = stateAtom;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 1;
        xev.xclient.data.l[1] = stateAbove;
        xev.xclient.data.l[2] = 0;

        XSendEvent(display, RootWindow(display, DefaultScreen(display)),
                   False,  SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    }
    else
    {
        // one or both of _NET_WM_STATE and _NET_WM_STATE_ABOVE aren't supported
        // try to use XRaiseWindow
        XWindowAttributes winAttrib;

        Window root_return, parent_return, *children;
        unsigned int nchildren, i=0;
        XTextProperty windowName;
        bool xraise = false;

        // get the window tree around our current window
        XQueryTree(display, _parent, &root_return, &parent_return, &children, &nchildren);
        while (!xraise &&  i<nchildren)
        {
            XGetWMName(display,children[i++],&windowName);
            if ((windowName.nitems != 0) && (strcmp(_traits->windowName.c_str(),(const char *)windowName.value) == 0))
                xraise = true;
        }
        if (xraise)
            XRaiseWindow(display,_window);
        else
        {
            XGetWindowAttributes(display, _window, &winAttrib);
            XReparentWindow(display, _window, _parent, winAttrib.x, winAttrib.y);
        }

        XFree(children);
    }

    XFlush(display);
    XSync(display,0);
}
