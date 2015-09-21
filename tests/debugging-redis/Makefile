JID_OWNER="wyliodrin_test@wyliodrin.org"
JID_BOARD="wyliodrin_board@wyliodrin.org"
PASSWORD="wyliodrin"

test: test.c
	gcc -g test.c -o mytest

test2: test2.c
	gcc -g test2.c -o mytest2

debugger:
	gdb -q -x debugger.py &

sim_board: test test2 debugger
	/usr/bin/python3 sim_board.py

sim_owner: test test2
	/usr/bin/python3 sim_owner.py -q --jid $(JID_OWNER) --password $(PASSWORD) --to $(JID_BOARD)

clean:
	rm -rf mytest mytest2 out.log err.log

stop_all_gdb:
	kill -9 $(pgrep gdb)
