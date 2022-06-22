#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <ttf/ttf.h>
#include <x++/X.h>
#include <sys/keydef.h>
#include <dirent.h>


using namespace Ewok;

class Finder: public XWin {
	ttf_font_t* font;
	uint32_t bgColor;
	uint32_t fgColor;
	uint32_t hideColor;
	uint32_t selectColor;
	uint32_t titleColor;
	graph_t* dirIcon;
	graph_t* fileIcon;
	uint32_t itemSize;

	int     selected;
	int     start;
	static const int MAX_FILES = 256;

	char cwd[FS_FULL_NAME_MAX+1];
	struct dirent files[MAX_FILES];
	int nums;

	void upBack(void) {
		if(strcmp(cwd, "/") == 0)
			return;

		int len = strlen(cwd)  - 1;
		for(int i=len; i>=0; i--) {
			if(cwd[i] == '/') {
				cwd[i] = 0;
				break;
			}
		}
		if(cwd[0] == 0)
			strcpy(cwd, "/");

		readDir(cwd);
		repaint(true);
	}

	bool check(const char* fname, const char* ext) {
		int i = strlen(fname) - strlen(ext);
		if(strcmp((fname+i), ext) == 0)
			return true;
		return false;
	}

	void load(const char* fname) {
		char cmd[FS_FULL_NAME_MAX+1] = "";
		if(check(fname, ".nes")) {
			snprintf(cmd, FS_FULL_NAME_MAX, "/apps/emu/emu %s", fname);
		}
		else if(check(fname, ".png")) {
			snprintf(cmd, FS_FULL_NAME_MAX, "/bin/x/png %s", fname);
		}
		if(cmd[0] == 0)
			return;

		int pid = fork();
		if(pid == 0)  {
			exec(cmd);
			exit(0);
		}
	}

	void runProc(int i) {
		struct dirent* it = &files[i];
		char fname[FS_FULL_NAME_MAX+1];
		if(strcmp(cwd, "/") == 0)
			snprintf(fname, FS_FULL_NAME_MAX, "/%s", it->d_name);
		else
			snprintf(fname, FS_FULL_NAME_MAX, "%s/%s", cwd, it->d_name);

		if(it->d_type == DT_DIR) {
			readDir(fname);
			repaint(true);
			return;
		}
		else if(strncmp(fname, "/bin/x/", 7) == 0) {
			int pid = fork();
			if(pid == 0)  {
				exec(fname);
				exit(0);
			}
		}
		else 
			load(fname);
	}

	void readDir(const char* r) {
		DIR* dirp = opendir(r);
		if(dirp == NULL)
			return;
		if(r != cwd)	
			strcpy(cwd, r);
		selected = 0;
		nums = 0;
		start = 0;

		int i;
		for(i=0; i<MAX_FILES; i++) {
			struct dirent* it = readdir(dirp);
			if(it == NULL)
				break;
			memcpy(&files[i], it, sizeof(struct dirent));
		}
		closedir(dirp);
		nums = i;
	}

protected:
	void onRepaint(graph_t* g) {
		char name[FS_FULL_NAME_MAX+1];
		int h = itemSize;
		int yMargin = (itemSize - ttf_font_hight(font))/2;
		int xMargin = 8;

		graph_clear(g, bgColor);
		snprintf(name, FS_FULL_NAME_MAX, "[%s]", cwd);
		if(dirIcon != NULL) {
			int iconMargin = (itemSize - dirIcon->h)/2;
			graph_blt_alpha(dirIcon, 0, 0, dirIcon->w, dirIcon->h,
					g, xMargin, iconMargin, dirIcon->w, dirIcon->h, 0xff);
			xMargin += dirIcon->w + 4;
		}
		graph_draw_text_ttf(g, xMargin, yMargin, name, font, 0, titleColor);

		for(int i=start; i<nums; i++) {
			struct dirent* it = &files[i];
			if(i == selected)
				graph_fill(g, 2, (i+1-start)*h, g->w-2, h, selectColor);

			uint32_t color = fgColor;
			if(it->d_name[0] == '.')
				color = hideColor;

			graph_t* icon = NULL;
			if(it->d_type == DT_DIR)
				icon = dirIcon;
			else
				icon = fileIcon;

			xMargin = 8;
			if(icon != NULL) {
				int iconMargin = (itemSize - dirIcon->h)/2;
				graph_blt_alpha(icon, 0, 0, icon->w, icon->h,
						g, xMargin, (i+1-start)*h+iconMargin, icon->w, icon->h, 0xff);
				xMargin += icon->w + 4;
			}
			graph_draw_text_ttf(g, xMargin, (i+1-start)*h+yMargin, it->d_name, font, 0, color);
		}
	}

