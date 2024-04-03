#include <Widget/Label.h>
#include <x++/XTheme.h>

using namespace EwokSTL;
namespace Ewok {

void Label::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	if(theme->getFont() == NULL)
		return;
	if(!alpha)
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
	graph_draw_text_font_align(g, r.x+marginH, r.y+marginV, r.w-marginH*2, r.h-marginV*2,
				label.c_str(), theme->getFont(), theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);
}

Label::Label(const string& str) {
	label = str;
	alpha = true;
}

Label::~Label(void) {
}

void Label::setLabel(const string& str) {
	label = str;
	update();
}

}