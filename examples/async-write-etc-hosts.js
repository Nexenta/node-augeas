var aug = require('../build/Release/libaugeas');

// NOTE:
// running this example as non-root user will probably fail,
// but for the goodness sake we set AUG_SAVE_NEWFILE flag here,
// so your /etc/hosts will be kept untouched :-)
aug.createAugeas(
        {lens: 'hosts', incl: '/etc/hosts', flags:aug.AUG_SAVE_NEWFILE},
        function(aug) {
            if (!aug.error()) {
                console.log('Making changes ...');
                aug.set('/files/etc/hosts/1/ipaddr', '127.0.0.2');
                aug.set('/files/etc/hosts/1/canonical', 'localhost');
                aug.save(function(success) {
                    if (success) {
                        console.log('Saved! Take a look at /etc/hosts.augnew');
                    } else {
                        console.log('Failed :-( Here is the reason:');
                        console.log(aug.get('/augeas/files/etc/hosts/error/message'));
                    }
                });
                console.log('Saving started!');
            } else {
                console.log('Sad, but true :-(');
            }
        }
);
console.log('Waiting for Augeas ...');

