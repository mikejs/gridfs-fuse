# -*- mode: python; -*-
import os

env = Environment()
env.Append(CPPFLAGS=['-D_FILE_OFFSET_BITS=64'])

if 'darwin' == os.sys.platform:
    env.Append(CPPPATH=['/opt/local/include'],
               LIBPATH=['/opt/local/lib'])

env.Program('mount-gridfs', 'fs.cpp', LIBS=['mongoclient', 'boost_thread-mt',
                                            'boost_filesystem-mt',
                                            'boost_system-mt', 'fuse'])
