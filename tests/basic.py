#!/usr/bin/env python
from __future__ import with_statement
import unittest
import os
import subprocess
import time
import glob
import stat

class BasicGridfsFUSETestCase(unittest.TestCase):

    def setUp(self):
        self.mount = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                  'mount')
        os.mkdir('tests/mount')
        subprocess.check_call(['./mount_gridfs', '--db=gridfstest',
                               self.mount])

        # wait for mount to complete
        time.sleep(1)

    def tearDown(self):
        for filename in glob.iglob(os.path.join(self.mount, '*')):
            os.remove(filename)

        if os.sys.platform == 'linux2':
            subprocess.check_call(['fusermount', '-u', self.mount])
        else:
            subprocess.check_call(['umount', self.mount])

        os.rmdir(self.mount)

    def test_read_write(self):
        with open(os.path.join(self.mount, 'testfile.txt'), 'w') as w:
            w.write("This is a test of GridFS FUSE.")

        with open(os.path.join(self.mount, 'testfile.txt'), 'r') as r:
            self.assertEqual("This is a test of GridFS FUSE.",
                             r.read())

    def test_stat(self):
        def assert_stat(path, mode=0555, size=4):
            stat_result = os.stat(path)
            st_mode = stat_result.st_mode
            self.assert_(stat.S_ISREG(st_mode))
            self.assertEquals(mode, stat.S_IMODE(st_mode))
            self.assertEquals(size, stat_result.st_size)

        path = os.path.join(self.mount, 'testfile.txt')
        with open(path, 'w') as w:
            assert_stat(path, size=0)
            w.write('test')
            w.flush()
            assert_stat(path)

        assert_stat(path)

    def test_read_write(self):
        path = os.path.join(self.mount, 'file')

        with open(path, 'w') as w:
            self.assertRaises(IOError, open, path, 'w')

            w.write('Hello')
            w.flush()

            with open(path, 'r') as r:
                self.assertEquals('Hello', r.read())
                w.write(' world')
                w.flush()
                self.assertEquals(' world', r.read())

    def test_ls(self):
        self.assertEquals(0, len(os.listdir(self.mount)))

        with open(os.path.join(self.mount, 'file1'), 'w') as f1:
            f1.write("file1")
        with open(os.path.join(self.mount, 'file2'), 'w') as f2:
            f2.write("file2")

        files = os.listdir(self.mount)
        self.assertEquals(2, len(files))
        self.assert_('file1' in files)
        self.assert_('file2' in files)

    def test_unlink(self):
        path = os.path.join(self.mount, 'file')

        with open(path, 'w') as w:
            w.write("a file")

        self.assert_('file' in os.listdir(self.mount))

        os.unlink(path)

        self.assert_('file' not in os.listdir(self.mount))

    def test_rename(self):
        path1 = os.path.join(self.mount, 'file1')
        path2 = os.path.join(self.mount, 'file2')

        with open(path1, 'w') as w:
            w.write('file1')

        self.assert_('file1' in os.listdir(self.mount))
        self.assert_('file2' not in os.listdir(self.mount))

        os.rename(path1, path2)

        self.assert_('file1' not in os.listdir(self.mount))
        self.assert_('file2' in os.listdir(self.mount))

        with open(path2, 'r') as r:
            self.assertEquals('file1', r.read())

    def test_big_file(self):
        # Test creation/reading of a file that's bigger than
        # the chunk size
        path = os.path.join(self.mount, 'big')
        size = 256 * 1024 * 3 + 100
        data = 'A' * size

        with open(path, 'w') as w:
            w.write(data)

        with open(path, 'r') as r:
            self.assertEquals(data, r.read())

        self.assertEquals(size, os.stat(path).st_size)

    def test_seek(self):
        path = os.path.join(self.mount, 'file')
        size1 = 256 * 1024 * 3
        data1 = 'A' * size1
        data2 = 'B' * size1
        expected = ('A' * (size1 / 2)) + data2
        size2 = len(expected)

        with open(path, 'w') as w:
            w.write(data1)
            w.seek(size1 / 2)
            w.write(data2)

        with open(path, 'r') as r:
            self.assertEquals(expected, r.read())

        self.assertEquals(size2, os.stat(path).st_size)

def suite():
    suite = unittest.TestSuite()
    suite.addTest(BasicGridfsFUSETestCase())
    return suite

if __name__ == '__main__':
    unittest.main()
