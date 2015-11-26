var exec = require('child_process').exec;
var spawn = require('child_process').spawn;

var child;

function startWyliodrinHypervisor() {
  child = spawn('wyliodrin_hypervisor', {detached: true});
}

function stopWyliodrinHypervisor() {
  process.kill(-child.pid);
}

startWyliodrinHypervisor();
setTimeout(function() {
  console.log("now kill");
  stopWyliodrinHypervisor();
}, 5000);