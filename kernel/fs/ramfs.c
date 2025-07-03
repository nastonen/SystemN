#include "ramfs.h"

struct ramfs_file {
    char name[MAX_NAME_LEN];
    char *data;
    ulong size;
    uint mode;  // VFS_MODE_FILE or VFS_MODE_DIR
    struct inode* inode;
} ramfs_file_t;

static ramfs_file_t file_table[MAX_OPEN_FILES];
static ulong next_inode_no = 1;

static filesystem_t ramfs_fs;
static inode_t root_inode;

static ramfs_file_t *
ramfs_find(const char *name)
{
    for (int i = 0; i < RAMFS_MAX_FILES; i++)
        if (file_table[i].inode && strcmp(file_table[i].name, name) == 0)
            return &file_table[i];

    return NULL;
}

static inode_t *
ramfs_lookup(inode_t *dir, const char *name)
{
    ramfs_file_t *f = ramfs_find(name);
    return f ? f->inode : NULL;
}

static int
ramfs_read(inode_t *inode, ulong offset, void *buf, ulong len)
{
    ramfs_file_t *f = (ramfs_file_t *)inode->internal;
    if (!f || f->mode != VFS_MODE_FILE)
        return -1;

    if (offset >= f->size)
        return 0;

    ulong to_read = (offset + len > f->size) ? (f->size - offset) : len;
    memcpy(buf, f->data + offset, to_read);

    return to_read;
}

static int
ramfs_write(inode_t *inode, ulong offset, const void *buf, ulong len)
{
    ramfs_file_t *f = (ramfs_file_t *)inode->internal;
    if (!f || f->mode != VFS_MODE_FILE)
        return -1;

    ulong new_size = offset + len;
    if (new_size > f->size) {
        char *new_data = realloc(f->data, new_size);
        if (!new_data)
            return -1;

        f->data = new_data;
        f->size = new_size;
        inode->size = new_size;
    }

    memcpy(f->data + offset, buf, len);

    return len;
}

static int
ramfs_create(inode_t *dir, const char *name, uint mode)
{
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!file_table[i].inode) {
            ramfs_file_t *f = &file_table[i];
            strncpy(f->name, name, RAMFS_NAME_LEN);
            f->mode = mode;
            f->data = NULL;
            f->size = 0;

            inode_t *inode = malloc(sizeof(struct inode));
            if (!inode)
                return -1;

            inode->inode_no = next_inode_no++;
            inode->mode = mode;
            inode->size = 0;
            inode->fs = &ramfs_fs;
            inode->internal = f;

            f->inode = inode;

            return 0;
        }
    }

    return -1; // No space
}

static int
ramfs_mkdir(struct inode *dir, const char *name)
{
    // For now, don't support nested directories
    return -1;
}

static filesystem_ops_t ramfs_ops = {
    .lookup = ramfs_lookup,
    .read   = ramfs_read,
    .write  = ramfs_write,
    .create = ramfs_create,
    .mkdir  = ramfs_mkdir
};

filesystem_t *
ramfs_create(void)
{
    memset(&file_table, 0, sizeof(file_table));
    memset(&root_inode, 0, sizeof(inode_t));

    root_inode.inode_no = 0;
    root_inode.mode = VFS_MODE_DIR;
    root_inode.size = 0;
    root_inode.fs = &ramfs_fs;
    root_inode.internal = NULL;

    ramfs_fs.name = "ramfs";
    ramfs_fs.ops = &ramfs_ops;
    ramfs_fs.root = &root_inode;

    return &ramfs_fs;
}
