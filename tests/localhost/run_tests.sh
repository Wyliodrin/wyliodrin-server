#!/bin/bash



### VARIABLES #####################################################################################

VALID_JID="user1@localhost"
INVALID_JID="notauser@localhost"
VALID_JID_PASSWORD="user1"
OWNER="owner@localhost"
WYLIODRIN_JSON_PATH="/etc/wyliodrin/wyliodrin.json"
num_tests_passed=0

###################################################################################################



### TEST 1 ########################################################################################

function run_test1 {
  # Keep original wyliodrind.json
  sudo mv $WYLIODRIN_JSON_PATH $WYLIODRIN_JSON_PATH.orig

  # Write wyliodrin.json
  printf '{
  "jid": "%s",
  "password": "%s",
  "owner": "%s",
  "ssid": "",
  "psk": ""
}' $VALID_JID $VALID_JID_PASSWORD $OWNER > $WYLIODRIN_JSON_PATH

  # Start server in background
  sudo python servers/server1.py $VALID_JID > /dev/null 2>&1 &
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

  # Restore old wyliodrin.json
  sudo mv $WYLIODRIN_JSON_PATH.orig $WYLIODRIN_JSON_PATH

  return $status
}

###################################################################################################



### TEST 2 ########################################################################################

function run_test2 {
  # Keep original wyliodrind.json
  sudo mv $WYLIODRIN_JSON_PATH $WYLIODRIN_JSON_PATH.orig

  # Write wyliodrin.json
  printf '{
  "jid": "%s",
  "password": "%s",
  "owner": "%s",
  "ssid": "",
  "psk": ""
}' $VALID_JID $VALID_JID_PASSWORD $OWNER > $WYLIODRIN_JSON_PATH

  # Start server in background
  sudo python servers/server2.py > /dev/null 2>&1 &
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

  # Restore old wyliodrin.json
  sudo mv $WYLIODRIN_JSON_PATH.orig $WYLIODRIN_JSON_PATH

  return $status
}

###################################################################################################



### TEST 3 ########################################################################################

function run_test3 {
  # Keep original wyliodrind.json
  sudo mv $WYLIODRIN_JSON_PATH $WYLIODRIN_JSON_PATH.orig

  # Write wyliodrin.json
  printf '{
  "jid": "%s",
  "password": "%s",
  "owner": "%s",
  "ssid": "",
  "psk": ""
}' $INVALID_JID $VALID_JID_PASSWORD $OWNER > $WYLIODRIN_JSON_PATH

  # Start server in background
  sudo python servers/server3.py > /dev/null 2>&1 &
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

  # Restore old wyliodrin.json
  sudo mv $WYLIODRIN_JSON_PATH.orig $WYLIODRIN_JSON_PATH

  return $status
}

###################################################################################################


### TEST 3 ########################################################################################

function run_test3 {
  # Keep original wyliodrind.json
  sudo mv $WYLIODRIN_JSON_PATH $WYLIODRIN_JSON_PATH.orig

  # Write wyliodrin.json
  printf '{
  "jid": "%s",
  "password": "%s",
  "owner": "%s",
  "ssid": "",
  "psk": ""
}' $VALID_JID $VALID_JID_PASSWORD $OWNER > $WYLIODRIN_JSON_PATH

  # Start server in background
  sudo python servers/server4.py > /dev/null 2>&1 &
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

  # Restore old wyliodrin.json
  sudo mv $WYLIODRIN_JSON_PATH.orig $WYLIODRIN_JSON_PATH

  return $status
}

###################################################################################################



### TEST DESCRIPTION ##############################################################################

test_name=(                                                                                       \
  "Test logs are sent to <domain>/gadgets/logs/<jid>"                                             \
  "Test first log is info about startup"                                                          \
  "Test connection with invalid jid"                                                              \
  "Test connection with valid jid"                                                                \
)

###################################################################################################



### TESTING #######################################################################################

printf "\n\tWyliodrin Automated Testing\n\n"

for i in `seq 1 ${#test_name[@]}`; do
  printf "[%d/%d]: %-60s - " "$i" "${#test_name[@]}" "${test_name[$i - 1]:0:60}"

  run_test$i

  if [ $? -eq 0 ]; then
    printf "PASSED\n"
    num_tests_passed=$(($num_tests_passed + 1))
  else
    printf "FAILED\n"
  fi
done

printf "\nTotal : %d/%d\n\n" "$num_tests_passed" "${#test_name[@]}"

###################################################################################################
