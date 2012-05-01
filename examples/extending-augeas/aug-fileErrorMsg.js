var libaugeas = require('../../build/Release/libaugeas');

/*
 * This module overrides function createAugeas() from libaugeas,
 * so that created Augeas object has two additional functions:
 * fileErrorMsg(fname) and dumpFileErrors().
 */



// Error message formatter, sample return value:
// "/etc/mke2fs.conf:3:0: Get did not match entire input"
function file_error(fname) {
    var aug = this;
    var base = "/augeas/files" + fname + "/error";
    var msg = aug.get(base + "/message");
    var line = aug.get(base + "/line");
    var col = aug.get(base + "/char");

    var ret = undefined;

    if (aug.nmatch(base)) {
        ret = fname;
        if (line)
            ret += ":" + line;
        if (col)
            ret += ":" + col;

        if (msg)
            ret += ": " + msg;
        else
            ret += ": unknown error";
    }
    
    return ret;
}

// Returns an array of error messages for every file with error:
function dump_file_error() {
    var aug = this;

    var files_with_errors =
        aug.match(
                '/augeas/files/*/*[error]'
        ).map(function(s){
            return s.substr(13); // cutoff /augeas/files: 13 = strlen(/augeas/files)
         });

    var res = 
        files_with_errors.map(function(fname){
            return aug.fileErrorMsg(fname);
        });

    return res;
}

function createAugeas(cb) {
    if (undefined == cb) {
        var aug = libaugeas.createAugeas();
        aug.fileErrorMsg = file_error;
        aug.dumpFileErrors = dump_file_error;
        return aug;
    } else {
        libaugeas.createAugeas(function(aug) {
            aug.fileErrorMsg = file_error;
            aug.dumpFileErrors = dump_file_error;
            cb(aug);
        })
    }
}

exports.createAugeas = createAugeas;

