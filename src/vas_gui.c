#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "vas.h"

#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#include "nuklear.h"

#define NK_XLIB_IMPLEMENTATION
#include "nuklear_xlib.h"

extern float a4;
extern int env_count;
extern int midi_channel;
extern int mod_count;
extern int part_count;

extern struct envelope *env;
extern struct modulator *mod;
extern struct partial *part;

struct XWindow{
    Display *dpy;
    Window root;
    Visual *vis;
    Colormap cmap;
    XWindowAttributes attr;
    XSetWindowAttributes swa;
    Window win;
    int screen;
    XFont *font;
    unsigned int width;
    unsigned int height;
    Atom wm_delete_window;
};

static struct XWindow xw;

void gui_cleanup(void){
    nk_xfont_del(xw.dpy, xw.font);
    nk_xlib_shutdown();
    XUnmapWindow(xw.dpy, xw.win);
    XFreeColormap(xw.dpy, xw.cmap);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return;
}

void vas_gui_start(void){
    struct nk_context *ctx;
    char preset_name[80] = "vas_default";

    memset(&xw, 0, sizeof(xw));
    xw.dpy = XOpenDisplay(NULL);
    if(!xw.dpy){
        fprintf(stderr, "VAS: Failed opening X display.\n");
        vas_exit(1);
    }

    xw.root = DefaultRootWindow(xw.dpy);
    xw.screen = XDefaultScreen(xw.dpy);
    xw.vis = XDefaultVisual(xw.dpy, xw.screen);
    xw.cmap = XCreateColormap(xw.dpy, xw.root, xw.vis, AllocNone);

    xw.swa.colormap = xw.cmap;
    xw.swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask | ButtonMotionMask | Button1MotionMask
        | Button3MotionMask | Button4MotionMask | Button5MotionMask |
        PointerMotionMask | KeymapStateMask;
    xw.win = XCreateWindow(xw.dpy, xw.root, 0, 0, WIN_WIDTH, WIN_HEIGHT, 0,
        XDefaultDepth(xw.dpy, xw.screen), InputOutput, xw.vis,
        CWEventMask | CWColormap, &xw.swa);

    XStoreName(xw.dpy, xw.win, "VAS");
    XMapWindow(xw.dpy, xw.win);
    xw.wm_delete_window = XInternAtom(xw.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(xw.dpy, xw.win, &xw.wm_delete_window, 1);
    XGetWindowAttributes(xw.dpy, xw.win, &xw.attr);
    xw.width = (unsigned int)xw.attr.width;
    xw.height = (unsigned int)xw.attr.height;

    xw.font = nk_xfont_create(xw.dpy, "fixed");
    ctx = nk_xlib_init(xw.font, xw.dpy, xw.screen, xw.win, xw.width,
        xw.height);

    while(true){
        XEvent evt;
        nk_input_begin(ctx);
        while(XPending(xw.dpy)){
            XNextEvent(xw.dpy, &evt);
            if(evt.type == ClientMessage) vas_exit(0);
            if(XFilterEvent(&evt, xw.win)) continue;
            nk_xlib_handle_event(xw.dpy, xw.screen, xw.win, &evt);
        }
        nk_input_end(ctx);

        if(nk_begin(ctx, "topbar", nk_rect(0, 0, WIN_WIDTH, 40),
            NK_WINDOW_BORDER)){
            nk_layout_row_begin(ctx, NK_STATIC, 16, 2);
            nk_layout_row_push(ctx, 100);
            if(nk_menu_begin_label(ctx, "file", NK_TEXT_LEFT,
                nk_vec2(300, 300))){
                nk_layout_row_static(ctx, 40, 200, 1);
                nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD,
                    preset_name, 79, NULL);
                if(nk_button_label(ctx, "save")){
                    save_preset(preset_name);
                }
                if(nk_button_label(ctx, "load")){
                    load_preset(preset_name);
                }
                nk_menu_end(ctx);
            }
            nk_layout_row_push(ctx, 100);
            if(nk_menu_begin_label(ctx, "options", NK_TEXT_LEFT,
                nk_vec2(300, 300))){
                static char a4s[80] = "440.0";
                char tmp[80];
                strcpy(tmp, a4s);
                nk_layout_row_begin(ctx, NK_STATIC, 40, 2);
                nk_layout_row_push(ctx, 40);
                nk_label(ctx, "A4:", NK_TEXT_CENTERED);
                nk_layout_row_push(ctx, 200);
                nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD,
                    a4s, 79, nk_filter_float);
                if(strcmp(a4s, tmp)) a4 = atof(a4s);
                nk_menu_end(ctx);
            }
            nk_end(ctx);
        }

        if(nk_begin(ctx, "PARTIALS", nk_rect(0, 40, 400, WIN_HEIGHT),
            NK_WINDOW_TITLE | NK_WINDOW_BORDER)){
            nk_layout_row_begin(ctx, NK_STATIC, 40, 4);
            for(int i = 0; i < part_count; i++){
                char tmp[80];
                nk_layout_row_push(ctx, 128);
                nk_slider_float(ctx, 0, &part[i].gain, 1, 0.01);
                nk_layout_row_push(ctx, 128);
                nk_slider_float(ctx, 1, &part[i].ratio, 16, 0.25);
                nk_layout_row_push(ctx, 40);
                strcpy(tmp, part[i].env_s);
                nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD,
                    part[i].env_s, 79, nk_filter_decimal);
                if(strcmp(tmp, part[i].env_s))
                    part[i].env = atoi(part[i].env_s);
                nk_layout_row_push(ctx, 40);
                if(nk_button_label(ctx, "-")){
                    part_count--;
                    for(int j = i; j < part_count; j++) part[j] = part[j + 1];
                }
            }
            nk_layout_row_push(ctx, 40);
            if(nk_button_label(ctx, "+")){
                part_count++;
                if(part_count % BLOCK_SIZE == 1){
                    struct partial *tmp;
                    tmp = (struct partial*)realloc(part,
                        BLOCK_SIZE * ((part_count - 1) / BLOCK_SIZE + 1) *
                        sizeof(struct partial));
                    if(!tmp){
                        fprintf(stderr, "VAS: Memory reallocation failed.\n");
                        abort();
                    }
                    part = tmp;
                }
                memset(part + part_count - 1, 0, sizeof(struct partial));
                part[part_count - 1].ratio = 1.0;   // interval ratio of 0
                                                    // is invalid
                snprintf(part[part_count - 1].env_s, 80, "%d", 0);
                snprintf(part[part_count - 1].mod_s, 80, "%d", 0);
            }
            nk_layout_row_end(ctx);
            nk_end(ctx);
        }

        if(nk_begin(ctx, "ENVELOPES", nk_rect(400, 40, 440, WIN_HEIGHT),
            NK_WINDOW_TITLE | NK_WINDOW_BORDER)){
            for(int i = 0; i < env_count; i++){
                nk_layout_row_static(ctx, 40, 40, 5);
                nk_knob_float(ctx, 0, &env[i].attack, 1, 0.01, NK_DOWN, 90);
                nk_knob_float(ctx, 0, &env[i].decay, 1, 0.01, NK_DOWN, 90);
                nk_knob_float(ctx, 0, &env[i].sustain, 1, 0.01, NK_DOWN, 90);
                nk_knob_float(ctx, 0, &env[i].release, 1, 0.01, NK_DOWN, 90);
                if(nk_button_label(ctx, "-")){
                    env_count--;
                    for(int j = i; j < env_count; j++) env[j] = env[j + 1];
                }
                nk_layout_row_static(ctx, 12, 40, 4);
                nk_label(ctx, "A", NK_TEXT_CENTERED);
                nk_label(ctx, "D", NK_TEXT_CENTERED);
                nk_label(ctx, "S", NK_TEXT_CENTERED);
                nk_label(ctx, "R", NK_TEXT_CENTERED);
            }
            nk_layout_row_static(ctx, 40, 40, 1);
            if(nk_button_label(ctx, "+")){
                env_count++;
                if(env_count % BLOCK_SIZE == 1){
                    struct envelope *tmp;
                    tmp = (struct envelope*)realloc(env,
                        BLOCK_SIZE * ((env_count - 1) / BLOCK_SIZE + 1) *
                        sizeof(struct envelope));
                    if(!tmp){
                        fprintf(stderr, "VAS: Memory reallocation failed.\n");
                        abort();
                    }
                    env = tmp;
                }
                memset(env + env_count - 1, 0, sizeof(struct envelope));
            }
            nk_end(ctx);
        }

        XClearWindow(xw.dpy, xw.win);
        nk_xlib_render(xw.win, nk_rgb(0, 0, 0));
        XFlush(xw.dpy);

        struct timespec dur = {0, 16666667};    // 60 hz (ideally)
        nanosleep(&dur, NULL);                  // it's not like we need
                                                // precise timings here or
                                                // anything.
    }
}
