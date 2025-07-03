#pragma once

#include "../types.h"

#define MAX_NAME_LEN     64
#define MAX_PATH_LEN    256
#define MAX_OPEN_FILES   64

// File types and mode flags
#define VFS_MODE_DIR  0x4000
#define VFS_MODE_FILE 0x8000

struct inode;
struct file;
struct dentry;
struct filesystem;

// Filesystem operation table
struct filesystem_ops {
    inode_t *(*lookup)(struct inode *dir, const char *name);
    int (*read)(inode_t *inode, ulong offset, void *buf, ulong len);
    int (*write)(inode_t *inode, ulong offset, const void *buf, ulong len);
    int (*create)(inode_t *dir, const char *name, uint mode);
    int (*mkdir)(inode_t *dir, const char *name);
} filesystem_ops_t;

// Core inode structure
struct inode {
    ulong inode_no;
    ulong size;
    uint mode;
    filesystem_t *fs;
    void *internal;  // FS-specific pointer
} inode_t;

// Directory entry
struct dentry {
    char name[MAX_NAME_LEN];
    inode_t *inode;
} dentry_t;

// Open file handle
struct file {
    inode_t *inode;
    ulong offset;
    int flags;
} file_t;

// Filesystem mountpoint
struct filesystem {
    const char *name;
    filesystem_ops_t *ops;
    inode_t *root;
} filesystem_t;

void vfs_init(void);
int vfs_mount(filesystem_t *fs);

inode_t *vfs_lookup(const char *path);
int vfs_open(const char *path);
int vfs_read(int fd, void *buf, ulong len);
int vfs_write(int fd, const void *buf, ulong len);
int vfs_create(const char *path, uint mode);
int vfs_mkdir(const char *path);

// FD helpers
int vfs_install_fd(file_t *f);
file_t *vfs_get_file(int fd);
