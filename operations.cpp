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

#include "operations.h"
#include "options.h"
#include "utils.h"
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <fcntl.h>

#include <mongo/client/dbclient.h>
#include <mongo/client/gridfs.h>
#include <mongo/client/connpool.h>

#ifdef __linux__
#include <attr/xattr.h>
#endif

using namespace std;
using namespace mongo;

int gridfs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));
    
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0555;
        stbuf->st_nlink = 2;
    } else {
        path = fuse_to_mongo_path(path);

        ScopedDbConnection sdc(gridfs_options.host);
        GridFS gf(sdc.conn(), gridfs_options.db);
        GridFile file = gf.findFile(path);
        sdc.done();

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

int gridfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi)
{
    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    ScopedDbConnection sdc(gridfs_options.host);
    GridFS gf(sdc.conn(), gridfs_options.db);

    auto_ptr<DBClientCursor> cursor = gf.list();
    while(cursor->more()) {
        BSONObj f = cursor->next();
        filler(buf, f.getStringField("filename") , NULL , 0);
    }
    sdc.done();

    return 0;
}

int gridfs_open(const char *path, struct fuse_file_info *fi)
{
    if((fi->flags & O_ACCMODE) != O_RDONLY) {
        return -EACCES;
    }

    path = fuse_to_mongo_path(path);

    ScopedDbConnection sdc(gridfs_options.host);
    GridFS gf(sdc.conn(), gridfs_options.db);
    GridFile file = gf.findFile(path);
    sdc.done();

    if(file.exists()) {
        return 0;
    }

    return -ENOENT;
}

int gridfs_read(const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi)
{
    path = fuse_to_mongo_path(path);
    size_t len = 0;

    ScopedDbConnection sdc(gridfs_options.host);
    GridFS gf(sdc.conn(), gridfs_options.db);
    GridFile file = gf.findFile(path);

    if(!file.exists()) {
        sdc.done();
        return -EBADF;
    }

    int chunk_size = file.getChunkSize();
    int chunk_num = offset / chunk_size;

    while(len < size && chunk_num < file.getNumChunks()) {
        Chunk chunk = file.getChunk(chunk_num);
        int to_read;
        int cl = chunk.len();

        const char *d = chunk.data(cl);

        if(len) {
            to_read = min((long unsigned)cl, (long unsigned)(size - len));
            memcpy(buf + len, d, to_read);
        } else {
            to_read = min((long unsigned)(cl - (offset % chunk_size)), (long unsigned)(size - len));
            memcpy(buf + len, d + (offset % chunk_size), to_read);
        }

        len += to_read;
        chunk_num++;
    }

    sdc.done();
    return len;
}

int gridfs_listxattr(const char* path, char* list, size_t size)
{
    path = fuse_to_mongo_path(path);

    ScopedDbConnection sdc(gridfs_options.host);
    GridFS gf(sdc.conn(), gridfs_options.db);
    GridFile file = gf.findFile(path);
    sdc.done();

    if(!file.exists()) {
        return -ENOENT;
    }

    int len = 0;
    BSONObj metadata = file.getMetadata();
    set<string> field_set;
    metadata.getFieldNames(field_set);
    for(set<string>::const_iterator s = field_set.begin(); s != field_set.end(); s++) {
#ifdef __linux__
        string attr_name = "user." + *s;
#else
        string attr_name = *s;
#endif
        int field_len = attr_name.size() + 1;
        len += field_len;
        if(size >= len) {
            memcpy(list, attr_name.c_str(), field_len);
            list += field_len;
        }
    }

    if(size == 0) {
        return len;
    } else if(size < len) {
        return -ERANGE;
    }

    return len;
}

int gridfs_getxattr(const char* path, const char* name, char* value, size_t size)
{
    if(strcmp(path, "/") == 0) {
        return -ENOATTR;
    }

    path = fuse_to_mongo_path(path);
    const char* attr_name;

#ifdef __linux__
    if(strstr(name, "user.") == name) {
        attr_name = name + 5;
    } else {
        return -ENOATTR;
    }
#else
    attr_name = name;
#endif

    ScopedDbConnection sdc(gridfs_options.host);
    GridFS gf(sdc.conn(), gridfs_options.db);
    GridFile file = gf.findFile(path);
    sdc.done();

    if(!file.exists()) {
        return -ENOENT;
    }

    BSONObj metadata = file.getMetadata();
    if(metadata.isEmpty()) {
        return -ENOATTR;
    }

    BSONElement field = metadata[attr_name];
    if(field.eoo()) {
        return -ENOATTR;
    }

    string field_str = field.toString();
    int len = field_str.size() + 1;
    if(size == 0) {
        return len;
    } else if(size < len) {
        return -ERANGE;
    }

    memcpy(value, field_str.c_str(), len);

    return len;
}

int gridfs_setxattr(const char* path, const char* name, const char* value,
                    size_t size, int flags)
{
    return -ENOTSUP;
}
