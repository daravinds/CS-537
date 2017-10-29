import os, subprocess, shutil
from time import time, sleep
import signal

import toolspath
from testing import Test, BuildTest

#shell_prompt = "0> "
call_shell = "mysh <"

def readall(filename):
  f = open(filename, 'r')
  s = f.read()
  f.close()
  return s

class ShellBuildTest(BuildTest):
  targets = ['mysh']

  def run(self):
    self.clean(['mysh', '*.o'])
    if not self.make(self.targets, required=False):
      self.run_util(['gcc', 'mysh.c', '-o', 'mysh', '-Wall', '-std=gnu11'])
    self.done()

def restore_signals(): # from http://hg.python.org/cpython/rev/768722b2ae0a/
  signals = ('SIGPIPE', 'SIGXFZ', 'SIGXFSZ')
  for sig in signals:
    if hasattr(signal, sig):
      signal.signal(getattr(signal, sig), signal.SIG_DFL)

class ShellTest(Test):
  def shell_prompt(self, histnum, cwd=None):
    return 'mysh ({})> '.format(histnum)

  def startexe(self, args, libs = None, path = None,
        stdin = None, stdout = None, stderr = None, cwd = None):
     name = args[0]
     if stdout is None:
        stdout = self.logfd
     if stderr is None:
        stderr = self.logfd
     if path is None:
        path = os.path.join(self.project_path, name)
     if libs is not None:
        os.environ["LD_PRELOAD"] = libs
     if cwd is None:
        cwd = self.project_path
     args[0] = path
     self.log(" ".join(args))
     if self.use_gdb:
        child = subprocess.Popen(["xterm",
           "-title", "\"" + " ".join(args) + "\"",
           "-e", "gdb", "--args"] + args,
              cwd=cwd,
              stdin=stdin, stdout=stdout, stderr=stderr,
              shell=True, preexec_fn=restore_signals)
        self.children.append(child)
        return child
     if self.use_valgrind:
        self.log("WITH VALGRIND")
        child = subprocess.Popen(["valgrind"] + args,
              stdin=stdin, stdout=stdout, stderr=stderr,
              cwd=cwd, preexec_fn=restore_signals)
        sleep(1)
        self.children.append(child)
        return child
     else:
        child = subprocess.Popen(args, cwd=cwd,
              stdin=stdin, stdout=stdout, stderr=stderr,
              preexec_fn=restore_signals)
        self.children.append(child)
        return child

     if libs is not None:
        os.environ["LD_PRELOAD"] = ""
        del os.environ["LD_PRELOAD"]

  def run(self, command = None, stdout = None, stderr = None,
          addl_args = []):
    in_path = self.test_path + '/ctests/' + self.name + '/in'
    if command == None:
      command = ['./mysh']

    out_path = self.test_path + '/ctests/' + self.name + '/out'
    err_path = self.test_path + '/ctests/' + self.name + '/err'
    self.command = call_shell + in_path + \
        "\n and check out the test folder\n " + self.test_path \
        + '/ctests/' + self.name + \
        "\n to compare your output with reference outputs. "

    # process the input file with input redirection
    stdin = readall(in_path)

    if stdout == None:
      stdout = readall(out_path)
      import re
      stdout = re.sub(r'mysh (\d)> ', r'[{}]\n\1> '.format(self.project_path), stdout)
    if stderr == None:
      stderr = readall(err_path)

    self.runexe(command, status=self.status,
                stdin=stdin, stderr=stderr, stdout=stdout)
    self.done()

class LintTest(Test):
    name = "linttest"
    description = "Lint test for C programming style"
    point_value = 5
    timeout = 5

    def run(self):
        lint_path = self.test_path + "/../../lint/"
        config_file = "CPPLINT.cfg"
        shutil.copy(lint_path + config_file, self.project_path + "/" + config_file)
        cpplint = self.test_path + "/../../lint/cpplint.py"
        files = os.listdir(os.getcwd())
        candidates = list()
        for file in files:
            if file.endswith('.c') or file.endswith('.h'):
                candidates.append(file)

        for file in candidates:
            self.runexe([cpplint] + ["--extensions", "c,h", file], status = 0)

        os.remove(self.project_path + "/" + config_file)
        self.done()

