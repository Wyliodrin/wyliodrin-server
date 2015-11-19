'use strict'

var test_passed = false;
var assert = require('assert');

before(function(done) {
  var Client = require('node-xmpp-client')
  var argv = process.argv

  /* Credentials */
  var board    = 'board@x550jk'
  var owner    = 'owner@x550jk'
  var password = "wyliodrin"

  var client = new Client({
    port: 5222,
    jid: owner,
    password: password,
  })

  client.on('online', function() {
    // console.log('online')

    /* Send priority */
    client.send(new Client.Element('presence', {})
      .c('priority').t('50')
    )

    /* Send status */
    client.send(new Client.Element('presence', {})
      .c('status').t('Happily echoing your <message/> stanzas')
    )
  })

  client.on('stanza', function (stanza) {
    // console.log('stanza: ' + stanza.toString() + '\n');

    /* Manage subscription */
    if (stanza.is('presence') && (stanza.attrs.from.indexOf(board) !== -1) &&
        (stanza.attrs.type == 'subscribe')) {
      console.log("Subscripton");
      client.send(new Client.Element('presence', {to: board, type: 'subscribed'}))
      client.send(new Client.Element('presence', {to: board, type: 'subscribe'}))
      client.send(new Client.Element('presence', {})
        .c('status').t('Happily echoing your <message/> stanzas')
      )
    }

    if (stanza.is('message') && (stanza.attrs.from.indexOf(board) !== -1)) {
      test_passed = true;
      client.end();
      done();
    }
  })
});

/* Connection tests */
describe('Connection', function() {
  describe('User connects with valid credentials', function () {
    it('test_passed should be true', function () {
      assert.equal(test_passed, true);
    });
  });
});
