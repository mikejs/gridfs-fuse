GridFS FUSE
===========

Allows you to mount a MongoDB GridFS instance as a local filesystem.

Requirements
------------

* MongoDB 1.6+
* FUSE
* Boost

Building
--------

    $ make

Usage
-----

    $ ./mount_gridfs --db=db_name --host=localhost mount_point

Current Limitations
-------------------

* No directories
* No permissions or Mongo authentication
* File creation/writing very experimental
