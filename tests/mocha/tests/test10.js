'use strict'

var assert = require('assert');
var exec = require('child_process').exec;
var spawn = require('child_process').spawn;
var https = require('https');
var fs = require('fs');

var wyliodrinJsonPath = '/etc/wyliodrin';
var logsPath = '/gadgets/logs/board@localhost';

var is_test_passed;
var listen_for_error_logs = false;

var hypervisor_proc;
var wyliodrin_proc;

var options = {
  key: fs.readFileSync('res/key.pem'),
  cert: fs.readFileSync('res/cert.pem')
};

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
  wyliodrin_proc = spawn('wyliodrind', {detached: true});
}

function startWyliodrinHypervisor() {
  hypervisor_proc = spawn('wyliodrin_hypervisor', {detached: true});
}

function killWyliodrind() {
  process.kill(-wyliodrin_proc.pid);
}

function killWyliodrinHypervisor() {
  process.kill(-hypervisor_proc.pid);
}

function run(done) {
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

      killWyliodrinHypervisor();

      var shells_stz = new Client.Element('shells', {
        request: "0",
        width: "80",
        height: "10",
        action: "open"
      });
      shells_stz.attrs.xmlns = "wyliodrin";
      var msg_stz = new Client.Element('message', {to : board}).cnode(shells_stz);

      client.send(msg_stz);

      setTimeout(function() {
        var shells_stz = new Client.Element('shells', {
          request: "1",
          width: "80",
          height: "10",
          action: "open"
        });
        shells_stz.attrs.xmlns = "wyliodrin";
        var msg_stz = new Client.Element('message', {to : board}).cnode(shells_stz);

        client.send(msg_stz);
        listen_for_error_logs = true;

        setTimeout(function() {
          var shells_stz = new Client.Element('shells', {
            request: "2",
            width: "80",
            height: "10",
            action: "open"
          });
          shells_stz.attrs.xmlns = "wyliodrin";
          var msg_stz = new Client.Element('message', {to : board}).cnode(shells_stz);

          client.send(msg_stz);
          listen_for_error_logs = true;
        }, 1000);

      }, 5000);
    }
  });

  replaceWyliodrinJson();
  startWyliodrinHypervisor();
  setTimeout(function() {
    startWyliodrind();
    var server = https.createServer(options, function (req, res) {
      if (listen_for_error_logs == true && req.method == 'POST' && req.url == logsPath) {
        var body = "";
        req.on('data', function (chunk) {
          body += chunk;
        });
        req.on('end', function () {
          if (body.indexOf("Hypervisor is dead") != -1) {
            module.exports.is_test_passed = true;
          } else {
            module.exports.is_test_passed = false;
          }
          restoreWyliodrinJson();
          killWyliodrind();
          client.end();
          server.close();
          done();
        });
      }

      res.writeHead(200);
      res.end();
    }).listen(443);
  }, 2000);
};

module.exports = {
  run: run,
  desc: 'Shell error mesage when hypervisor is dead',
  is_test_passed: is_test_passed
};
