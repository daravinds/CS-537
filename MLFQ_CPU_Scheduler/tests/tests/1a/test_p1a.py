#!/usr/bin/env python

import os, sys
from subprocess import Popen, PIPE, CalledProcessError
import signal


class TimeoutException(Exception):
    def __init__(self):
        super(TimeoutException, self).__init__("Timeout")


class Timeout:
    """Raise TimeoutError when a function call times out"""

    def __init__(self, seconds=0):
        self.seconds = seconds

    def handle_timeout(self, signum, frame):
        raise TimeoutException()

    def __enter__(self):
        signal.signal(signal.SIGALRM, self.handle_timeout)
        signal.alarm(self.seconds)

    def __exit__(self, type, value, traceback):
        signal.alarm(0)


class Test(object):
    """Barebone of a unit test."""

    name = "empty test"
    description = "this tests nothing and should be overriden"
    point_value = 10
    timeout = 20
    child = None

    @classmethod
    def popen_as_child(cls, cmd, check=True):
        p = Popen(cmd, stdout=PIPE, stderr=PIPE)
        cls.child = p
        stdout, stderr = p.communicate()
        cls.child = None
        if check:
            if p.returncode != 0:
                raise CalledProcessError(p.returncode, cmd, stdout, stderr)
            return stdout
        return stdout, stderr, p.returncode

    @classmethod
    def test(cls):
        """any real implementation of the test goes here
        it should raise exceptions when things fails (mostly through assert)
        and the run() below should handle that"""
        pass

    @classmethod
    def run(cls, args=None):
        """run the test
        This method prints useful message/line separators/etc.
        Any exception comes out of test() will be catched and regarded as
        failure.
        If nothing fails, the point value of the test is returned."""

        print('='*21 + ('%s starts' % cls.name).center(30) + '='*21)
	print cls.description
        print 'point value: %d' % cls.point_value

        try:
            with Timeout(cls.timeout):
                cls.test()
        except Exception as e:
            print('Test failed: %s' % cls.name)
            print('Reason:      %s' % e)
            print
            if cls.child is not None:
                try:
                    with Timeout(5):
                        cls.child.kill()
                        cls.child.wait()
                except Exception as e:
                    print('Failed to kill child %d' % cls.child.pid)
            return 0
        print('Test succeeded: %s' % cls.name)
        print
        return cls.point_value


class TestCompile(Test):
    name = "compilation test"
    description = "compiles source code"

    blacklist = 'gets fgets getchar getc getline getdelim'.split()

    obj_cmd = 'gcc -c shuffle.c'.split()
    nm_cmd = 'nm -u shuffle.o'.split()
    cmd = 'gcc -Wall -o shuffle shuffle.c -O'.split()

    @classmethod
    def test(cls):
        # first check if blacklisted functions are used
        cls.popen_as_child(cls.obj_cmd)
        output = cls.popen_as_child(cls.nm_cmd)
        for function in cls.blacklist:
            assert function not in output, 'Illegal use of blacklisted function: %s' % function

        cls.popen_as_child(cls.cmd)


def shuffle(lines):
    """python implementation of the shuffle program"""
    output = []
    i = 0
    j = len(lines)-1
    if j == -1:
        return output
    while True:
        output.append(lines[i])
        if i == j:
            break
        if i < j:
            i += 1
        else:
            i -= 1
        i, j = j, i
    return output


class ShuffleTest(Test):
    """abstract class for all test that runs the shuffle program to extend"""
    inputfile = 'inputfile'
    cmd = './shuffle -i %s -o outputfile'
    stdout = ''
    stderr = ''

    @staticmethod
    def generate_lines():
        """generate inputfile lines for testing"""
        return []

    @classmethod
    def test(cls):
	if os.path.isfile('outputfile'):
            os.remove('outputfile')

        input_lines = cls.generate_lines()
        with open('inputfile', 'w') as f:
            f.write(''.join(input_lines))

        cmd = (cls.cmd % cls.inputfile).split()
        cls.stdout, cls.stderr, rc = cls.popen_as_child(cmd, check=False)
        assert rc == 0, 'Non-zero exit status %d' % rc

        with open('outputfile', 'r') as f:
            test_output_lines = f.read()
        output_lines = ''.join(shuffle(input_lines))

        assert output_lines == test_output_lines, 'Output does not match'

class ExpectErrorTest(ShuffleTest):
    """abstract class for all shuffle tests that expect an error"""
    returncode = 1
    err_msg = ''

    @classmethod
    def test(cls):
        try:
            super(ExpectErrorTest, cls).test()
        except AssertionError as e:
            assert e.message == 'Non-zero exit status %d' % cls.returncode, 'Expect non-zero exit status'
            assert cls.stdout == '', 'Expect nothing to be printed in stdout'
            assert cls.stderr == cls.err_msg, 'Unexpected error message'
            return cls.point_value
        assert False, 'Expect an error from the program'

