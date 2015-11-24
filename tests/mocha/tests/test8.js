'use strict'

var assert = require('assert');
var exec = require('child_process').exec;
var fs = require('fs');
var request = require('request');

var wyliodrinJsonPath = '/etc/wyliodrin';
var localLogsPath = '/etc/wyliodrin';
var logsPath = '/gadgets/logs/board@localhost';

var is_test_passed;

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

function removeLocalLogs() {
  exec('rm ' + localLogsPath + '/logs.*',
    function (error, stdout, stderr) {});
}

function run(done) {
  replaceWyliodrinJson();
  removeLocalLogs();
  startWyliodrind();

  setTimeout(function() {
    exec('/etc/init.d/openfire stop',
      function (error, stdout, stderr) {});
  }, 1000);

  setTimeout(function() {
    fs.readFile(localLogsPath + '/logs.err', 'utf-8', function(err, data) {
      if (err) throw err;

      if (data.indexOf('XMPP connection error') != -1) {
        module.exports.is_test_passed = true;
      }

      killWyliodrind();
      restoreWyliodrinJson();
      exec('/etc/init.d/openfire start',
        function (error, stdout, stderr) {});
      setTimeout(function() {
        done();
      }, 3000);
    });
  }, 2000);
};

module.exports = {
  run: run,
  desc: 'Test error log on xmpp server disconnected',
  is_test_passed: is_test_passed
};
