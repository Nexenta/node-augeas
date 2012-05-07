var libaugeas = require('../build/Release/libaugeas');

var aug = libaugeas.createAugeas();

console.log('Before:');
aug.match('/files/etc/hosts/1/alias').forEach(function(p){
    console.log(p + ' = ' + aug.get(p));
});

aug.defnode('newalias',
        '/files/etc/hosts/1/alias[last()+1]',
        'myhost', // may be omitted (= empty string)
         function(created) {
            if (created) {
                console.log('New node created:');
                console.log(aug.match('$newalias')[0]);
            } else {
                console.log('No new node created');
            }
         });

console.log('After:');
aug.match('/files/etc/hosts/1/alias').forEach(function(p){
    console.log(p + ' = ' + aug.get(p));
});

