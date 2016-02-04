'use strict'

var assert = require('assert');
var Client = require('node-xmpp-client')
var argv = process.argv

/* Credentials */
var board    = 'board@x550jk'
var owner    = 'owner@x550jk'
var password = "wyliodrin"

function run() {
  var client = new Client({
    port: 5222,
    jid: owner,
    password: password,
  })

  client.on('online', function() {
    /* I'm online */
    client.send(new Client.Element('presence', {})
      .c('priority').t('50')
    )

    /* Send status */
    client.send(new Client.Element('presence', {})
      .c('status').t('Happily echoing your <message/> stanzas')
    )
  })

  client.on('stanza', function (stanza) {
    console.log(stanza.toString() + "\n")

    /* Manage subscription */
    if (stanza.is('presence') && (stanza.attrs.from.indexOf(board) !== -1) &&
        (stanza.attrs.type == 'subscribe')) {
      client.send(new Client.Element('presence', {to: board, type: 'subscribed'}))
      client.send(new Client.Element('presence', {to: board, type: 'subscribe'}))
      client.send(new Client.Element('presence', {})
        .c('status').t('Happily echoing your <message/> stanzas')
      )
    }

    /* Check presence from board */
    if (stanza.is('presence') && (stanza.attrs.from.indexOf(board) !== -1)) {
      console.log("Board is online");
    }

    if (stanza.is('message') && (stanza.attrs.from.indexOf(board) !== -1) &&
        stanza.children.length == 1 && stanza.children[0].name === 'version') {
      console.log('children = ' + stanza.children.length);
    }
  })
}

run();