class ValgrindTest(Test):
    name = "valgrindtest"
    description = "Valgrind test for memory management"
    timeout = 120
    point_value = 5

    def run(self):
        vgname = "vg_summary.xml"
        vgfile = self.project_path + "/" + vgname
        in_path = self.test_path + "/ctests/" + 'valgrind/in'
        stdin = readall(in_path)

        self.log("Running valgrind")
        child = self.runexe(["/usr/bin/valgrind", "--show-reachable=yes",
                             "--xml=yes", "--child-silent-after-fork=yes", "--undef-value-errors=no",
                             "--xml-file=" + vgfile, "./mysh"], stdin=stdin)
        self.log("Valgrind completed in " + str(child.wallclock_time) + " seconds")
        import xml.etree.ElementTree
        summary = xml.etree.ElementTree.parse(vgfile).getroot()
        if summary.find('error') != None:
            shutil.copy2(vgfile, vgname)
            self.fail("Valgrind error, check error section of summary: {0}\n".format(vgname))
        self.done();

######################### Built-in Commands #########################

class Exit(ShellTest):
  name = 'exit'
  description = 'run exit'
  timeout = 10
  status = 0
  point_value = 5

class PWD(ShellTest):
  name = 'pwd'
  description = 'run pwd'
  timeout = 10
  status = 0
  point_value = 5

class BadPWD(ShellTest):
  name = 'badpwd'
  description = 'run pwd with extra argument'
  timeout = 10
  status = 0
  point_value = 1

class CD(ShellTest):
  name = 'cd'
  description = 'cd with argument'
  timeout = 10
  status = 0
  point_value = 5
  def run(self):
    out1 = subprocess.Popen(['/bin/ls', '/u'],
                             stdout=subprocess.PIPE).communicate()[0]
    out2 = subprocess.Popen(['/bin/ls', os.environ['HOME']],
                            stdout=subprocess.PIPE).communicate()[0]
    super(CD, self).run(stdout = self.shell_prompt(1) +
                        self.shell_prompt(2,'/afs/cs.wisc.edu/u') + out1 +
                        self.shell_prompt(3,'/afs/cs.wisc.edu/u') +
                        self.shell_prompt(4,'/afs/cs.wisc.edu'+os.environ['HOME']) + out2 +
                        self.shell_prompt(5,'/afs/cs.wisc.edu'+os.environ['HOME']))

class BadCD(ShellTest):
  name = 'badcd'
  description = 'cd to a bad directory'
  timeout = 10
  status = 0
  point_value = 2

class BadCD2(ShellTest):
  name = 'badcd2'
  description = 'cd with 2 arguments'
  timeout = 10
  status = 0
  point_value = 1

class Exec(ShellTest):
  name = 'exec'
  description = 'path should be /bin'
  timeout = 10
  status = 0
  point_value = 5

class BadExec(ShellTest):
  name = 'badexec'
  description = 'run programs that do not exist in /bin'
  timeout = 10
  status = 0
  point_value = 3

class Stress(ShellTest):
  name = 'stress'
  description = 'stress testing the shell with large number of commands'
  timeout = 10
  status = 0
  point_value = 2
  def run(self):
    generate_path = self.test_path + "/ctests/" + self.name + "/gen"

    status = self.run_util([generate_path, "-s", str(1), "-n", str(10000)])
    if status != 0:
      raise Exception("generate failed with error " + str(status))
    super(Stress, self).run()


######################### Formatting ###########################

class Line(ShellTest):
  name = 'line'
  description = 'line with maximum allowed length'
  timeout = 10
  status = 0
  point_value = 2

class BadLine(ShellTest):
  name = 'badline'
  description = 'a line that is too long'
  timeout = 10
  status = 0
  point_value = 2

class BadArg(ShellTest):
  name = 'badarg'
  description = 'extra argument to mysh'
  timeout = 10
  status = 1
  def run(self):
    super(BadArg, self).run(command = ['./mysh', '/extra'])
  point_value = 2

