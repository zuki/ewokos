#include <Widget/WidgetWin.h>
#include <Widget/LabelButton.h>
#include <x++/X.h>
#include <unistd.h>
#include <stdlib.h>
#include <font/font.h>
#include <upng/upng.h>
#include <ewoksys/sys.h>
#include <ewoksys/session.h>
#include <ewoksys/syscall.h>
#include <procinfo.h>
#include <arch_context.h>

using namespace Ewok;

struct Trace {
	int32_t pid;
	context_t ctx;
};

static bool _hold = false;

class Cores : public Widget {
	uint32_t index;
	sys_info_t sysInfo;
	Trace *traces;
	int fps;
	int tracesNum;

	int x_off;
	int y_off_bottom;
	int y_off;
	gpos_t mousePos;

	static const uint32_t COLOR_NUM = 7;
	const  uint32_t colors[COLOR_NUM] = {
		0xff0000ff, 
		0xff00ff00, 
		0xffff0000, 
		0xff8800ff,
		0xff0088ff,
		0xffff8800,
		0xff000000
	};

	void dump_ctx(int pid, context_t *ctx) {
		printf("PID:  %d\n", pid);
		printf("ctx dump:\n"
			"  cpsr=0x%x\n"
			"  pc=0x%x\n"
			"  sp=0x%x\n"
			"  lr=0x%x\n",
			ctx->cpsr,
			ctx->pc,
			ctx->sp,
			ctx->lr);

		uint32_t i;
		for(i=0; i<13; i++) 
			printf("  r%d: 0x%x\n", i, ctx->gpr[i]);
	}

	void pitch(int x, int y) {
		grect_t r = {area.x, area.y, area.w, area.h};
		for(int core=0; core<sysInfo.cores; core++) {
			uint32_t line = 8;
			uint32_t seg = fps/line;

			uint32_t w = (float)(r.w-x_off*2)/(float)seg;
			uint32_t h = 12;
			int x = r.x + x_off;
			int y = r.y + core*h + y_off;

			for(uint32_t l=0; l < line; l++) {
				for(uint32_t trace=0; trace < seg; trace++) {
					int i = core*fps+ seg*l +trace;
					if((seg*l +trace) < tracesNum) {
						int pid = traces[i].pid;
						if(pid >= 0) {
							if(mousePos.x > (x + trace*w) && mousePos.x < (x+trace*w+w) &&
									mousePos.y > y && mousePos.y < (y+h-2)) {
								dump_ctx(pid, &traces[i].ctx);
								return;
							}
						}
					}
				}
				y += sysInfo.cores*h+10;
			}
		}
	}
public:
	inline Cores() {
		index = 0;
		x_off = 4;
		y_off = 24;
		y_off_bottom = 4;
		tracesNum = 0;
		mousePos.x = 0;
		mousePos.y = 0;
    	fps = syscall0(SYS_GET_TRACE_FPS);
		sys_get_sys_info(&sysInfo);
		traces = (Trace*)malloc(sysInfo.cores * fps * sizeof(Trace));
	}
	
	inline ~Cores() {
		if(traces != NULL)
			free(traces);
	}

	void updateCores() {
		sys_get_sys_info(&sysInfo);
		if(traces != NULL)
	    	tracesNum = syscall1(SYS_GET_TRACE, (int)traces);
		update();
	}

protected:
	void drawTitle(graph_t* g, XTheme* theme, uint32_t core, uint32_t color, const grect_t& r) {
		int x = r.x + x_off + core*60;
		graph_fill(g, x, r.y+4, 10, 10, color);
		char s[16];
		snprintf(s, 15, "%d:%d%%", core, 100 - (sysInfo.core_idles[core]/10000));
		graph_draw_text_font(g, x+12, r.y+4, s, theme->getFont(), theme->basic.fontSize, color);
	}

	void drawChat(graph_t* g, XTheme* theme, uint32_t core, uint32_t color, const grect_t& r) {
		uint32_t line = 8;
		uint32_t seg = fps/line;

		uint32_t w = (float)(r.w-x_off*2)/(float)seg;
		uint32_t h = 12;
		int x = r.x + x_off;
		int y = r.y + core*h + y_off;

		for(uint32_t l=0; l < line; l++) {
			graph_fill(g, x, y, w*seg, (h-2), 0xffffffff);
		
			for(uint32_t trace=0; trace < seg; trace++) {
				int i = core*fps+ seg*l +trace;
				if((seg*l +trace) < tracesNum) {
					int pid = traces[i].pid;
					if(pid >= 0) {
						char s[16];
						snprintf(s, 15, "%d", pid);

						graph_fill(g, x + trace*w, y, w, h-2, color);
						if(w >= 16) {
							graph_draw_text_font_align(g, x + trace*w, y, w, h, s, theme->getFont(), 8, 0xffffffff, FONT_ALIGN_CENTER);
							graph_box(g, x + trace*w, y, w, h-2, theme->basic.widgetBGColor);
						}
					}
				}
			}
			y += sysInfo.cores*h+10;
		}
	}

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		if(sysInfo.cores == 0 || traces == NULL)
			return;

		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.widgetBGColor, true);

		for(uint32_t i=0; i<sysInfo.cores; i++) {
			uint32_t color = colors[i%COLOR_NUM];
			drawTitle(g, theme, i, color, r);
			drawChat(g, theme, i, color, r);
		}
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		if(!_hold)
			updateCores();
	}

	void onClick(xevent_t* ev) {
		gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
		pos = getRootPos(pos.x, pos.y);
		mousePos.x = pos.x;
		mousePos.y = pos.y;
		pitch(mousePos.x, mousePos.y);
		update();
	}
};

static void onHoldClickFunc(Widget* wd) {
	_hold = !_hold;
}

int main(int argc, char** argv) {
	_hold = false;
	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::VERTICLE);
	root->setAlpha(false);

	Cores* cores = new Cores();
	root->add(cores);

	LabelButton* holdButton = new LabelButton("hold");
	holdButton->onClickFunc = onHoldClickFunc;
	holdButton->fix(0, 20);
	root->add(holdButton);

	win.open(&x, 0, -1, -1, 640, 480, "xtrace", XWIN_STYLE_NORMAL);
	win.setTimer(1);
	x.run(NULL, &win);
	return 0;
}