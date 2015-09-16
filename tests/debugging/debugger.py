import base64
import gdb
import msgpack
import os
import time



"""
  Read the gdb commands from the commands pipe, run them and write the results
  to the results pipe.
"""



gdb_commands_pipe_name = "/tmp/gdb_commands"
gdb_results_pipe_name  = "/tmp/gdb_results"
is_project_loaded = False



if __name__ == '__main__':
  # Create the commands and results pipes
  if not os.path.exists(gdb_commands_pipe_name):
    os.mkfifo(gdb_commands_pipe_name)
  if not os.path.exists(gdb_results_pipe_name):
    os.mkfifo(gdb_results_pipe_name)

  # Open the pipes
  gdb_commands_pipe_fd = open(gdb_commands_pipe_name, 'r')
  gdb_results_pipe_fd  = open(gdb_results_pipe_name,  'w')

  while True:
    # Get command
    content = os.read(gdb_commands_pipe_fd.fileno(), 3 * 1024).decode("utf-8")

    data = msgpack.unpackb(base64.b64decode(content))

    # Get project
    if b'p' not in data:
      logging.info("No project in data")
      break
    project = data[b'p'].decode("utf-8")
    if not is_project_loaded:
      gdb.execute('file ' + project)
      is_project_loaded = True

    # Disassembly
    if b'd' in data:
      disassemble_func = data[b'd']

      result = {}
      for func in disassemble_func:
        o = gdb.execute('disassemble ' + func.decode("utf-8"), to_string=True)
        result[func.decode("utf-8")] = o

      to_write = base64.b64encode(msgpack.packb(
        {
        "p" : project,
        "d" : result
        })).decode("utf-8")

      print(to_write)
      gdb_results_pipe_fd.write(to_write)
      gdb_results_pipe_fd.flush()


    if b'b' in data:
      breakpoints = data[b'b']

      for breakpoint in breakpoints:
        gdb.execute("break " + breakpoint.decode("utf-8"))

    if b'w' in data:
      watchpoints = data[b'w']

      for watchpoint in watchpoints:
        gdb.execute("watch " + watchpoint.decode("utf-8"))

    if b'c' in data:
      cmd = data[b'c'].decode("utf-8")
      cid = data[b'i'].decode("utf-8")

      if cmd == "run":
        os.system("rm out.log")
        os.system("rm err.log")
        o = gdb.execute("run > out.log 2> err.log")
      else:
        o = gdb.execute(cmd, to_string=True)
        gdb.execute('call fflush(0)')

      to_write = base64.b64encode(msgpack.packb(
        {
        "p" : project,
        "i" : cid,
        "r" : o,
        "o" : open("out.log").read(),
        "e" : open("err.log").read()
        })).decode("utf-8")

      gdb_results_pipe_fd.write(to_write)
      gdb_results_pipe_fd.flush()
