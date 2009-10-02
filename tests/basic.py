#!/usr/bin/env python
from __future__ import with_statement
import unittest
import os
import subprocess
import time
import glob

class BasicGridfsFUSETestCase(unittest.TestCase):

    def setUp(self):
        self.mount = os.path.join(os.path.dirname(
                os.path.abspath(__file__)),
                                  'mount')
        os.mkdir('tests/mount')
        subprocess.check_call(['./mount_gridfs', '--db=gridfstest',
                               self.mount])

        # wait for mount to complete
        time.sleep(1)
            
    def tearDown(self):
        for filename in glob.iglob(os.path.join(self.mount, '*')):
            os.remove(filename)
        subprocess.check_call(['umount', self.mount])
        os.rmdir(self.mount)
        
    def testreadwrite(self):
        with open(os.path.join(self.mount, 'testfile.txt'), 'w') as f:
            w.write("This is a test of GridFS FUSE.")
        
        with open(os.path.join(self.mount, 'testfile.txt'), 'r') as r:
            self.assertEqual("This is a test of GridFS FUSE.",
                             r.read())

def suite():
    suite = unittest.TestSuite()
    suite.addTest(BasicGridfsFUSETestCase())
    return suite

if __name__ == '__main__':
    unittest.main()
