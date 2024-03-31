#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <x++/X.h>
#include <unistd.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/proc.h>
#include <elf/elf.h>
#include <tinyjson/tinyjson.h>
#include <upng/upng.h>
#include <dirent.h>

#include <string>
using namespace EwokSTL;
using namespace Ewok;

#define MAX_FILES 1024

class CWDLabel: public Label {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.titleBGColor, true);
		font_t* font = theme->getFont();
		int y = r.y + (r.h-font->max_size.y)/2;
		graph_draw_text_font(g, r.x+4, y, label.c_str(), font, theme->basic.titleColor);
	}
public:
	CWDLabel(const char* label) : Label(label) {}
};

static void freeIcon(void*p) {
	graph_t* g = (graph_t*)p;
	graph_free(g);
}

class FileGrid: public Grid {
	struct dirent files[MAX_FILES];
	char cwd[FS_FULL_NAME_MAX+1];
	var_t* fileTypes;
	static const uint32_t iconSize = 48;
	static const uint32_t titleMargin = 4;
	graph_t* dirIcon;
	graph_t* fileIcon;
	graph_t* devIcon;

	CWDLabel *cwdLabel;

	string getIconFname(const char* appName) {
		//try theme icon first
		string ret = x_get_theme_fname(X_THEME_ROOT, appName, "icon.png");
		if(access(ret.c_str(), R_OK) == 0)
			return ret;

		ret = "/apps/";
		ret = ret + appName + "/res/icon.png";
		return ret;
	}
 
	void drawIcon(graph_t* g, int at, XTheme* theme, int x, int y, int w, int h) {
		int icon_size = iconSize < w ? iconSize : w;
		graph_t* img = NULL;
		if(files[at].d_type == DT_DIR)
			img = dirIcon;
		else if(files[at].d_type == DT_CHR || files[at].d_type == DT_BLK)
			img = devIcon;
		else {
			img = getIcon(files[at].d_name);
			if(img == NULL)
				img = fileIcon;
		}

		int dx = (w - img->w)/2;
		int dy = (h - (int)(img->w + titleMargin + theme->getFont()->max_size.y)) / 2;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x+dx, y+dy, img->w, img->h, 0xff);
	}

	void drawTitle(graph_t* g, int at, XTheme* theme, int x, int y, int w, int h) {
		const char* title = files[at].d_name;
		uint32_t tw, th;
		font_text_size(title, theme->getFont(), &tw, &th);
		int xto = x + (w - (int32_t)tw)/2;
		if(xto > x)
			x = xto;

		y += (h - (int)(iconSize + titleMargin + (int32_t)th)) /2 +
				iconSize + titleMargin;
		graph_draw_text_font(g, x, y, title, theme->getFont(), theme->basic.fgColor);
	}

	void setCWD(const char* r) {
		strcpy(cwd, r);
		if(cwdLabel != NULL)
			cwdLabel->setLabel(cwd);
	}

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
			setCWD("/");

		readDir(cwd);
		update();
	}

	bool check(const char* fname, const char* ext) {
		int i = strlen(fname) - strlen(ext);
		if(strcmp((fname+i), ext) == 0)
			return true;
		return false;
	}

	const char* fileType(const char* fname) {
		if(fileTypes == NULL)
			return "";
		int num = var_array_size(fileTypes);
		for(int i=0; i<num; i++) {
			var_t* it = var_array_get_var(fileTypes, i);
			if(it == NULL)
				break;

			const char* ext = get_str(it, "ext");
			const char* open_with = get_str(it, "open_with");
			if(ext[0] == 0 || open_with[0] == 0)
				break;
			if(check(fname, ext))
				return open_with;
		}
		return "";
	}

	bool check_elf(const char* fname) {
		elf_header_t header;
		if(elf_read_header(fname, &header) != 0)
			return false;

		int pid = fork();
		if(pid == 0)  {
			proc_detach();
			proc_exec(fname);
			exit(0);
		}
		return true;
	}

	void load(const char* fname) {
		if(check_elf(fname))
			return;

		char cmd[FS_FULL_NAME_MAX+1] = "";
		const char* prog = fileType(fname);

		if(prog[0] == 0)
			return;

		snprintf(cmd, FS_FULL_NAME_MAX, "%s %s", prog, fname);
		int pid = fork();
		if(pid == 0)  {
			proc_detach();
			proc_exec(cmd);
			exit(0);
		}
	}

	void readDir(const char* r) {
		DIR* dirp = opendir(r);
		if(dirp == NULL)
			return;
		if(r != cwd) {
			setCWD(r);
		}

		strcpy(files[0].d_name, "..");
		files[0].d_type = DT_DIR;

		int i;
		for(i=1; i<MAX_FILES; i++) {
			struct dirent* it = readdir(dirp);
			if(it == NULL)
				break;
			memcpy(&files[i], it, sizeof(struct dirent));
		}
		closedir(dirp);
		setItemNum(i);
		itemStart = 0;
		itemSelected = -1;
		updateScroller();
	}

	void loadIcons() {
		fileIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "finder", "icons/file.png"));
		if(fileIcon == NULL)
			fileIcon = png_image_new(X::getResName("icons/file.png"));

		dirIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "finder", "icons/folder.png"));
		if(dirIcon == NULL)
			dirIcon = png_image_new(X::getResName("icons/folder.png"));

		devIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "finder", "icons/device.png"));
		if(devIcon == NULL)
			devIcon = png_image_new(X::getResName("icons/device.png"));
	}

	var_t* loadFileTypes(const char* confFile) {
		int sz;
		char* str = (char*)vfs_readfile(confFile, &sz);
		if(str == NULL) 
			return NULL;
		str[sz] = 0;
		var_t* ret = json_parse(str);
		free(str);
		return ret;
	}

	graph_t*  getIcon(const char* fname) {
		if(fileTypes == NULL)
			return NULL;
		int num = var_array_size(fileTypes);
		for(int i=0; i<num; i++) {
			var_t* it = var_array_get_var(fileTypes, i);
			if(it == NULL)
				break;

			const char* ext = get_str(it, "ext");
			const char* icon = get_str(it, "icon");
			if(ext[0] == 0 || icon[0] == 0)
				continue;

			if(check(fname, ext)) {
				graph_t* img = (graph_t*)get_raw(it, "icon_image");
				if(img != NULL)
					return img;

				img = png_image_new(x_get_theme_fname(X_THEME_ROOT, "finder", icon));
				if(img == NULL)
					return NULL;
				var_add(it, "icon_image", var_new_obj(img, freeIcon));
				return img;
			}
		}
		return NULL;
	}

