#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/Text.h>
#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <sys/basic_math.h>
#include <sys/klog.h>

using namespace Ewok;

class MyButton: public LabelButton {
protected:
	void onClick() {
		klog("click %s!\n", label.c_str());
	}

public: 
	MyButton(const string& label = "") : LabelButton(label) {
	}
};


/*static void loop(void* p) {
	WidgetWin* xwin = (WidgetWin*)p;
	xwin->repaint();
	usleep(1000000);
}
*/

int main(int argc, char** argv) {
	X x;
	WidgetWin win;
	win.setRoot(new RootWidget());
	win.getRoot()->setAlpha(false);

	Container* c = new Container();
	c->setType(Container::HORIZONTAL);
	win.getRoot()->add(c);

	Widget* wd = new Image("/usr/system/images/mac1984.png");
	c->add(wd);

	Text* txt = new Text("text\nHello world\n[中文测试]\n123～！@");
	Theme* theme = new Theme(font_new("/usr/system/fonts/system_cn.ttf", 18, true));
	theme->bgColor = 0xff000000;
	theme->fgColor = 0xffffaa88;
	txt->setTheme(theme);
	c->add(txt);

	c = new Container();
	c->setType(Container::HORIZONTAL);
	c->fix(0, 40);
	win.getRoot()->add(c);

	Label* label = new Label("Label");
	theme = new Theme(font_new("/usr/system/fonts/system.ttf", 32, true));
	label->setTheme(theme);
	c->add(label);

	wd = new MyButton("test");
	c->add(wd);

	wd = new MyButton("disable");
	wd->disable();
	c->add(wd);

	win.getRoot()->setType(Container::VERTICLE);

	x.open(0, &win, 400, 300, "widgetTest", XWIN_STYLE_NORMAL);
	win.setVisible(true);
	//x.run(loop, &win);
	x.run(NULL, &win);
	return 0;
}