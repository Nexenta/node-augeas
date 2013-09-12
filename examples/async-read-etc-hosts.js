var aug = require('../build/Release/augeas');


/* 
 * Create Augeas object to work with '/etc/hosts'
 * through 'hosts' lens:
 */
aug.createAugeas(
        {lens: 'hosts', incl: '/etc/hosts'},
        function(aug) {
            if (!aug.error()) {
                console.log(aug.get('/files/etc/hosts/1/ipaddr'));
                console.log(aug.match('/files/etc/hosts/*/*'));
            } else {
                console.log('Sad, but true :-(');
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

