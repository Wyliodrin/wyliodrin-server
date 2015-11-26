'use strict'

var assert = require('assert');

var test1  = require('./tests/test1');
var test2  = require('./tests/test2');
var test3  = require('./tests/test3');
var test4  = require('./tests/test4');
var test5  = require('./tests/test5');
var test6  = require('./tests/test6');
var test7  = require('./tests/test7');
var test8  = require('./tests/test8');
var test9  = require('./tests/test9');
var test10 = require('./tests/test10');

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

  describe('Test3', function() {
    before(test3.run);
    it(test3.desc, function() {
      assert.equal(test3.is_test_passed, true);
    });
  });

  describe('Test4', function() {
    before(test4.run);
    it(test4.desc, function() {
      assert.equal(test4.is_test_passed, true);
    });
  });

  describe('Test5', function() {
    before(test5.run);
    it(test5.desc, function() {
      assert.equal(test5.is_test_passed, true);
    });
  });

  describe('Test6', function() {
    before(test6.run);
    it(test6.desc, function() {
      assert.equal(test6.is_test_passed, true);
    });
  });

  describe('Test7', function() {
    before(test7.run);
    it(test7.desc, function() {
      assert.equal(test7.is_test_passed, true);
    });
  });

  describe('Test8', function() {
    before(test8.run);
    it(test8.desc, function() {
      assert.equal(test8.is_test_passed, true);
    });
  });

  describe('Test9', function() {
    before(test9.run);
    it(test9.desc, function() {
      assert.equal(test9.is_test_passed, true);
    });
  });

  describe('Test10', function() {
    before(test10.run);
    it(test10.desc, function() {
      assert.equal(test10.is_test_passed, true);
    });
  });
});
