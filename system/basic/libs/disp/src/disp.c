#include <disp/disp.h>
#include <sys/vdevice.h>
#include <string.h>
#include <unistd.h>

const char* get_disp_fb_dev(const char* disp_man_dev, uint32_t disp_index) {
	static char ret[128];
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->addi(&in, disp_index);

	if(dev_cntl(disp_man_dev, DISP_GET_DISP_DEV, &in, &out) == 0)
		strncpy(ret, proto_read_str(&out), 127);
	else
		strncpy(ret, "/dev/fb0", 127);

	PF->clear(&in);
	PF->clear(&out);
	return ret;
}

uint32_t get_disp_num(const char* disp_man_dev) {
	proto_t out;
	PF->init(&out);
	uint32_t ret = 0;

	if(dev_cntl(disp_man_dev, DISP_GET_DISP_NUM, NULL, &out) == 0)
		ret = proto_read_int(&out);
	PF->clear(&out);
	return ret;
}