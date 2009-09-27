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

#ifndef __OPTIONS_H
#define __OPTIONS_H

#include <fuse/fuse_opt.h>
#include <cstddef>

struct gridfs_options {
    const char* host;
    const char* db;
};

extern gridfs_options gridfs_options;

#define GRIDFS_OPT_KEY(t, p, v) { t, offsetof(struct gridfs_options, p), v }

enum {
    KEY_VERSION,
    KEY_HELP
};

extern struct fuse_opt gridfs_opts[];

int gridfs_opt_proc(void* data, const char* arg, int key,
                    struct fuse_args* outargs);

void print_help();
#endif
