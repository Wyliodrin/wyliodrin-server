#!/bin/bash

JID="wyliodrin_test@wyliodrin.org"
PASSWORD="wyliodrin"
TO="matei94_galileiorg@wyliodrin.org"
PATH="/home/matei/Desktop/prez.pdf"

/usr/bin/python test_upload.py --jid $JID --password $PASSWORD --to $TO --path $PATH
