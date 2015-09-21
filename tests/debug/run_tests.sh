#!/bin/bash

JID_OWNER="wyliodrin_test@wyliodrin.org"
JID_BOARD="wyliodrin_board@wyliodrin.org"
PASSWORD="wyliodrin"
num_tests_passed=0

test_name=(                                                                    \
  "Test start and close good debug session"                                    \
  "Test start debug session with wrong project id"                             \
  "Test simple run"                                                            \
  "Test both valid and invalid breakpoints"                                    \
)

printf "\n"

for i in `seq 1 ${#test_name[@]}`; do
  printf "\t[%d/%d]: %-60s - " "$i" "${#test_name[@]}" "${test_name[$i - 1]:0:60}"

  /usr/bin/python3 test$i.py -q --jid $JID_OWNER --password $PASSWORD --to $JID_BOARD

  if [ $? -eq 0 ]; then
    printf "PASSED\n"
    num_tests_passed=$(($num_tests_passed + 1))
  else
    printf "FAILED\n"
  fi
done

printf "\nTotal : %d/%d\n\n" "$num_tests_passed" "${#test_name[@]}"