class WhiteSpace(ShellTest):
  name = 'whitespace'
  description = 'leading and trailing whitespace, full whitespaces or empty string'
  timeout = 10
  status = 0
  point_value = 2


######################### Redirection #############################

class Rdr(ShellTest):
  name = 'rdr'
  description = 'simple output redirection'
  timeout = 10
  status = 0
  point_value = 5
  def run(self):
    super(Rdr, self).run()
    if os.path.isfile(self.project_path + "/output.out") == False:
      self.fail("missing redirected standard output output.out")
    out = readall(self.project_path + "/output.out")
    ref_out = readall(self.test_path + '/ctests/' + self.name + '/output.out')
    if (out != ref_out): self.fail("redirected output does not match expected output")

    self.done()

class Rdr2(ShellTest):
  name = 'rdr2'
  description = 'simple input redirection'
  timeout = 10
  status = 0
  point_value = 5
  def run(self):
    shutil.copyfile(self.test_path + '/ctests/' + self.name + '/input1.txt',
                    self.project_path + "/input1.txt")
    shutil.copyfile(self.test_path + '/ctests/' + self.name + '/input2.txt',
                    self.project_path + "/input2.txt")
    super(Rdr2, self).run()

class Rdr3(ShellTest):
  name = 'rdr3'
  description = 'input and output redirection'
  timeout = 10
  status = 0
  point_value = 4
  def run(self):
    shutil.copyfile(self.test_path + '/ctests/' + self.name + '/input1.txt',
                    self.project_path + "/input1.txt")
    super(Rdr3, self).run()
    if os.path.isfile(self.project_path + "/output.txt") == False:
      self.fail("missing redirected standard output output.txt")
    out = readall(self.project_path + "/output.txt")
    ref_out = readall(self.test_path + '/ctests/' + self.name + '/input1.txt')
    if (out != ref_out): self.fail("redirected output does not match expected output")
    self.done()

class Rdr4(ShellTest):
  name = 'rdr4'
  description = 'another input and output redirection'
  timeout = 10
  status = 0
  point_value = 3
  def run(self):
    shutil.copyfile(self.test_path + '/ctests/' + self.name + '/input1.txt',
                    self.project_path + "/input1.txt")
    super(Rdr4, self).run()
    if os.path.isfile(self.project_path + "/output.txt") == False:
      self.fail("missing redirected standard output output.txt")
    out = readall(self.project_path + "/output.txt")
    ref_out = readall(self.test_path + '/ctests/' + self.name + '/input1.txt')
    if (out != ref_out): self.fail("redirected output does not match expected output")
    self.done()


class Rdr5(ShellTest):
  name = 'rdr5'
  description = 'simple output redirection that overwrites existing file'
  timeout = 10
  status = 0
  point_value = 2
  def run(self):
    shutil.copyfile(self.test_path + '/ctests/' + self.name + '/output.txt',
                      self.project_path + "/output.txt")
    super(Rdr5, self).run()
    if os.path.isfile(self.project_path + "/output.txt") == False:
      self.fail("missing redirected standard output output.txt")
    out = readall(self.project_path + "/output.txt")
    ref_out = readall(self.test_path + '/ctests/' + self.name + '/outputref.txt')
    if (out != ref_out): self.fail("redirected output does not match expected output")

    self.done()

class BadRdr(ShellTest):
  name = 'badrdr'
  description = 'bad redirection format'
  timeout = 10
  status = 0
  point_value = 2

class BadRdr2(ShellTest):
  name = 'badrdr2'
  description = 'bad input redirection that redirects to invalid file'
  timeout = 10
  status = 0
  point_value = 2

