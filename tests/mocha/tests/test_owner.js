'use strict'

var exec = require('child_process').exec;

var wyliodrinJsonPath = '/etc/wyliodrin';

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
    function (error, stdout, stderr) {});
}

function run() {
  var Client = require('node-xmpp-client')
  var argv = process.argv

  /* Credentials */
  var board    = 'board@localhost';
  var owner    = 'owner@localhost';
  var password = "wyliodrin";

  var client = new Client({
    port: 5222,
    jid: owner,
    password: password
  })


  client.on('online', function() {
    console.log('online');
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
   if (stanza.is('message') && (stanza.attrs.from.indexOf(board) !== -1) &&
       (stanza.children.length == 1) && (stanza.children[0].name === 'version')) {
//      module.exports.is_test_passed = true;
      /* Clean */
      killWyliodrind();
      restoreWyliodrinJson();
      client.end();
    }
  })
}

run();