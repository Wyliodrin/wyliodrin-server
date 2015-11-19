'use strict'

var assert = require('assert');

var test1 = require('./tests/test1');
var test2 = require('./tests/test2');

/* Connection tests */
describe('All tests', function() {
  describe('Test1', function() {
    before(test1.run);
    it(test1.desc, function() {
      assert.equal(test1.is_test_passed, true);
    });
  });

  describe('Test2', function() {
    before(test2.run);
    it(test2.desc, function() {
      assert.equal(test2.is_test_passed, true);
    });
  });
});
