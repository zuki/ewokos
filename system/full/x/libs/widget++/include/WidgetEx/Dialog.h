#ifndef WIDGET_DIALOG_HH
#define WIDGET_DIALOG_HH

#include <Widget/WidgetWin.h>
#include <string>

using namespace EwokSTL;
namespace Ewok {


class Dialog: public WidgetWin {
protected:
	XWin* owner;
public:
	static const int RES_CANCEL = 0;
	static const int RES_OK = 1;

	Dialog();

	void submit(int res);
	bool popup(XWin* owner, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style);
	bool popup(XWin* owner, uint32_t w, uint32_t h, const char* title, uint32_t style);
};

}

#endif
