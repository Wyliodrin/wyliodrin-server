import os
import time



gdb_commands_pipe_name = "/tmp/gdb_commands"
gdb_results_pipe_name  = "/tmp/gdb_results"



if __name__ == '__main__':
  # Create the commands and results pipes
  if not os.path.exists(gdb_commands_pipe_name):
    print("Creating " + gdb_commands_pipe_name)
    os.mkfifo(gdb_commands_pipe_name)
  if not os.path.exists(gdb_results_pipe_name):
    print("Creating " + gdb_results_pipe_name)
    os.mkfifo(gdb_results_pipe_name)

  # Open the pipes
  print("Opening pipes")
  gdb_commands_pipe_fd = open(gdb_commands_pipe_name, 'r')
  gdb_results_pipe_fd  = open(gdb_results_pipe_name,  'w')
  print("Both pipes open")

  while True:
    content = os.read(gdb_commands_pipe_fd.fileno(), 64).decode("utf-8")
    print("Read: " + content)
    gdb_results_pipe_fd.write(content)
    gdb_results_pipe_fd.flush()
