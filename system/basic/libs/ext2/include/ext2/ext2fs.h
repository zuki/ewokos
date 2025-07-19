#ifndef EXT2_FS_H
#define EXT2_FS_H

#include <ext2/ext2head.h>

int32_t ext2_init(ext2_t* ext2, read_block_func_t read_block, write_block_func_t write_block, uint32_t buffer_size);

void ext2_quit(ext2_t* ext2);

int32_t ext2_rmdir(ext2_t* ext2, const char* fname);

int32_t ext2_unlink(ext2_t* ext2, const char* fname);

int32_t ext2_read(ext2_t* ext2, INODE* node, char *buf, int32_t nbytes, int32_t offset);

int32_t ext2_write(ext2_t* ext2, INODE* node, const char *data, int32_t nbytes, int32_t offset);

uint32_t ext2_ino_by_fname(ext2_t* ext2, const char* fname);

int32_t ext2_node_by_fname(ext2_t* ext2, const char* fname, INODE* node);

int32_t ext2_node_by_ino(ext2_t* ext2, uint32_t ino, INODE* node);

int32_t put_node(ext2_t* ext2, uint32_t ino, INODE *node);

int32_t ext2_create_dir(ext2_t* ext2, INODE* father_inp, const char *base,
		uint16_t uid, uint16_t gid, uint16_t mode);

int32_t ext2_create_file(ext2_t* ext2, INODE* father_inp, const char *base,
		uint16_t uid, uint16_t gid, uint16_t mode);

void*   ext2_readfile(ext2_t* ext2, const char* fname, int32_t* size);

#endif
