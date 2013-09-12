var libaugeas = require('../build/Release/augeas');

var createAugeas = libaugeas.createAugeas;

var aug = createAugeas();

aug.defvar('thing', '/files/etc/hosts'); // set "thing" = /files/etc/hosts

console.log(aug.get('$thing/1/ipaddr')); // use "thing"

aug.defvar('thing'); // delete "thing"

// Throws exception:
try {
    console.log(aug.get('$thing/1/ipaddr'));
} catch(err) {
    console.log('Ok, caught: ' + err);
}


