import base64
import gdb
import logging
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
is_project_loaded = False



if __name__ == '__main__':
  logging.basicConfig(level=logging.DEBUG,
            format='%(levelname)-8s %(message)s')

  r = redis.Redis()
  pubsub = r.pubsub()
  pubsub.subscribe([gdb_commands_channel_name])

  while True:
    # Get commands
    for content in pubsub.listen():
      logging.info(content)

      if (content['data'] is None) or (content['type'] != "message"):
        continue

      data = msgpack.unpackb(base64.b64decode(content['data']))

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

        r.publish(gdb_results_channel_name, to_write)


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

        r.publish(gdb_results_channel_name, to_write)
