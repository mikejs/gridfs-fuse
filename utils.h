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

#ifndef __UTILS_H
#define __UTILS_H

#include <ctime>
#include <string>
#include <cstring>

inline const char* fuse_to_mongo_path(const char* path)
{
  if(path[0] == '/') {
    return path + 1;
  } else {
    return path;
  }
}

inline const bool is_leaf(const char* path) {
  int pp = -1;
  int sp = -1;
  for(int i=0; i<strlen(path); i++) {
    if(path[i] == '/') sp = i;
    if(path[i] == '.') pp = i;
  }
  return pp > sp;
}

inline const int path_depth(const char* path) {
  int sc = 0;
  for(int i=0; i<strlen(path); i++) {
    if(path[i] == '/') sc++;
  }
  return sc;
}

inline time_t mongo_time_to_unix_time(unsigned long long mtime)
{
  return mtime / 1000;
}

inline time_t unix_time_to_mongo_time(unsigned long long utime)
{
  return utime * 1000;
}

inline time_t mongo_time()
{
  return unix_time_to_mongo_time(time(NULL));
}

inline std::string namespace_xattr(const std::string name)
{
#ifdef __linux__
  return "user." + name;
#else
  return name;
#endif
}

inline const char* unnamespace_xattr(const char* name) {
#ifdef __linux__
  if(std::strstr(name, "user.") == name) {
    return name + 5;
  } else {
    return NULL;
  }
#else
  return name;
#endif
}

#endif
