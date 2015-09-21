#!/bin/bash

JID_OWNER="wyliodrin_test@wyliodrin.org"
JID_BOARD="wyliodrin_board@wyliodrin.org"
PASSWORD="wyliodrin"
NUM_TESTS=1

test_name=(                                                                    \
  "Test start new debug session"                                               \
)

for i in `seq 1 $(($NUM_TESTS))`; do
  printf "${test_name[$i - 1]} ... "

  /usr/bin/python3 test$i.py -q --jid $JID_OWNER --password $PASSWORD --to $JID_BOARD

  if [ $? -eq 0 ]; then
    printf "PASSED\n"
  else
    printf "FAILED\n"
  fi
done
