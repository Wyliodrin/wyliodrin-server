'use strict'

var assert = require('assert');

var test1 = require('./tests/test1');

/* Connection tests */
describe('All tests', function() {
  describe('Test1', function() {
    before(test1.run);
    it(test1.desc, function() {
      assert.equal(test1.is_test_passed, true);
    });
  });
});