class WhiteSpace2(ShellTest):
  name = 'whitespace2'
  description = 'input and output redirection with more than one whitespace between tokens'
  timeout = 10
  status = 0
  point_value = 1
  def run(self):
    shutil.copyfile(self.test_path + '/ctests/' + self.name + '/input1.txt',
                    self.project_path + "/input1.txt")
    super(WhiteSpace2, self).run()
    if os.path.isfile(self.project_path + "/output.txt") == False:
      self.fail("missing redirected standard output output.txt")
    out = readall(self.project_path + "/output.txt")
    ref_out = readall(self.test_path + '/ctests/' + self.name + '/input1.txt')
    if (out != ref_out): self.fail("redirected output does not match expected output")
    self.done()


######################### Pipeline #############################

class Pipe(ShellTest):
    name = 'pipe'
    description = 'simple pipeline'
    timeout = 10
    status = 0
    point_value = 5

class Pipe2(ShellTest):
    name = 'pipe2'
    description = 'simple pipeline 2'
    timeout = 10
    status = 0
    point_value = 4

class Pipe3(ShellTest):
    name = 'pipe3'
    description = 'simple pipeline 3'
    timeout = 10
    status = 0
    point_value = 4

class BadPipe(ShellTest):
    name = 'badpipe'
    description = 'bad pipeline that has invalid command'
    timeout = 10
    status = 0
    point_value = 2

class BadPipe2(ShellTest):
    name = 'badpipe2'
    description = 'bad pipeline with no commands followed'
    timeout = 10
    status = 0
    point_value = 1


######################### Background Jobs #############################


class Bg(ShellTest):
    name = 'bg'
    description = 'simple background execution'
    timeout = 10
    status = 0
    point_value = 5

class BgStress(ShellTest):
    name = 'bgstress'
    description = 'stress test with multiple background jobs'
    timeout = 10
    status = 0
    point_value = 2
    def run(self):
      super(BgStress, self).run()
      out1 = subprocess.Popen(['/bin/ps'],
                               stdout=subprocess.PIPE).communicate()[0]
      if ('sleep' in out1): self.fail("some child processes are not terminated:\n" + out1)
      if ('defunct' in out1): self.fail("some zombie processes are not collected:\n" + out1)
      self.done()

class Bg2(ShellTest):
    name = 'bg2'
    description = 'check zombie processes'
    timeout = 10
    status = 0
    point_value = 4

class Bg3(ShellTest):
    name = 'bg3'
    description = 'check if all child processes are terminated after exit'
    timeout = 10
    status = 0
    point_value = 3
    def run(self):
      super(Bg3, self).run()
      out1 = subprocess.Popen(['/bin/ps'],
                               stdout=subprocess.PIPE).communicate()[0]
      if ('sleep' in out1): self.fail("child processes are not terminated")
      self.done()

class BgRdr(ShellTest):
    name = 'bgrdr'
    description = 'background execution + redirection'
    timeout = 20
    status = 0
    point_value = 4

    def run(self):
      shutil.copyfile(self.test_path + '/ctests/' + self.name + '/input1.txt',
                      self.project_path + "/input1.txt")
      super(BgRdr, self).run()
      if os.path.isfile(self.project_path + "/output.txt") == False:
        self.fail("missing redirected standard output output.txt")
      out = readall(self.project_path + "/output.txt")
      ref_out = readall(self.test_path + '/ctests/' + self.name + '/input1.txt')
      if (out != ref_out): self.fail("redirected output does not match expected output")
      self.done()


#=========================================================================

all_tests = [
  LintTest,
  # Basic
  Exit,

  # Exec
  Exec,
  BadExec,
  Stress,

  # Formatting
  Line,
  BadLine,
  BadArg,
  WhiteSpace,

  # Built-in Command
  CD,
  BadCD,
  BadCD2,
  PWD,
  BadPWD,

  # Redirection
  Rdr,
  Rdr2,
  BadRdr,
  BadRdr2,
  Rdr3,
  Rdr4,
  Rdr5,
  WhiteSpace2,

  # Pipeline
  Pipe,
  Pipe2,
  Pipe3,
  BadPipe,
  BadPipe2,

  # Background
  Bg,
  Bg2,
  Bg3,
  BgStress,
  BgRdr,

  ValgrindTest,
  ]

build_test = ShellBuildTest

from testing.runtests import main
main(build_test, all_tests)
