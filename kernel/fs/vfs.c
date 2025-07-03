#include "vfs.h"

static filesystem_t *root_fs;
static file_t *open_files[MAX_OPEN_FILES];

int
vfs_install_fd(file_t *f)
{
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i] == NULL) {
            open_files[i] = f;
            return i;
        }
    }

    return -1;
}

file_t *
vfs_get_file(int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES)
        return NULL;

    return open_files[fd];
}

void
vfs_init(void)
{
    // Currently nothing
}

int
vfs_mount(filesystem_t *fs)
{
    if (!fs || !fs->root)
        return -1;

    root_fs = fs;

    return 0;
}

inode_t *
vfs_lookup(const char *path)
{
    if (!path || path[0] != '/' || !root_fs)
        return NULL;

    inode_t *curr = root_fs->root;
    char *token = strtok(temp, "/");
    char temp[MAX_PATH_LEN];

    strncpy(temp, path, MAX_PATH_LEN);

    while (token && curr && (curr->mode & VFS_MODE_DIR)) {
        curr = curr->fs->ops->lookup(curr, token);
        token = strtok(NULL, "/");
    }

    return curr;
}

int
vfs_open(const char *path)
{
    inode_t *node = vfs_lookup(path);
    if (!node)
        return -1;

    file_t *f = malloc(sizeof(file_t));
    if (!f)
        return -1;

    f->inode = node;
    f->offset = 0;
    f->flags = 0;

    return vfs_install_fd(f);
}

int
vfs_read(int fd, void* buf, ulong len)
{
    file_t *f = vfs_get_file(fd);
    if (!f || !f->inode || !(f->inode->mode & VFS_MODE_FILE))
        return -1;

    int n = f->inode->fs->ops->read(f->inode, f->offset, buf, len);
    if (n > 0)
        f->offset += n;

    return n;
}

int
vfs_write(int fd, const void *buf, ulong len)
{
    file_t *f = vfs_get_file(fd);
    if (!f || !f->inode || !(f->inode->mode & VFS_MODE_FILE))
        return -1;

    int n = f->inode->fs->ops->write(f->inode, f->offset, buf, len);
    if (n > 0)
        f->offset += n;

    return n;
}

int
vfs_create(const char *path, uint mode)
{
    if (!path || !root_fs)
        return -1;

    const char* name = path + 1;  // assume single-level path for now
    return root_fs->ops->create(root_fs->root, name, mode);
}

int vfs_mkdir(const char *path) {
    if (!path || !root_fs)
        return -1;

    const char* name = path + 1;
    return root_fs->ops->mkdir(root_fs->root, name);
}
