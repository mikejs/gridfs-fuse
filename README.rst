===========
GridFS FUSE
===========

Allows you to mount a MongoDB GridFS instance as a local filesystem.

Requirements
============

* A recent (v1.1.2 or later) MongoDB
* FUSE
* scons
* Boost

Building
========

::

 $ scons

Usage
=====

::

 $ ./mount_gridfs --db=db_name --host=localhost mount_point

Current Limitations
===================

* No directories
* No permissions or Mongo authentication
* File creation/writing very experimental