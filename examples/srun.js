var libaugeas = require('../build/Release/libaugeas');

var aug = libaugeas.createAugeas();

console.log(aug.get('/files/etc/hosts/1/ipaddr'));
console.log(aug.get('/files/etc/hosts/1/canonical'));

aug.srun([
        'set /files/etc/hosts/1/ipaddr 127.1.2.23',
        'set /files/etc/hosts/1/canonical sweethome',
        ]);

console.log(aug.get('/files/etc/hosts/1/ipaddr'));
console.log(aug.get('/files/etc/hosts/1/canonical'));

/* Example output:
 * 127.0.0.1
 * localhost
 * 127.1.2.23
 * sweethome
 */
