var augeas = require('../build/Release/libaugeas');

augeas.createAugeas({
    flags: augeas.AUG_NO_MODL_AUTOLOAD,
    srun: [
    'set /augeas/load/1/lens Hosts.lns',
    'set /augeas/load/1/incl /etc/hosts',
    'load',
    'defnode hosts /files/etc/hosts'
    ]},
    function(aug) {
        if (!aug.error()) {
            console.log(aug.get('$hosts/1/ipaddr'));
            console.log(aug.match('$hosts/*/*'));
        } else {
            console.log(aug.errorMsg());
        }
    }
);
console.log('Waiting for Augeas ...');

/* Example output:
Waiting for Augeas ...
127.0.0.1
[ '/files/etc/hosts/1/ipaddr',
  '/files/etc/hosts/1/canonical',
  '/files/etc/hosts/1/alias[1]',
  '/files/etc/hosts/1/alias[2]',
  '/files/etc/hosts/1/alias[3]',
  '/files/etc/hosts/1/alias[4]',
  '/files/etc/hosts/2/ipaddr',
  '/files/etc/hosts/2/canonical',
  '/files/etc/hosts/3/ipaddr',
  '/files/etc/hosts/3/canonical' ]
*/

