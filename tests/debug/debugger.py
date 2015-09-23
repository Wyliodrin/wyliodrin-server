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
build_path = "/etc/wyliodrin/build"



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

          try:
            gdb.execute('cd ' + build_path + "/" + data[b'x'].decode("utf-8"))
          except:
            to_publish = base64.b64encode(msgpack.packb(
              {
              "f" : "wrong project id",
              "i" : i
              })).decode("utf-8")

            r.publish(gdb_results_channel_name, to_publish)

            gdb.execute('q')

          gdb.execute('file ' + my_project)

          r.publish(gdb_results_channel_name, content['data'])

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
          try:
            o = gdb.execute('disassemble ' + func.decode("utf-8"), to_string=True)
          except:
            o = "error"
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
          try:
            o = gdb.execute("break " + breakpoint.decode("utf-8"), to_string=True)
            result[breakpoint.decode('utf-8')] = o
          except:
            result[breakpoint.decode('utf-8')] = "error"

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
          o = gdb.execute("watch " + watchpoint.decode("utf-8"), to_string=True)
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

        if c == "q":
          # Send back quit command
          r.publish(gdb_results_channel_name, content['data'])

        if c == "r":
          os.system("rm out.log")
          os.system("rm err.log")
          try:
            o = gdb.execute("run > out.log 2> err.log", to_string=True)
          except:
            o = "error"
        else:
          try:
            o = gdb.execute(c, to_string=True)
          except:
            o = "error"

        try:
          gdb.execute('call fflush(0)')
        except:
          # Will fail when the process is finished
          pass

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
