GridFS FUSE
===========

Allows you to mount a MongoDB GridFS instance as a local filesystem.


Requirements
------------

* MongoDB 1.6+
* FUSE
* Boost (header files + the following separately built libraries --with-libraries=filesystem,program_options,system,thread)

Building
--------

    $ make

* Note that if you are using a debian or Ubuntu install, it might be better to
  use the debian make target (use "make debian") instead of the default target.
  You will need the following packages:

  g++, libfuse-dev, mongodb-dev, libboost-system-dev, libboost-filesystem-dev,
  libboost-thread-dev

Usage
-----

    $ ./mount_gridfs --db=db_name --prefix=fs --host=localhost --port=port --username=db_username --password=db_password mount_point

Current Limitations
-------------------
* Must specify all command-line arguments
* Database username and password sent in cleartext
* No directories
* No permissions 
* File creation/writing very experimental
