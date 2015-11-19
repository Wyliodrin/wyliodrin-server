'use strict'

var assert = require('assert');
var exec = require('child_process').exec;

var wyliodrinJsonPath = '/etc/wyliodrin';

var is_board_online = false;

function replaceWyliodrinJson() {
  exec('mv ' + wyliodrinJsonPath + '/wyliodrin.json ' +
               wyliodrinJsonPath + '/wyliodrin.jsonBCK',
    function(error, stdout, stderr) {});

  exec('cp res/wyliodrin.json ' +
           wyliodrinJsonPath + '/wyliodrin.json',
    function (error, stdout, stderr) {});
}

function restoreWyliodrinJson() {
  exec('mv ' + wyliodrinJsonPath + '/wyliodrin.jsonBCK ' +
               wyliodrinJsonPath + '/wyliodrin.json',
    function (error, stdout, stderr) {});
}

function startWyliodrind() {
  exec('wyliodrind',
    function (error, stdout, stderr) {});
}

function killWyliodrind() {
  exec('kill -9 $(pgrep wyliodrind)',
    function (error, stdout, stderr) {
        // console.log('stdout: ' + stdout);
        // console.log('stderr: ' + stderr);
        // if (error !== null) {
        //   console.log('exec error: ' + error);
        // }
    });
}

function connectOwnerAndWaitForBoard(done) {
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
      is_board_online = true;

      /* Clean */
      killWyliodrind();
      restoreWyliodrinJson();
      client.end();
      done();
    }
  })

  replaceWyliodrinJson();
  startWyliodrind();
};


/* Connection tests */
describe('Board connection tests', function() {
  describe('Board should become online', function() {
    before(function(done) {
      connectOwnerAndWaitForBoard(done);
    })

    it('is_board_online should be true', function() {
      assert.equal(is_board_online, true);
    });
  });
});
