#ifndef STDLIB_H
#define STDLIB_H

#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

void __malloc_buf_set(uint32_t buf_size, uint32_t seg_size);
void __malloc_init(void);
void __malloc_close(void);

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void* ptr);
void* realloc(void* s, uint32_t new_size);

void exit(int status);
int execl(const char* fname, const char* arg, ...);
const char* getenv(const char* name);
int setenv(const char* name, const char* value);

int atoi_base(const char *s, int b);
int atoi(const char *s);
int64_t atoll(const char *s);
float atof(const char *s);
uint32_t random(void);

#ifdef __cplusplus
}
#endif

#endif
