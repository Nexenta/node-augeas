var libaugeas;
try {
  libaugeas = require('./build/Release/libaugeas');
} catch (e) {
  libaugeas = require('./libaugeas');
}
module.exports = libaugeas;
