'use strict'

var assert = require('assert');
var exec = require('child_process').exec;
var spawn = require('child_process').spawn;
var fs = require('fs');
var request = require('request');

var wyliodrinJsonPath = '/etc/wyliodrin';
var localLogsPath = '/etc/wyliodrin';
var logsPath = '/gadgets/logs/board@localhost';

var is_test_passed;
var wyliodrin_proc;

function replaceWyliodrinJson() {
  exec('mv ' + wyliodrinJsonPath + '/wyliodrin.json ' +
               wyliodrinJsonPath + '/wyliodrin.jsonBCK',
    function(error, stdout, stderr) {});

  exec('cp res/invalid_jid_wyliodrin.json ' +
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

function removeLocalLogs() {
  exec('rm ' + localLogsPath + '/logs.*',
    function (error, stdout, stderr) {});
}

function run(done) {
  replaceWyliodrinJson();
  removeLocalLogs();
  startWyliodrind();

  setTimeout(function() {
    fs.readFile(localLogsPath + '/logs.err', 'utf-8', function(err, data) {
      if (err) throw err;

      if (data.indexOf('Retrying to connect') != -1) {
        module.exports.is_test_passed = true;
      }

      killWyliodrind();
      restoreWyliodrinJson();
      done();
    });
  }, 2000);
};

module.exports = {
  run: run,
  desc: 'Test retry to connect on xmpp connection error',
  is_test_passed: is_test_passed
};
