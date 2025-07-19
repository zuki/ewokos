#ifndef MARIO_STRING
#define MARIO_STRING

#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
string functions.
*/
typedef struct st_str {
	char* cstr;
	uint32_t max;
	uint32_t len;
} str_t;

void str_reset(str_t* str);
char* str_detach(str_t* str);
char* str_ncpy(str_t* str, const char* src, uint32_t l);
char* str_cpy(str_t* str, const char* src);
str_t* str_new(const char* s);
str_t* str_new_by_size(uint32_t sz);
char* str_add(str_t* str, const char* src);
char* str_addc(str_t* str, char c);
char* str_add_int(str_t* str, int i, int base);
char* str_add_float(str_t* str, float f);
void str_free(str_t* str);
const char* str_from_int(int i, int base);
const char* str_from_float(float f);
const char* str_from_bool(bool b);
int str_to_int(const char* str);
bool str_to_bool(const char* str);
float str_to_float(const char* str);
int str_to(const char* str, char c, str_t* res, uint8_t skipspace);
/*
str_t* str_format(str_t* str, const char *format, ...);
str_t* str_format_new(const char *format, ...);
*/

#define CS(s) ((s)->cstr)

#ifdef __cplusplus
}
#endif

#endif
