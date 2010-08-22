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
#include <cstring>

using namespace std;

int main(int argc, char *argv[])
{
  static struct fuse_operations gridfs_oper;
  gridfs_oper.getattr = gridfs_getattr;
  gridfs_oper.readdir = gridfs_readdir;
  gridfs_oper.open = gridfs_open;
  gridfs_oper.create = gridfs_create;
  gridfs_oper.release = gridfs_release;
  gridfs_oper.unlink = gridfs_unlink;
  gridfs_oper.read = gridfs_read;
  gridfs_oper.listxattr = gridfs_listxattr;
  gridfs_oper.getxattr = gridfs_getxattr;
  gridfs_oper.setxattr = gridfs_setxattr;
  gridfs_oper.write = gridfs_write;
  gridfs_oper.flush = gridfs_flush;
  gridfs_oper.rename = gridfs_rename;

  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  memset(&gridfs_options, 0, sizeof(struct gridfs_options));
  if(fuse_opt_parse(&args, &gridfs_options, gridfs_opts,
            gridfs_opt_proc) == -1)
  {
    return -1;
  }

  if(!gridfs_options.host) {
    gridfs_options.host = "localhost";
  }
  if(!gridfs_options.db) {
    gridfs_options.db = "test";
  }

  return fuse_main(args.argc, args.argv, &gridfs_oper, NULL);
}
