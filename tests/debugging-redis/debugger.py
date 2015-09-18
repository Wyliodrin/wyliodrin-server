import base64
import gdb
import msgpack
import os
import redis
import time



"""
  Read the gdb commands from the commands channel, run them and write the results
  to the results channel.
"""



gdb_commands_channel_name = "gdb_commands"
gdb_results_channel_name  = "gdb_results"



if __name__ == '__main__':
  r = redis.Redis()
  pubsub = r.pubsub()
  pubsub.subscribe([gdb_commands_channel_name])

  my_project = None

  while True:
    for content in pubsub.listen():
      if (content['data'] is None) or (content['type'] != "message"):
        continue

      data = msgpack.unpackb(base64.b64decode(content['data']))

      if b'i' not in data:
        # Every message should have an id
        continue

      i = data[b'i']

      # Start debugging a file
      if my_project is None:
        if b's' in data:
          my_project = data[b's'].decode("utf-8")
          gdb.execute('file ' + my_project)

          to_publish = base64.b64encode(msgpack.packb(
            {
            "s" : my_project,
            "i" : i
            })).decode("utf-8")

          r.publish(gdb_results_channel_name, to_publish)

        continue

      # Get project
      if b'p' not in data:
        # Every message other than start should have a project
        continue

      project = data[b'p'].decode("utf-8")
      if my_project != project:
        # Not my project
        continue

      # Disassembly
      if b'd' in data:
        disassemble_func = data[b'd']

        result = {}
        for func in disassemble_func:
          o = gdb.execute('disassemble ' + func.decode("utf-8"), to_string=True)
          result[func.decode("utf-8")] = o

        to_publish = base64.b64encode(msgpack.packb(
          {
          "p" : project,
          "i" : i,
          "d" : result
          })).decode("utf-8")

        r.publish(gdb_results_channel_name, to_publish)

      # Breakpoints
      if b'b' in data:
        breakpoints = data[b'b']

        result = {}
        for breakpoint in breakpoints:
          o = gdb.execute("break " + breakpoint.decode("utf-8"))
          result[breakpoint.decode('utf-8')] = o

        to_publish = base64.b64encode(msgpack.packb(
          {
          "p" : project,
          "i" : i,
          "b" : result
          })).decode("utf-8")

        r.publish(gdb_results_channel_name, to_publish)

      # Watchpoints
      if b'w' in data:
        watchpoints = data[b'w']

        result = {}
        for watchpoint in watchpoints:
          o = gdb.execute("watch " + watchpoint.decode("utf-8"))
          result[watchpoint.decode('utf-8')] = o

        to_publish = base64.b64encode(msgpack.packb(
          {
          "p" : project,
          "i" : i,
          "w" : result
          })).decode("utf-8")

        r.publish(gdb_results_channel_name, to_publish)

      # Commands
      if b'c' in data:
        c = data[b'c'].decode("utf-8")

        if c == "r":
          os.system("rm out.log")
          os.system("rm err.log")
          o = gdb.execute("run > out.log 2> err.log")
        else:
          o = gdb.execute(c, to_string=True)

        gdb.execute('call fflush(0)')

        out = open("out.log")
        err = open("err.log")

        to_publish = base64.b64encode(msgpack.packb(
          {
          "p" : project,
          "i" : i,
          "r" : o,
          "o" : out.read(),
          "e" : err.read()
          })).decode("utf-8")

        out.close()
        err.close()

        r.publish(gdb_results_channel_name, to_publish)
