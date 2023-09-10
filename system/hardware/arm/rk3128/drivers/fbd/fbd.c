#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <arch/rk3128/framebuffer.h>

int argv2rgb(uint8_t  *out,  uint32_t *in , int w, int h)
{
	for(int i = 0; i < w*h; i++){
		 register uint32_t color = *in++;
		 *out++=color ;
		 *out++=color >> 8;
		 *out++=color >> 16;
	}	

	return 0;
}


static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	(void)size;
	(void)rotate;

	//return argv2rgb((uint8_t*)fbinfo->pointer, (uint32_t*)buf, fbinfo->width, fbinfo->height);
	return memcpy((uint8_t*)fbinfo->pointer, (uint32_t*)buf, fbinfo->width * fbinfo->height * 4);
}

static fbinfo_t* get_info(void) {
	return rk3128_get_fbinfo();
}

static int32_t init(uint32_t w, uint32_t h, uint32_t dep) {
	return rk3128_fb_init(w, h, dep);
}

int main(int argc, char** argv) {
	fbd_t fbd;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";
	uint32_t rotate = argc > 4 ? atoi(argv[4]): G_ROTATE_NONE;
	uint32_t w = 640;
	uint32_t h = 480;

	if(argc > 3) {
		w = atoi(argv[2]);
		h = atoi(argv[3]);
	}

	fbd.splash = NULL;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;

	return fbd_run(&fbd, mnt_point, w, h, rotate);
}
