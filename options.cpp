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

#include "options.h"
#include <iostream>

using namespace std;

struct gridfs_options gridfs_options;

struct fuse_opt gridfs_opts[] =
{
    GRIDFS_OPT_KEY("--host=%s", host, 0),
    GRIDFS_OPT_KEY("--db=%s", db, 0),
    FUSE_OPT_KEY("-v", KEY_VERSION),
    FUSE_OPT_KEY("--version", KEY_VERSION),
    FUSE_OPT_KEY("-h", KEY_HELP),
    FUSE_OPT_KEY("--help", KEY_HELP),
    NULL
};

int gridfs_opt_proc(void* data, const char* arg, int key,
                    struct fuse_args* outargs)
{
    if(key == KEY_HELP) {
        print_help();
        return -1;
    }

    if(key == KEY_VERSION) {
        cout << "gridfs-fuse version 0.3" << endl;
        return -1;
    }

    return 1;
}

void print_help()
{
    cout << "usage: ./mount_gridfs [options] mountpoint" << endl;
    cout << endl << "general options:" << endl;
    cout << "\t--db=[dbname]\t\twhich mongo database to use" << endl;
    cout << "\t--host=[hostname]\thostname of your mongodb server" << endl;
    cout << "\t-h, --help\t\tprint help" << endl;
    cout << "\t-v, --version\t\tprint version" << endl;
    cout << endl << "FUSE options: " << endl;
    cout << "\t-d, -o debug\t\tenable debug output (implies -f)" << endl;
    cout << "\t-f\t\t\tforeground operation" << endl;
    cout << "\t-s\t\t\tdisable multi-threaded operation" << endl;
}
