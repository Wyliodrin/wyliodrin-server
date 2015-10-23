#!/bin/bash


JID="ajid@localhost"
WYLIODRIN_JSON_PATH="/etc/wyliodrin/wyliodrin.json"
num_tests_passed=0


function replace_wyliodrin_json {
  # Keep original
  sudo mv $WYLIODRIN_JSON_PATH $WYLIODRIN_JSON_PATH.orig

  # Write wyliodrin.json
  printf '{
  "jid": "%s",
  "password": "apassword",
  "owner": "owner@localhost",
  "ssid": "",
  "psk": ""
}' $JID > $WYLIODRIN_JSON_PATH
}


test_name=(                                                                                       \
  "Test logs are sent to <domain>/gadgets/logs/<jid>"                                             \
  "Test first log is info about startup"                                                          \
  "Test connection error logs"                                                                    \
)


replace_wyliodrin_json

printf "\n\tWyliodrin Automated Testing\n\n"

for i in `seq 1 ${#test_name[@]}`; do
  printf "[%d/%d]: %-60s - " "$i" "${#test_name[@]}" "${test_name[$i - 1]:0:60}"

  # Start server in background
  sudo python servers/server$i.py $JID > /dev/null 2>&1 &
  server_pid=$!

  # Wait for server to start
  sleep 1

  # Start wyliodrind in background
  sudo wyliodrind > /dev/null 2>&1 &
  wyliodrind_pid=$!

  # Wait for server to finish
  wait $server_pid
  status=$?

  # Clean
  sudo kill $wyliodrind_pid

  if [ $status -eq 0 ]; then
    printf "PASSED\n"
    num_tests_passed=$(($num_tests_passed + 1))
  else
    printf "FAILED\n"
  fi
done

printf "\nTotal : %d/%d\n\n" "$num_tests_passed" "${#test_name[@]}"

# Restore old wyliodrin.json
sudo mv $WYLIODRIN_JSON_PATH.orig $WYLIODRIN_JSON_PATH
