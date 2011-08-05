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

Usage
-----

    $ ./mount_gridfs --db=db_name --host=localhost --port=port --username=db_username --password=db_password mount_point

Current Limitations
-------------------
* Must specify all command-line arguments
* Database username and password sent in cleartext
* No directories
* No permissions 
* File creation/writing very experimental
