#!/bin/bash

JID="wyliodrin_test@wyliodrin.org"
PASSWORD="wyliodrin"
TO="matei94_galileiorg@wyliodrin.org"

/usr/bin/python test_shells.py -d --jid $JID --password $PASSWORD --to $TO