protected:
	void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
	}

	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index >= itemNum)
			return;

		if(index == itemSelected)
			graph_fill_round(g, r.x, r.y, r.w, r.h, 6, theme->basic.selectBGColor);

		drawIcon(g, index, theme, r.x , r.y, r.w, r.h);
		drawTitle(g, index, theme, r.x , r.y, r.w, r.h);
	}

	void onEnter(int i) {
		struct dirent* it = &files[i];
		char fname[FS_FULL_NAME_MAX+1];
		if(strcmp(it->d_name, "..") == 0) {
			upBack();
			return;
		}
		else if(strcmp(cwd, "/") == 0)
			snprintf(fname, FS_FULL_NAME_MAX, "/%s", it->d_name);
		else
			snprintf(fname, FS_FULL_NAME_MAX, "%s/%s", cwd, it->d_name);

		if(it->d_type == DT_DIR) {
			readDir(fname);
			update();
			return;
		}
		else 
			load(fname);
	}

public:
	FileGrid() {
		cwdLabel = NULL;
		scrollerV = NULL;
		loadIcons();
		fileTypes = loadFileTypes("/usr/system/filetypes.json");
		readDir("/");
	}

	void setCWDLabel(CWDLabel* l) {
		cwdLabel = l;
	}
};

int main(int argc, char** argv) {
	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);
	root->setAlpha(false);

	CWDLabel* cwdLabel = new CWDLabel("/");
	cwdLabel->fix(0, 20);
	root->add(cwdLabel);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	root->add(c);

	FileGrid* fgrid = new FileGrid();
	fgrid->setCWDLabel(cwdLabel);
	fgrid->setItemSize(72, 72);
	c->add(fgrid);

	Scroller* scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	c->add(scrollerV);
	fgrid->setScrollerV(scrollerV);

	x.open(0, &win, -1, -1, 320, 240, "xfinder", XWIN_STYLE_NORMAL);
	win.setVisible(true);
	x.run(NULL, &win);
	return 0;
}