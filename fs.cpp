/*
 *  Copyright 2009 Michael Stephens
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define FUSE_USE_VERSION  26

#include <algorithm>
#include <fuse.h>
#include <fuse/fuse_opt.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <mongo/client/dbclient.h>
#include <mongo/client/gridfs.h>

using namespace std;
using namespace mongo;

static struct fuse_operations gridfs_oper;
DBClientConnection c;
GridFS *gf;

time_t mongo_time_to_unix_time(unsigned long long mtime) {
    return mtime / 1000;
}

static int gridfs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0555;
        stbuf->st_nlink = 2;
    } else {
        GridFile file = gf->findFile(path);
        if(!file.exists()) {
            return -ENOENT;
        }
        stbuf->st_mode = S_IFREG | 0555;
        stbuf->st_nlink = 1;
        stbuf->st_size = file.getContentLength();

        time_t upload_time = mongo_time_to_unix_time(file.getUploadDate());
        stbuf->st_ctime = upload_time;
        stbuf->st_mtime = upload_time;
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

static int gridfs_listxattr(const char* path, char* list, size_t size)
{
    const char* example_list = "md5";
    int len = strlen(example_list) + 1;

    if(size == 0) {
        return len;
    } else if(size < len) {
        return -ERANGE;
    }

    memcpy(list, example_list, len);
    return len;
}

static int gridfs_getxattr(const char* path, const char* name, char* value, size_t size)
{
    if(strcmp(name, "md5") != 0) {
        return -ENOATTR;
    }

    GridFile file = gf->findFile(path);
    if(!file.exists()) {
        return -ENOENT;
    }

    const char* attr = file.getMD5().c_str();
    int len = strlen(attr) + 1;

    if(size == 0) {
        return len;
    } else if(size < len) {
        return -ERANGE;
    }

    memcpy(value, attr, len);
    return len;
}

static int gridfs_setxattr(const char* path, const char* name, const char* value,
                           size_t size, int flags)
{
    return -ENOTSUP;
}

struct options {
    char* host;
    char* db;
} options;

#define GRIDFS_OPT_KEY(t, p, v) { t, offsetof(struct options, p), v }

enum {
    KEY_VERSION,
    KEY_HELP
};

static struct fuse_opt gridfs_opts[] =
{
    GRIDFS_OPT_KEY("--host=%s", host, 0),
    GRIDFS_OPT_KEY("--db=%s", db, 0),
    FUSE_OPT_KEY("-V", KEY_VERSION),
    FUSE_OPT_KEY("--version", KEY_VERSION),
    FUSE_OPT_KEY("-h", KEY_HELP),
    FUSE_OPT_KEY("--help", KEY_HELP),
    NULL
};

int main(int argc, char *argv[])
{
    gridfs_oper.getattr = gridfs_getattr;
    gridfs_oper.readdir = gridfs_readdir;
    gridfs_oper.open = gridfs_open;
    gridfs_oper.read = gridfs_read;
    gridfs_oper.listxattr = gridfs_listxattr;
    gridfs_oper.getxattr = gridfs_getxattr;
    gridfs_oper.setxattr = gridfs_setxattr;

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    memset(&options, 0, sizeof(struct options));
    if(fuse_opt_parse(&args, &options, gridfs_opts, NULL) == -1) {
        return -1;
    }

    if(!options.host) {
        options.host = "localhost";
    }
    if(!options.db) {
        options.db = "test";
    }

    c.connect(options.host);
    gf = new GridFS(c, options.db);

    return fuse_main(args.argc, args.argv, &gridfs_oper, NULL);
}
