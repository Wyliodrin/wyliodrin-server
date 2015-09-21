To improve:
  * Use separate channel for every debug session: gdb_commands:projectid

To run demo:
  * start redis-server
  * run ``make sim_board`` in one terminal
  * run ``make sim_owner`` is another terminal

Tested with gdb 7.9

Add to ``/etc/gdb/gdbinit`` (no confirmation on quit):
```
define hook-quit
    set confirm off
end
```

Add to ``/etc/gdb/gdbinit`` (no user query when unrecongnized breakpoint):
```
set breakpoint pending off
```

When se
