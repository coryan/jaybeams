#!/usr/bin/env python

import os
import socket
import subprocess
import time
import unittest

class Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        wnull = open(os.devnull, 'w')
        cls._etcd = subprocess.Popen(
            ["/usr/bin/etcd"], stderr=wnull, stdout=wnull)
        # try to setup a connection, if it fails all hope for the
        # tests is lost.  The program takes a bit to start up, thus
        # the disgusting Sleep() calls ...
        sleep = 0.1
        for i in range(0, 10):
            sleep = sleep * 2
            time.sleep(1)
            try:
                s = socket.create_connection(("localhost", "2379"), 0.5)
                print "etcd server is accepting connections"
                return
            except socket.timeout as to:
                continue
            except socket.error as ex:
                print " .. connecting"
        print "cannot connect to etcd server"

    @classmethod
    def tearDownClass(cls):
        cls._etcd.kill()
        cls._etcd.wait()
        cls._etcd = None
        
    @classmethod
    def __del__(cls):
        if cls._etcd:
            tearDownClass(cls)

    def test_session(self):
        try:
            text = subprocess.check_output(
                ["jb/etcd/session_ut"],
                stderr=subprocess.STDOUT)
            print text
        except subprocess.CalledProcessError as ex:
            self.assertEqual(ex.returncode, 0)
            print ex.output

    def test_leader_election_participant(self):
        try:
            text = subprocess.check_output(
                ["jb/etcd/leader_election_participant_ut"],
                stderr=subprocess.STDOUT)
            print text
        except subprocess.CalledProcessError as ex:
            self.assertEqual(ex.returncode, 0)
            print ex.output
            
if __name__ == '__main__':
    unittest.main()
