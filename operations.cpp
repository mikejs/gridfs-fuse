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
#include "local_gridfile.h"
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <fcntl.h>

#include <mongo/client/gridfs.h>
#include <mongo/client/connpool.h>

#ifdef __linux__
#include <sys/xattr.h>
#endif

using namespace std;
using namespace mongo;

std::map<string, LocalGridFile*> open_files;

unsigned int FH = 1;

int gridfs_getattr(const char *path, struct stat *stbuf)
{
  int res = 0;
  memset(stbuf, 0, sizeof(struct stat));

  if(strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0777;
    stbuf->st_nlink = 2;
    return 0;
  }

  path = fuse_to_mongo_path(path);

  map<string,LocalGridFile*>::const_iterator file_iter;
  file_iter = open_files.find(path);

  if(file_iter != open_files.end()) {
    stbuf->st_mode = S_IFREG | 0555;
    stbuf->st_nlink = 1;
    stbuf->st_ctime = time(NULL);
    stbuf->st_mtime = time(NULL);
    stbuf->st_size = file_iter->second->getLength();
    return 0;
  }

  // HACK: This is a protective measure to ensure we don't spin off into forever if a path without a period is requested.
  if(path_depth(path) > 10) {
    return -ENOENT;
  }

  // HACK: Assumes that if the last part of the path has a '.' in it, it's the leaf of the path, and if we haven't found a match by now,
  // give up and go home. This works just dandy as long as you avoid putting periods in your 'directory' names.
  if(!is_leaf(path)) {
    stbuf->st_mode = S_IFDIR | 0777;
    stbuf->st_nlink = 2;
    return 0;
  }

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

  for(map<string,LocalGridFile*>::const_iterator i = open_files.begin();
    i != open_files.end(); i++)
  {
    filler(buf, i->first.c_str(), NULL, 0);
  }

  return 0;
}

int gridfs_open(const char *path, struct fuse_file_info *fi)
{
  path = fuse_to_mongo_path(path);

  if((fi->flags & O_ACCMODE) == O_RDONLY) {
    if(open_files.find(path) != open_files.end()) {
      return 0;
    }

    ScopedDbConnection sdc(gridfs_options.host);
    GridFS gf(sdc.conn(), gridfs_options.db);
    GridFile file = gf.findFile(path);
    sdc.done();

    if(file.exists()) {
      return 0;
    }

    return -ENOENT;
  } else {
    return -EACCES;
  }
}

int gridfs_create(const char* path, mode_t mode, struct fuse_file_info* ffi)
{
  path = fuse_to_mongo_path(path);

  open_files[path] = new LocalGridFile(DEFAULT_CHUNK_SIZE);

  ffi->fh = FH++;

  return 0;
}

int gridfs_release(const char* path, struct fuse_file_info* ffi)
{
  path = fuse_to_mongo_path(path);

  if(!ffi->fh) {
    // fh is not set if file is opened read only
    // Would check ffi->flags for O_RDONLY instead but MacFuse doesn't
    // seem to properly pass flags into release
    return 0;
  }

  delete open_files[path];
  open_files.erase(path);

  return 0;
}

int gridfs_unlink(const char* path) {
  path = fuse_to_mongo_path(path);

  ScopedDbConnection sdc(gridfs_options.host);
  GridFS gf(sdc.conn(), gridfs_options.db);
  gf.removeFile(path);
  sdc.done();

  return 0;
}

int gridfs_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi)
{
  path = fuse_to_mongo_path(path);
  size_t len = 0;

  map<string,LocalGridFile*>::const_iterator file_iter;
  file_iter = open_files.find(path);
  if(file_iter != open_files.end()) {
    LocalGridFile *lgf = file_iter->second;
    return lgf->read(buf, size, offset);
  }

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
    GridFSChunk chunk = file.getChunk(chunk_num);
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

  if(open_files.find(path) != open_files.end()) {
    return 0;
  }

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
    string attr_name = namespace_xattr(*s);
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
  const char* attr_name = unnamespace_xattr(name);
  if(!attr_name) {
    return -ENOATTR;
  }

  if(open_files.find(path) != open_files.end()) {
    return -ENOATTR;
  }

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

int gridfs_write(const char* path, const char* buf, size_t nbyte,
         off_t offset, struct fuse_file_info* ffi)
{
  path = fuse_to_mongo_path(path);

  if(open_files.find(path) == open_files.end()) {
    return -ENOENT;
  }

  LocalGridFile *lgf = open_files[path];

  return lgf->write(buf, nbyte, offset);
}

int gridfs_flush(const char* path, struct fuse_file_info *ffi)
{
  path = fuse_to_mongo_path(path);

  if(!ffi->fh) {
    return 0;
  }

  map<string,LocalGridFile*>::iterator file_iter;
  file_iter = open_files.find(path);
  if(file_iter == open_files.end()) {
    return -ENOENT;
  }

  LocalGridFile *lgf = file_iter->second;

  if(!lgf->dirty()) {
    return 0;
  }

  ScopedDbConnection sdc(gridfs_options.host);
  DBClientBase &client = sdc.conn();
  GridFS gf(sdc.conn(), gridfs_options.db);

  if(gf.findFile(path).exists()) {
    gf.removeFile(path);
  }

  size_t len = lgf->getLength();
  char *buf = new char[len];
  lgf->read(buf, len, 0);

  gf.storeFile(buf, len, path);

  sdc.done();

  lgf->flushed();

  return 0;
}

int gridfs_rename(const char* old_path, const char* new_path)
{
  old_path = fuse_to_mongo_path(old_path);
  new_path = fuse_to_mongo_path(new_path);

  ScopedDbConnection sdc(gridfs_options.host);
  DBClientBase &client = sdc.conn();

  string db_name = gridfs_options.db;

  BSONObj file_obj = client.findOne(db_name + ".fs.files",
                    BSON("filename" << old_path));

  if(file_obj.isEmpty()) {
    return -ENOENT;
  }

  BSONObjBuilder b;
  set<string> field_names;
  file_obj.getFieldNames(field_names);

  for(set<string>::iterator name = field_names.begin();
    name != field_names.end(); name++)
  {
    if(*name != "filename") {
      b.append(file_obj.getField(*name));
    }
  }

  b << "filename" << new_path;

  client.update(db_name + ".fs.files",
          BSON("_id" << file_obj.getField("_id")), b.obj());

  sdc.done();

  return 0;
}
