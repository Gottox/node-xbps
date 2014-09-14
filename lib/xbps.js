var env = process.env.NODE_XBPS_ENV || 'Release';
var inherits = require('util').inherits;
module.exports = require('../build/'+env+'/xbps');

inherits(module.exports.Xbps, require('events').EventEmitter);
