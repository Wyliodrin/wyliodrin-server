'use strict'

var assert = require('assert');
var exec = require('child_process').exec;
var spawn = require('child_process').spawn;
var https = require('https');
var fs = require('fs');
var request = require('request');

var wyliodrinJsonPath = '/etc/wyliodrin';
var logsPath = '/gadgets/logs/board@localhost';

var is_test_passed;
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

function killWyliodrind() {
  process.kill(-wyliodrin_proc.pid);
}


function run(done) {
  replaceWyliodrinJson();
  startWyliodrind();

  setTimeout(function() {
    var server = https.createServer(options, function (req, res) {
      if (req.method == 'POST' && req.url == logsPath) {
        module.exports.is_test_passed = true;
        restoreWyliodrinJson();
        killWyliodrind();
        server.close();
        done();
      }

      res.writeHead(200);
      res.end();
    }).listen(443);
  }, 1000);
};

module.exports = {
  run: run,
  desc: 'Board sends logs',
  is_test_passed: is_test_passed
};
