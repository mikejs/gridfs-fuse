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

#ifndef __OPERATIONS_H
#define __OPERATIONS_H

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

// ENOATTR is not blessed by POSIX. Darwin uses 93.
#ifndef ENOATTR
#define ENOATTR 93
#endif

#include <fuse.h>

int gridfs_getattr(const char *path, struct stat *stbuf);

int gridfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
           off_t offset, struct fuse_file_info *fi);

int gridfs_open(const char *path, struct fuse_file_info *fi);

int gridfs_create(const char* path, mode_t mode, struct fuse_file_info* ffi);

int gridfs_release(const char* path, struct fuse_file_info* ffi);

int gridfs_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi);

int gridfs_unlink(const char* path);

int gridfs_listxattr(const char* path, char* list, size_t size);

int gridfs_getxattr(const char* path, const char* name, char* value, size_t size);

int gridfs_setxattr(const char* path, const char* name, const char* value,
          size_t size, int flags);

int gridfs_write(const char* path, const char* buf, size_t nbyte,
         off_t offset, struct fuse_file_info* ffi);

int gridfs_flush(const char* path, struct fuse_file_info* ffi);

int gridfs_rename(const char* old_path, const char* new_path);

#endif
