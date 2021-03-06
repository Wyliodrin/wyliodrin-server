'use strict'

var assert = require('assert');
var exec = require('child_process').exec;
var spawn = require('child_process').spawn;

var wyliodrinJsonPath = '/etc/wyliodrin';

var is_test_passed;
var wyliodrin_proc;

var gdone;

var request = require('request');

var username = 'admin',
    password = 'server',
    url = 'http://' + username + ':' + password +
          '@localhost:9090/plugins/restapi/v1/sessions/board';

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
    request.del(options, function (error, response, body) {
      setTimeout(function() {
        request(options, function (error, response, body) {
          var res = JSON.parse(body);
          var size = Object.keys(res).length;
          if (size == 1) {
            module.exports.is_test_passed = true;
          }
          killWyliodrind();
          restoreWyliodrinJson();
          gdone();
        });
      }, 3000);
    });
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
  wyliodrin_proc = spawn('wyliodrind', {detached: true});
}

function killWyliodrind() {
  process.kill(-wyliodrin_proc.pid);
}

function run(done) {
  gdone = done;
  replaceWyliodrinJson();
  startWyliodrind();

  setTimeout(function() {
    request(options, function (error, response, body) {
      callback(error, response, body);
    });
  }, 1000);
};

module.exports = {
  run: run,
  desc: 'Board reconnects to XMPP after kickout',
  is_test_passed: is_test_passed
};
