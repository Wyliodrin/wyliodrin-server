#!/bin/bash

JID="wyliodrin_test@wyliodrin.org"
PASSWORD="wyliodrin"
TO="matei94_galileiorg@wyliodrin.org"
PATH="/home/matei/Dropbox/Wyliodrin/Work/wyliodrin-server/build/wyliodrind"

/usr/bin/python test_upload.py -d --jid $JID --password $PASSWORD --to $TO --path $PATH
