# -*- mode: python; -*-
import os

env = Environment()
env.Append(CPPFLAGS=['-D_FILE_OFFSET_BITS=64'])

if 'darwin' == os.sys.platform:
    env.Append(CPPPATH=['/opt/local/include'],
               LIBPATH=['/opt/local/lib'])

conf = Configure( env )
libs = [ "mongoclient" , "fuse" ]
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

env.Program('mount-gridfs', 'fs.cpp' )
