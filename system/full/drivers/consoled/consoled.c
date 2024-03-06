#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <gterminal/gterminal.h>
#include <fb/fb.h>
#include <ewoksys/vdevice.h>
#include <display/display.h>
#include <upng/upng.h>
#include <sconf/sconf.h>
#include <ewoksys/klog.h>
#include <ewoksys/syscall.h>
#include <ewoksys/core.h>
#include <ewoksys/charbuf.h>
#include <sysinfo.h>
#include <font/font.h>

typedef struct {
	const char* id;
	const char* display_dev;
	uint32_t    display_index;
	fb_t        fb;
	graph_t*    g;
	gterminal_t terminal;
	graph_t*    icon;
} fb_console_t;

static int32_t read_config(fb_console_t* console, const char* fname) {
	const char* font_fname =  DEFAULT_SYSTEM_FONT;
	uint32_t font_size = 12;
	console->terminal.fg_color = 0xffcccccc;
	console->terminal.bg_color = 0xff000000;
	console->terminal.font_fixed = font_size;

	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL) {
		console->terminal.font = font_new(font_fname, font_size, true);
		return -1;
	}	

	const char* v = sconf_get(conf, "icon");
	if(v[0] != 0) 
		console->icon = png_image_new(v);

	v = sconf_get(conf, "bg_color");
	if(v[0] != 0) 
		console->terminal.bg_color = strtoul(v, NULL, 16);

	v = sconf_get(conf, "fg_color");
	if(v[0] != 0) 
		console->terminal.fg_color = strtoul(v, NULL, 16);
	
	v = sconf_get(conf, "font_size");
	if(v[0] != 0) 
		font_size = atoi(v);
	console->terminal.font_size = font_size;

	v = sconf_get(conf, "font_fixed");
	if(v[0] != 0) 
		console->terminal.font_fixed = atoi(v);

	v = sconf_get(conf, "font");
	if(v[0] != 0)
		font_fname = v;

	if(console->terminal.font != NULL)
		font_free(console->terminal.font);
	console->terminal.font = font_new(font_fname, font_size, true);
	sconf_free(conf);
	return 0;
}

static void init_graph(fb_console_t* console) {
	console->g = fb_fetch_graph(&console->fb);
	graph_clear(console->g, 0xff000000);
}

static int init_console(fb_console_t* console, const char* display_dev, const uint32_t display_index) {
	memset(console, 0, sizeof(fb_console_t));
	font_init();

	console->display_dev = display_dev;
	console->display_index = display_index;
	const char* fb_dev = get_display_fb_dev(display_dev, console->display_index);
	if(fb_open(fb_dev, &console->fb) != 0)
		return -1;
	init_graph(console);
	gterminal_init(&console->terminal);
	read_config(console, "/etc/console.conf");
	return 0;
}

static void close_console(fb_console_t* console) {
	if(console->g != NULL)
		graph_free(console->g);

	fb_close(&console->fb);
	if(console->icon != NULL)
		graph_free(console->icon);
	gterminal_close(&console->terminal);
}

static int reset_console(fb_console_t* console) {
	gterminal_resize(&console->terminal, console->g->w, console->g->h);
	return 0;
}

static void draw_bg(fb_console_t* console) {
	graph_t* g = console->g;
	graph_clear(g, console->terminal.bg_color);
}

static void flush(fb_console_t* console) {
	draw_bg(console);

	if(console->display_index == 0) {
		//draw cores
		if(console->icon != NULL) {
			sys_info_t sys_info;
			syscall1(SYS_GET_SYS_INFO, (int32_t)&sys_info);
			for(uint32_t i=0; i<sys_info.cores; i++) {
				graph_blt_alpha(console->icon, 
						0, 0, console->icon->w, console->icon->w,
						console->g,
						console->g->w - console->icon->w * (i+1),
						console->g->h - console->icon->h,
						console->icon->w, console->icon->h, 
						0xff);
			}	
		}
	}

	gterminal_paint(&console->terminal, console->g);
	fb_flush(&console->fb, true);
}

static int _ux_index = 0;
static int console_write(int fd, 
		int from_pid,
		fsinfo_t* node,
		const void* buf,
		int size,
		int offset,
		void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;

	fb_console_t* console = (fb_console_t*)p;
	if(size <= 0 || console->g == NULL)
		return 0;

	const char* pb = (const char*)buf;
	gterminal_put(&console->terminal, pb, size);

	if(_ux_index == core_get_ux())
		flush(console);
	return size;
}

static int _keyb_fd = -1;
static const char* _keyb_dev = "/dev/keyb0";
static charbuf_t *_buffer;

static int console_read(int fd,
		int from_pid,
		fsinfo_t* node,
		void* buf,
		int size,
		int offset,
		void* p) {

	char c;
	int res = charbuf_pop(_buffer, &c);

	if(res != 0)
		return VFS_ERR_RETRY;

	((char*)buf)[0] = c;
	return 1;
}

static bool _flush = true;
static int console_loop(void* p) {
	if(_ux_index != core_get_ux()) {
		usleep(200000);
		_flush = true;
		return 0;
	}

	if(_flush) {
		flush((fb_console_t*)p);
		_flush = false;
	}

	if(_keyb_fd < 0) {
		_keyb_fd = open(_keyb_dev, O_RDONLY | O_NONBLOCK);
		if(_keyb_fd < 0) {
			usleep(200000);
			return 0;
		}
	}

	char c = 0;
	if(read(_keyb_fd, &c, 1) == 1 && c != 0) {
		charbuf_push(_buffer, c, true);
		proc_wakeup(RW_BLOCK_EVT);
	}

	usleep(20000);
	return 0;
}

int main(int argc, char** argv) {
	_buffer = charbuf_new(0);
	_ux_index = 0;
	if(argc > 1)
		_ux_index = atoi(argv[1]);
	char mnt_point[128];
	snprintf(mnt_point, 127, "/dev/console%d", _ux_index);

	_keyb_fd = -1;
	_keyb_dev = argc > 2 ? argv[2]: "/dev/keyb0";

	const char* display_dev = argc > 3 ? argv[3]: "/dev/display";
	const uint32_t display_index = argc > 4 ? atoi(argv[4]): 0;

	fb_console_t _console;
	init_console(&_console, display_dev, display_index);
	reset_console(&_console);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "console");
	dev.write = console_write;
	dev.read = console_read;
	dev.extra_data = &_console;
	dev.loop_step = console_loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	close_console(&_console);
	charbuf_free(_buffer);
	return 0;
}
