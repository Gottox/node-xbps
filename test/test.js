var assert = require("assert")
var Xbps = require("../index.js").Xbps;

describe('Xbps', function() {
	describe('new instance', function() {
		var xbps = new Xbps();
		assert.equal(xbps.target_arch, null);
	});
});
