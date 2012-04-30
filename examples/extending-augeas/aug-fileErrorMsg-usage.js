var libaugeas = require('./aug-fileErrorMsg.js');


// should give an error if running unprivileged:
var testfile = '/etc/sudoers';


// Creating Augeas asynchronously:
libaugeas.createAugeas(function(aug) {
    console.log(aug.fileErrorMsg(testfile));
    console.log(aug.dumpFileErrors());
});

/* Example output:
/etc/sudoers: Permission denied
[ '/etc/mke2fs.conf:3:0: Get did not match entire input',
  '/etc/host.conf:15:0: Iterated lens matched less than it should',
  '/etc/securetty: Permission denied',
  '/etc/sysctl.conf: Permission denied',
  '/etc/sudoers: Permission denied' ]
*/ 
