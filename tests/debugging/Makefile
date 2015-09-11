JID_OWNER="wyliodrin_test@wyliodrin.org"
JID_BOARD="wyliodrin_board@wyliodrin.org"
PASSWORD="wyliodrin"

test: test.c
	gcc -g test.c -o test

sim_board: test
	gdb -q -x sim_board.py

sim_owner: test
	/usr/bin/python3 sim_owner.py -d --jid $(JID_OWNER) --password $(PASSWORD) --to $(JID_BOARD)

clean:
	rm -rf test out.log err.log