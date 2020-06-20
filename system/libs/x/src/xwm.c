#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/proc.h>
#include <sys/vdevice.h>
#include <graph/graph.h>
#include <x/xcntl.h>
#include <x/xwm.h>

static void draw_drag_frame(xwm_t* xwm, proto_t* in) {
	grect_t r;
	int shm_id = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));
	void* gbuf = shm_map(shm_id);
	if(gbuf != NULL) {
		graph_t* g = graph_new(gbuf, xw, xh);
		if(xwm->draw_drag_frame != NULL)
			xwm->draw_drag_frame(g, &r, xwm->data);
		else
			graph_box(g, r.x, r.y, r.w, r.h, 0xffffffff);
		graph_free(g);
		shm_unmap(shm_id);
	}
}

static void draw_frame(xwm_t* xwm, proto_t* in) {
	xinfo_t info;
	int shm_id = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);
	proto_read_to(in, &info, sizeof(xinfo_t));
	bool top = (proto_read_int(in) != 0);

	if(xw <= 0 || xh <= 0)
		return;

	void* gbuf = shm_map(shm_id);
	if(gbuf != NULL) {
		grect_t rtitle, rclose, rmax, rmin, rresize;
		memset(&rtitle, 0, sizeof(grect_t));
		memset(&rclose, 0, sizeof(grect_t));
		memset(&rmax, 0, sizeof(grect_t));
		memset(&rmin, 0, sizeof(grect_t));
		memset(&rresize, 0, sizeof(grect_t));

		if(xwm->get_title != NULL)
			xwm->get_title(&info, &rtitle, xwm->data);
		if(xwm->get_close != NULL)
			xwm->get_close(&info, &rclose, xwm->data);
		if(xwm->get_max != NULL)
			xwm->get_max(&info, &rmax, xwm->data);
		if(xwm->get_min != NULL)
			xwm->get_min(&info, &rmin, xwm->data);
		if(xwm->get_resize != NULL)
			xwm->get_resize(&info, &rresize, xwm->data);

		graph_t* g = graph_new(gbuf, xw, xh);

		if((info.style & X_STYLE_NO_TITLE) == 0) {
			if(xwm->draw_title != NULL && rtitle.w > 0 && rtitle.h > 0)
				xwm->draw_title(g, &info, &rtitle, top, xwm->data);

			if((info.style & X_STYLE_NO_RESIZE) == 0) {
				if(xwm->draw_min != NULL && rmin.w > 0 && rmin.h > 0)
					xwm->draw_min(g, &info, &rmin, top, xwm->data);
				if(xwm->draw_max != NULL && rmax.w > 0 && rmax.h > 0)
					xwm->draw_max(g, &info, &rmax, top, xwm->data);
				if(xwm->draw_resize != NULL && rresize.w > 0 && rresize.h > 0)
					xwm->draw_resize(g, &info, &rresize, top, xwm->data);
			}
			if(xwm->draw_close != NULL && rclose.w > 0 && rclose.h > 0)
				xwm->draw_close(g, &info, &rclose, top, xwm->data);
		}
		if(xwm->draw_frame != NULL)
			xwm->draw_frame(g, &info, top, xwm->data);

		graph_free(g);
		shm_unmap(shm_id);
	}
}

static void get_pos(xwm_t* xwm, proto_t* in, proto_t* out) {
	xinfo_t info;
	int x = proto_read_int(in);
	int y = proto_read_int(in);
	proto_read_to(in, &info, sizeof(xinfo_t));

	int res = -1;
	if((info.style & X_STYLE_NO_TITLE) == 0 &&
			(info.style & X_STYLE_NO_FRAME) == 0) {
		grect_t rtitle, rclose, rmax, rmin, rresize;
		memset(&rtitle, 0, sizeof(grect_t));
		memset(&rclose, 0, sizeof(grect_t));
		memset(&rmax, 0, sizeof(grect_t));
		memset(&rmin, 0, sizeof(grect_t));
		memset(&rresize, 0, sizeof(grect_t));

		if(xwm->get_title != NULL)
			xwm->get_title(&info, &rtitle, xwm->data);
		if(xwm->get_close != NULL)
			xwm->get_close(&info, &rclose, xwm->data);
		if(xwm->get_max != NULL)
			xwm->get_max(&info, &rmax, xwm->data);
		if(xwm->get_min != NULL)
			xwm->get_min(&info, &rmin, xwm->data);
		if(xwm->get_resize != NULL)
			xwm->get_resize(&info, &rresize, xwm->data);

		if(check_in_rect(x, y, &rtitle) == 0)
			res = XWM_FRAME_TITLE;
		else if(check_in_rect(x, y, &rclose) == 0)
			res = XWM_FRAME_CLOSE;
		else if((info.style & X_STYLE_NO_RESIZE) == 0) {
			if(check_in_rect(x, y, &rmax) == 0)
				res = XWM_FRAME_MAX;
			else if(check_in_rect(x, y, &rresize) == 0) {
				res = XWM_FRAME_RESIZE;
			}
		}
	}
	PF->addi(out, res);
}

static void draw_desktop(xwm_t* xwm, proto_t* in) {
	int shm_id = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);

	void* gbuf = shm_map(shm_id);
	if(gbuf != NULL) {
		graph_t* g = graph_new(gbuf, xw, xh);
		if(xwm->draw_desktop != NULL)
			xwm->draw_desktop(g, xwm->data);
		graph_free(g);
		shm_unmap(shm_id);
	}
}

static void get_workspace(xwm_t* xwm, proto_t* in, proto_t* out) {
	grect_t r, wsr;
	int style = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));
	memcpy(&wsr, &r, sizeof(grect_t));
	
	if(xwm->get_workspace != NULL)
		xwm->get_workspace(style, &r, &wsr, xwm->data);

	PF->add(out, &wsr, sizeof(grect_t));
}

static void handle(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)from_pid;
	xwm_t* xwm = (xwm_t*)p;

	if(cmd == XWM_CNTL_DRAW_FRAME) { //draw frame
		draw_frame(xwm, in);
	}
	else if(cmd == XWM_CNTL_DRAW_DRAG_FRAME) {
		draw_drag_frame(xwm, in);
	}
	else if(cmd == XWM_CNTL_DRAW_DESKTOP) { //draw desktop
		draw_desktop(xwm, in);
	}
	else if(cmd == XWM_CNTL_GET_POS) { //get pos
		get_pos(xwm, in, out);
	}
	else if(cmd == XWM_CNTL_GET_WORKSPACE) { //get workspace
		get_workspace(xwm, in, out);
	}
}

void xwm_run(xwm_t* xwm) {
	setenv("XWM", "true");
	dev_cntl("/dev/x", X_DCNTL_SET_XWM, NULL, NULL);
	ipc_serv_run(handle, xwm, false);
	while(true) {
		sleep(1);
	}
}
