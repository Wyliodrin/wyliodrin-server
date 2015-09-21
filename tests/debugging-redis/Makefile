JID_OWNER="wyliodrin_test@wyliodrin.org"
JID_BOARD="wyliodrin_board@wyliodrin.org"
PASSWORD="wyliodrin"

test: test.c
	gcc -g test.c -o mytest

debugger:
	gdb -q -x debugger.py &

sim_board: test debugger
	/usr/bin/python3 sim_board.py

sim_owner: test
	/usr/bin/python3 sim_owner.py -d --jid $(JID_OWNER) --password $(PASSWORD) --to $(JID_BOARD)

clean:
	rm -rf mytest out.log err.log

stop_all_gdb:
	kill -9 $(pgrep gdb)
