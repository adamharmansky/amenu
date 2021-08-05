#ifndef DRAW_C
#define DRAW_C
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xft/Xft.h>
#include <pthread.h>
#include <string.h>

typedef struct {
	XftColor xftcolor;
} color_t;

typedef struct {
	XftFont* xftfont;
} font_t;

typedef struct {
	int width, height;
	char* data;
	XImage* image;
} image_t;

typedef struct {
	Display* disp;
	int scr;
	Window win;
	XftDraw* draw;
	Drawable drawable;
	GC gc;
	int w, h;
} draw_context;

extern draw_context draw_init(int, int, char*, long);

extern color_t create_color(draw_context,char, char, char);

extern int draw_rectangle(draw_context,int, int, int, int, color_t);

extern font_t  load_font(draw_context,char* name);

extern int draw_text(draw_context,int, int, font_t, char*, color_t);
extern void draw_char(draw_context,int, int, font_t, char, color_t);

extern int draw_char_width(draw_context,font_t f, char c);
extern int draw_string_width(draw_context,font_t f, char* c);

extern void draw_flush_all(draw_context);
extern void draw_flush(draw_context,int x, int y, int w, int h);
extern void draw_resize(draw_context*,int w, int h);

extern unsigned int draw_width (draw_context);
extern unsigned int draw_height(draw_context);

extern image_t create_image(draw_context c, color_t bg, char* filename);
extern void draw_image(draw_context c, int x, int y, image_t img);
extern void free_image(image_t img);
#endif