	void onEvent(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		int h = itemSize;
		int lines = xinfo.wsr.h/h - 1;
		
		if(ev->type == XEVT_IM) {
			int key = ev->value.im.value;
			if(ev->state == XIM_STATE_PRESS) {
				if(key == KEY_UP)
					selected--;
				else if(key == KEY_DOWN)
					selected++;
				else
					return;
			}
			else {//RELEASE
				if(key == KEY_LEFT || key == KEY_BUTTON_B) {
					upBack();
					return;
				}
				else if(key == KEY_RIGHT || key == KEY_ENTER) {
					runProc(selected);
					return;
				}
			}

			if(selected >= nums)
				selected = nums-1;
			if(selected < 0)
				selected = 0;

			if(selected < start) {
				start -= lines;
				if(start < 0)
					start = 0;
			}
			else if((selected - start) >= lines) 
				start += lines - 1;
			repaint(true);
		}
	}
public:
	inline Finder() {
		bgColor = 0xff000000;
		fgColor = 0xffffffff;
		hideColor = 0xff888888;
		selectColor = 0xff444444;
		titleColor = 0xffffff00;
		fileIcon = png_image_new("/data/icons/system/32/file.png");
		dirIcon = png_image_new("/data/icons/system/32/dir.png");
		itemSize = 36;

		selected = 0;
		start = 0;
		font = NULL;
		readDir("/");
	}

	inline ~Finder() {
		if(fileIcon != NULL)
			graph_free(fileIcon);
		if(dirIcon != NULL)
			graph_free(dirIcon);
		if(font != NULL)
			ttf_font_free(font);
	}

	bool readConfig(const char* fname) {
		sconf_t *conf = sconf_load(fname);	
		if(conf == NULL)
			return false;

		const char* v = sconf_get(conf, "font");
		if(v[0] != 0)
			font = ttf_font_load(v, 12);
		else
			font = ttf_font_load("/data/fonts/system.ttf", 12);

		v = sconf_get(conf, "item_size");
		if(v[0] != 0)
			itemSize = atoi(v);
		v = sconf_get(conf, "bg_color");
		if(v[0] != 0)
			bgColor = atoi_base(v, 16);
		v = sconf_get(conf, "fg_color");
		if(v[0] != 0)
			fgColor = atoi_base(v, 16);
		v = sconf_get(conf, "select_color");
		if(v[0] != 0)
			selectColor = atoi_base(v, 16);
		v = sconf_get(conf, "hide_color");
		if(v[0] != 0)
			hideColor = atoi_base(v, 16);
		v = sconf_get(conf, "title_color");
		if(v[0] != 0)
			titleColor = atoi_base(v, 16);
		v = sconf_get(conf, "file_icon");
		if(v[0] != 0)
			fileIcon = png_image_new(v);
		v = sconf_get(conf, "dir_icon");
		if(v[0] != 0)
			dirIcon = png_image_new(v);

		sconf_free(conf);
		return true;
	}
};

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	Finder xwin;
	xwin.readConfig("/etc/x/finder.conf");

	X x;
	x.open(&xwin, 0,
			0,
			300,
			200,
			"finder",
			X_STYLE_NORMAL);

	xwin.setVisible(true);
	x.run(NULL);
	return 0;
}