class TestInvalidFile(ExpectErrorTest):
    name = "invalid file test"
    description = "tests on a non-existing file"

    inputfile = 'youshouldnothavethisfileinyourdirectory'
    err_msg = 'Error: Cannot open file %s\n' % inputfile


class TestInvalidArg1(ExpectErrorTest):
    name = "invalid argument test 1"
    description = "tests on too few of arguments"
    point_value = 5

    cmd = './shuffle # %s'
    returncode = 1
    err_msg = "Usage: shuffle -i inputfile -o outputfile\n"


class TestInvalidArg2(ExpectErrorTest):
    name = "invalid argument test 2"
    description = "tests on too many arguments"
    point_value = 5

    cmd = './shuffle -i %s -o outputfile -xyz somejunk'
    returncode = 1
    err_msg = "Usage: shuffle -i inputfile -o outputfile\n"


class TestInvalidArg3(ExpectErrorTest):
    name = "invalid argument test 3"
    description = "tests on arguments with wrong flags"
    point_value = 5

    cmd = './shuffle -x %s -y outputfile'
    returncode = 1
    err_msg = "Usage: shuffle -i inputfile -o outputfile\n"


class TestExample(ShuffleTest):
    name = "example test"
    description = "comes from the project specification"

    @staticmethod
    def generate_lines():
        return ['first\n', 'second\n', 'third\n', 'fourth\n', 'fifth\n']


class TestSwitchArg(ShuffleTest):
    name = "switched arguments test"
    description = "tests different order of arguments"
    point_value = 5

    cmd = './shuffle -o outputfile -i %s'

    @staticmethod
    def generate_lines():
	return ['first\n', 'second\n', 'third\n', 'fourth\n', 'fifth\n']


class TestEmptyFile(ShuffleTest):
    name = "empty file test"
    description = "tests on an empty file"


class TestNormal1(ShuffleTest):
    name = "normal test 1"
    description = "tests on 1000 short lines"

    @staticmethod
    def generate_lines():
        return ['%d\n' % i for i in range(1000)]


class TestNormal2(ShuffleTest):
    name = "normal test 2"
    description = "tests on 1000 long lines"

    @staticmethod
    def generate_lines():
        return [('%d\n' % i).rjust(511, '*') for i in range(1000)]


class TestNormal3(ShuffleTest):
    name = "normal test 3"
    description = "tests on 5000 varying length lines"
    point_value = 5

    @staticmethod
    def generate_lines():
	return [('%d\n' % i).rjust(i % 511, '*') for i in range(5000)]


class TestNormal4(ShuffleTest):
    name = "normal test 4"
    description = "tests on odd number of long lines"
    point_value = 5

    @staticmethod
    def generate_lines():
	return [('%d\n' % i).rjust(511, '*') for i in range(5001)]


class TestNormal5(ShuffleTest):
    name = "normal test 5"
    description = "tests on 10 thousand newlines"
    point_value = 5

    @staticmethod
    def generate_lines():
	return ['\n' for i in range(10000)]

class TestStress1(ShuffleTest):
    name = "stress test 1"
    description = "tests on 100 thousand long lines (~50MB)"
    point_value = 5
    timeout = 30

    @staticmethod
    def generate_lines():
        return [('%d\n' % i).rjust(511, '*') for i in range(100*1000)]


class TestStress2(ShuffleTest):
    name = "stress test 2"
    description = "tests on 10 million short lines (~100MB)"
    point_value = 5
    timeout = 60

    @staticmethod
    def generate_lines():
        return ['%d\n' % i for i in range(10*1000*1000)]

def run_tests(tests, short_circuit=True, cleanup=False):
    points = 0
    tot_points = sum(test.point_value for test in tests)
    succeeded = failed = 0
    for test in tests:
        point = test.run()
        if point == 0:
            failed += 1
            if short_circuit:
                break
        else:
            succeeded += 1
        points += point
    if cleanup:
        print('='*21 + 'cleaning up'.center(30) + '='*21)
        for f in ['inputfile', 'outputfile', 'shuffle', 'shuffle.o']:
            if os.path.isfile(f):
                print('removing %s' % f)
                os.remove(f)
        print('='*21 + 'cleanup done'.center(30) + '='*21)
        print

    notrun = len(tests)-succeeded-failed
    print('succeeded: %d' % succeeded)
    print('failed: %d' % failed)
    print('not-run: %d\n' % notrun)
    print('score: %d (out of %d)' % (points, tot_points))

def main():
    testall = True
    cleanup = True
    for arg in sys.argv[1:]:
        if arg.startswith('-a'):
            testall = True
        elif arg.startswith('-c'):
            cleanup = True

    run_tests([TestCompile,
        TestInvalidFile,
        TestInvalidArg1,
        #TestInvalidArg2,
        TestInvalidArg3,
        TestExample,
        TestSwitchArg,
        TestEmptyFile,
        TestNormal1,
        TestNormal2,
        TestNormal3,
        TestNormal4,
        TestNormal5,
        TestStress1,
        TestStress2], short_circuit=not testall, cleanup=cleanup)

if __name__ == '__main__':
    main()
