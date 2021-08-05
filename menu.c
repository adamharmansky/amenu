#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>
#include <X11/Xatom.h>
#include "draw.h"

#define APPDIR "/usr/share/applications"
#define ICONDIR "/usr/share/amenu/icons/48"
#define SEARCH_ICON "/usr/share/amenu/icons/24/search.ff"
#define FONT "Sans-serif:size=16"
#define FONT_1 "UniSans:size=18"
#define FONT_2 "Sans-serif:size=12"
#define FONT_3 "UniSansHeavy:size=100"

typedef struct {
	char name[256];
	char comment[512];
	char path[512];
	char has_icon;
	image_t icon;
	image_t selected_icon;
	uint16_t score;
} Application;

static draw_context dc;
static color_t bg;
static color_t bg1;
static color_t white;
static color_t dark_white;
static color_t yellow;
static color_t red;
static color_t green;
static font_t font;
static font_t font1;
static font_t font2;
static font_t font3;
static int window_width, window_height;
static char* time_string = NULL;
static char* date_string = NULL;

static Application* applications;
static int application_count;
static char query[256];

static int selected_app = 0;

static image_t search_icon;

static void
fuzz(Application* app)
{
	int i, j;
	int start = 0;
	int distance = 0;

	if(!*query) {
		app->score = -1;
		return;
	}
	app->score = 0;

	for(i = 0; query[i]; i++) {
		for(j = start; app->name[j]; j++) {
			if(app->name[j] == query[i] || app->name[j] == query[i] - 0x20) {
				app->score += j-start;
				start = j+1;
				break;
			}
		}
		if(!app->name[j]) {
			app->score = -1;
			return;
		}
	}
	if(j == i) app->score = 0;
}

int
compare_applications(const void* a, const void* b) {
	return (((Application*)a)->score>((Application*)b)->score)-(((Application*)a)->score<((Application*)b)->score);
}

static void
sort_applications()
{
	int i;
	for(i = 0; i < application_count; i++) {
		fuzz(&applications[i]);
	}
	qsort(applications, application_count, sizeof(Application), compare_applications);
}

void
exit_safely(int aaaa, void* aaaaa)
{
	int i;

	for(i = 0; i < application_count; i++) {
		if(applications[i].has_icon) {
			free_image(applications[i].icon);
			free_image(applications[i].selected_icon);
		}
	}
}

static void
load_app_entry(Application* app, char* filename)
{
	FILE* f = fopen(filename, "r");
	uint64_t i;
	int c;
	char icon_name[256];
	char icon_path[512];
	char found_name = 0;
	char found_icon = 0;
	char found_comment = 0;

	app->has_icon = 0;
	app->score = -1;
	*app->comment = 0;

	while((c = getc(f)) != EOF) {
		if(c == '\n') {
			c = getc(f);
			if(c == 'N') {
				if(found_name) continue;
				if((c = getc(f)) != 'a') continue;
				if((c = getc(f)) != 'm') continue;
				if((c = getc(f)) != 'e') continue;
				if((c = getc(f)) != '=') continue;
				for(i = 0; ; i++) {
					c = getc(f);
					if(c == '\n' || c == EOF) {
						app->name[i] = 0;
						break;
					};
					app->name[i] = c;
				}
				found_name = 1;
			} else if(c == 'I') {
				if(found_icon) continue;
				if((c = getc(f)) != 'c') continue;
				if((c = getc(f)) != 'o') continue;
				if((c = getc(f)) != 'n') continue;
				if((c = getc(f)) != '=') continue;
				for(i = 0; ; i++) {
					c = getc(f);
					if(c == '\n' || c == EOF) {
						icon_name[i] = 0;
						break;
					};
					icon_name[i] = c;
				}
				found_icon = 1;
				if(icon_name[0] != '/') {
					strcpy(icon_path, ICONDIR);
					strcat(icon_path, "/");
					strcat(icon_path, icon_name);
					strcat(icon_path, ".ff");
					if(!access(icon_path, F_OK)) {
						app->icon = create_image(dc, bg, icon_path);
						app->selected_icon = create_image(dc, bg1, icon_path);
						app->has_icon = 1;
					}
				}
			} else if(c == 'C') {
				if(found_comment) continue;
				if((c = getc(f)) != 'o') continue;
				if((c = getc(f)) != 'm') continue;
				if((c = getc(f)) != 'm') continue;
				if((c = getc(f)) != 'e') continue;
				if((c = getc(f)) != 'n') continue;
				if((c = getc(f)) != 't') continue;
				if((c = getc(f)) != '=') continue;
				for(i = 0; ; i++) {
					c = getc(f);
					if(c == '\n' || c == EOF) {
						app->comment[i] = 0;
						break;
					};
					app->comment[i] = c;
				}
				found_comment = 1;
			}
		}
	}
}

static int
load_applications()
{
	DIR* dir;
	FILE* f;
	char fullpath[512];
	struct dirent* ent;
	int i;

	applications = malloc(application_count = 0);
	
	dir = opendir(APPDIR);

	for(i = 0; (ent = readdir(dir)) != NULL; i++) {
		if(!(ent->d_type == DT_REG || ent->d_type == DT_LNK)) {
			i--;
			continue;
		}
		strcpy(fullpath, APPDIR);
		strcat(fullpath, "/");
		strcat(fullpath, ent->d_name);
		applications = realloc(applications, (++application_count)*sizeof(Application));
		load_app_entry(&applications[i], fullpath);
		strcpy(applications[i].path, ent->d_name);
	}

	closedir(dir);
}

