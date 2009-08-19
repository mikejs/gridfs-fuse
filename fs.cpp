#define FUSE_USE_VERSION  26

#include <algorithm>
#include <fuse.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include "client/dbclient.h"
#include "client/gridfs.h"

using namespace std;
using namespace mongo;

static struct fuse_operations gridfs_oper;
DBClientConnection c;
GridFS *gf;

static int gridfs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        GridFile file = gf->findFile(path);
        if(!file.exists()) {
            return -ENOENT;
        }
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = file.getContentLength();
    }

    return 0;
}

static int gridfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    auto_ptr<DBClientCursor> cursor = gf->list();
    while(cursor->more()) {
        filler(buf, cursor->next().getStringField("filename") + 1, NULL, 0);
    }

    return 0;
}
static int gridfs_open(const char *path, struct fuse_file_info *fi)
{
    GridFile file = gf->findFile(path);
    if(file.exists()) {
        return 0;
    }

    return -ENOENT;
}

static int gridfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len = 0;

    GridFile file = gf->findFile(path);
    if(!file.exists()) {
        return 0;
    }

    int chunk_num = offset / file.getChunkSize();

    while(len < size && chunk_num < file.getNumChunks()) {
        Chunk chunk = file.getChunk(chunk_num);
        int to_read;
        int cl = chunk.len();

        const char *d = chunk.data(cl);

        if(len) {
            to_read = min((long unsigned)cl, size - len);
            memcpy(buf + len, d, sizeof(char)*to_read);
        } else {
            to_read = min((long unsigned)(cl - (offset % cl)), size - len);
            memcpy(buf + len, d + (offset % cl), to_read);
        }

        len += to_read;
        chunk_num++;
    }

    return len;
}

int main(int argc, char *argv[])
{
    gridfs_oper.getattr = gridfs_getattr;
    gridfs_oper.readdir = gridfs_readdir;
    gridfs_oper.open = gridfs_open;
    gridfs_oper.read = gridfs_read;

    c.connect("localhost");
    gf = new GridFS(c, argv[1]);

    return fuse_main(argc, argv, &gridfs_oper, NULL);
}
