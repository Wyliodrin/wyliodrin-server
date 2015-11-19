'use strict'

var assert = require('assert');
var exec = require('child_process').exec;

var wyliodrinJsonPath = '/etc/wyliodrin';

var is_test_passed;

var request = require('request');

var username = 'admin',
    password = 'server',
    url = 'http://' + username + ':' + password +
          '@x550jk:9090/plugins/restapi/v1/sessions/board';

var options = {
  url: url,
  headers: {
    'Accept': 'application/json'
  }
};

function callback(error, response, body) {
  var res = JSON.parse(body);
  var size = Object.keys(res).length;
  if (size == 1) {
    module.exports.is_test_passed = true;
  }
}

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

function run(done) {
  replaceWyliodrinJson();
  startWyliodrind();

  setTimeout(function() {
    request(options, function (error, response, body) {
      callback(error, response, body);
      restoreWyliodrinJson();
      killWyliodrind();
      done();
    });
  }, 1000);
};

module.exports = {
  run: run,
  desc: 'Board connects to XMPP (tested via REST api)',
  is_test_passed: is_test_passed
};
