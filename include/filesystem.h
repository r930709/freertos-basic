#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include <stdint.h>
#include <hash-djb2.h>

#define MAX_FS 16
#define OPENFAIL (-1)

#define OPENDIR_NOTFOUNDFS (-2)
#define OPENDIR_NOTFOUND (-1)

typedef int (*fs_open_t)(void * opaque, const char * fname, int flags, int mode);
typedef int (*fs_open_dir_t)(void * opaque, const char * fname);
//add

typedef int (*fs_check_dir_t)(void * opaque, const char * fname);

/* Need to be called before using any other fs functions */
__attribute__((constructor)) void fs_init();

int register_fs(const char * mountpoint, fs_open_t callback, fs_open_dir_t dir_callback, void * opaque);
//int register_fs(const char * mountpoint, fs_open_t callback, fs_open_dir_t dir_callback, fs_check_dir_t check_callback, void * opaque); //add,colin code
int fs_open(const char * path, int flags, int mode);
int fs_opendir(const char * path);
//int fs_checkdir(const char * path);  //add,colin code

#endif
