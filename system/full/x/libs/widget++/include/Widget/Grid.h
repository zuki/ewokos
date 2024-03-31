#ifndef WIDGET_GRID_HH
#define WIDGET_GRID_HH

#include <Widget/ListBase.h>

namespace Ewok {

class Grid: public ListBase {
protected:
	uint32_t itemW;
	uint32_t itemH;
	uint32_t rows;
	uint32_t cols;

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	void onResize();
	bool onMouse(xevent_t* ev);
	bool onKey(xevent_t* ev);

	virtual void onScroll(int step);
public:
	Grid();
	~Grid(void);

	void setItemSize(uint32_t itemW, uint32_t itemH);
	inline gsize_t getItemSize() { gsize_t r = {itemW, itemH}; return r;}
};

}

#endif
