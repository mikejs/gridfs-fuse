# -*- mode: python; -*-
import os

env = Environment()
env.Append(CPPFLAGS=['-D_FILE_OFFSET_BITS=64'])

fuse_lib = "fuse"

if 'darwin' == os.sys.platform:
    env.Append(CPPPATH=['/opt/local/include'],
               LIBPATH=['/opt/local/lib'])

    if os.uname()[2] == '10.0.0':
        # Snow Leopard uses 64 bit inodes
        fuse_lib += '_ino64'

conf = Configure( env )
libs = [ "mongoclient" , fuse_lib ]
boostLibs = [ "thread" , "filesystem" ]

def checkLib( n ):
    if conf.CheckLib( n ):
        return True
    print( "Error: can't find library: " + str( n ) )
    Exit(-1)
    return False

for x in libs:
    checkLib( x )

def makeBoost( x ):
    # this will have to get more complicated later
    return "boost_" + x + "-mt";

for x in boostLibs:
    checkLib( makeBoost( x ) )

conf.CheckLib( makeBoost( "system" ) )

env = conf.Finish()

files = ['main.cpp', 'operations.cpp', 'options.cpp']

env.Program('mount_gridfs', files)
