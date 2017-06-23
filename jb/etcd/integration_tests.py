#!/usr/bin/env python

import os
import subprocess
import time
import unittest

class Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        wnull = open(os.devnull, 'w')
        cls._etcd = subprocess.Popen(
            ["/usr/bin/etcd"], stderr=wnull, stdout=wnull)

    @classmethod
    def tearDownClass(cls):
        cls._etcd.terminate()
        cls._etcd.wait()

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