static void
redraw()
{
	int i;

	draw_rectangle(dc, 0, 0, window_width, window_height, bg);
	for(i = 0; i < application_count && applications[i].score != (uint16_t)-1 && (i+1)*64+36 < window_height; i++) {
		if(i == selected_app) {
			draw_rectangle(dc, 8, 36 + 4 + i*64, window_width - 16, 56, bg1);
		}
		if(applications[i].has_icon) {
			if(i == selected_app) {
				draw_image(dc, 12, 36 + 8 + i*64, applications[i].selected_icon);
			} else {
				draw_image(dc, 12, 36 + 8 + i*64, applications[i].icon);
			}
		}
		draw_rectangle(dc, 0, 36+i*64+4, 3, 56, (applications[i].score < 5) ?  green : (applications[i].score < 10) ? yellow : red);
		draw_text(dc, 76, 36 + 10 + i*64 + font2.xftfont->ascent, font, applications[i].name, white);
		draw_text(dc, 76, 36 + 60 + i*64 - font2.xftfont->descent - 4, font2, applications[i].comment, dark_white);
	}
	if(!i) {
		draw_text(dc, window_width/2 - draw_string_width(dc, font3, time_string)/2, window_height/2, font3, time_string, white);
		draw_text(dc, window_width/2 - draw_string_width(dc, font1, date_string)/2, window_height/2 + 20 + font1.xftfont->ascent, font1, date_string, white);
	}
	draw_rectangle(dc, 0,0, window_width, 32, bg1);
	draw_image(dc, 4,4, search_icon);
	if(!*query) {
		draw_text(dc, 32,4+font.xftfont->ascent,font1, "type something...", dark_white);
	} else {
		draw_text(dc, 32,4+font.xftfont->ascent,font1, query, white);
	}
	draw_rectangle(dc, 0,0, i*window_width / application_count, 3, red);
	draw_flush_all(dc);
}

void
launch()
{
	static char cmd[256];

	sprintf(cmd, "(gtk-launch %s &)", applications[selected_app].path);
	system(cmd);
	exit(0);
}

int
main()
{
	XEvent e;
	Atom stop_atom;
	KeySym k;
	char s[64];
	int i;
	XEvent fullscreen;
	FILE* datepipe;
	size_t n;

	dc = draw_init(window_width = 800, window_height = 600, "menu", ExposureMask | StructureNotifyMask | KeyPressMask | PointerMotionMask | ButtonPressMask);

	memset(&fullscreen, 0, sizeof(fullscreen));
	fullscreen.xclient = (XClientMessageEvent) { .type = ClientMessage, .window = dc.win, .message_type = XInternAtom(dc.disp, "_NET_WM_STATE", False), .format = 32, .data.l =  { 1, XInternAtom(dc.disp, "_NET_WM_STATE_FULLSCREEN", False), 0 }, };
	XSendEvent(dc.disp, DefaultRootWindow(dc.disp), False, SubstructureNotifyMask | SubstructureRedirectMask, &fullscreen);

	draw_resize(&dc, window_width = draw_width(dc), window_height = draw_height(dc));
	draw_rectangle(dc, 0, 0, window_width, window_height, bg);
	draw_flush_all(dc);

	stop_atom = XInternAtom(dc.disp, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dc.disp, dc.win, &stop_atom, 1);

	bg = create_color(dc, 0x3b,0x42,0x52);
	bg1= create_color(dc, 0x4c,0x56,0x6a);
	white = create_color(dc, 255,255,255);
	dark_white = create_color(dc, 200,200,200);
	yellow = create_color(dc, 0xf4, 0xca, 0x79);
	red = create_color(dc,0xcb,0x68,0x71);
	green = create_color(dc,0xa9,0xcb,0x8c);
	font = load_font(dc, FONT);
	font1 = load_font(dc, FONT_1);
	font2 = load_font(dc, FONT_2);
	font3 = load_font(dc, FONT_3);

	on_exit(exit_safely, NULL);

	*query = 0;
	load_applications();
	search_icon = create_image(dc, bg1, SEARCH_ICON);

	datepipe = popen("date +%H:%M", "r");
	getline(&time_string, &n, datepipe);
	pclose(datepipe);
	for(i = 0; time_string[i]; i++) if (time_string[i] == '\n') time_string[i] = 0;

	datepipe = popen("date +%A\\ %B\\ %d", "r");
	getline(&date_string, &n, datepipe);
	pclose(datepipe);
	for(i = 0; date_string[i]; i++) if (date_string[i] == '\n') date_string[i] = 0;

	for(;;){
		XNextEvent(dc.disp, &e);
		switch(e.type) {
			case Expose:
				if(!e.xexpose.count) redraw();
				break;
			case ConfigureNotify:
				if(e.xconfigure.send_event) break;
				draw_flush_all(dc);
				draw_resize(&dc, window_width = draw_width(dc), window_height = draw_height(dc));
				break;
			case ClientMessage:
				if(e.xclient.data.l[0] == stop_atom) exit(0);
				break;
			case KeyPress:
				XLookupString(&e.xkey, s, 64, &k, NULL);
				if( k == XK_Return ) {
					launch();
				}else if( k == XK_Escape ) {
					exit(0);
				} else if( k == XK_Down ) {
					if(applications[selected_app+1].score != (uint16_t)-1) selected_app++;
				} else if( k == XK_Up ) {
					if(selected_app) selected_app--;
				} else if(k == XK_BackSpace) {
					selected_app = 0;
					i = 0;
					while(query[i]) i++;
					if(i) query[i-1] = 0;
				} else if (!s[1]) {
					selected_app = 0;
					i = 0;
					while(query[i]) i++;
					query[i] = *s;
				}
				sort_applications();
				redraw();
				break;
			case MotionNotify:
				selected_app = (e.xmotion.y-36)/64;
				redraw();
				break;
			case ButtonPress:
				launch();
				break;
		}
	}
}
