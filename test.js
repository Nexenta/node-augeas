/*
 * Copyright (C) 2012, Nexenta Systems, Inc.
 *
 * The contents of this file are subject to the terms of
 * the Common Development and Distribution License ("CDDL").
 * You may not use this file except in compliance with this license.
 *
 * You can obtain a copy of the License at
 * http://www.opensource.org/licenses/CDDL-1.0
 */

var libaugeas = require('./index');

var aug = libaugeas.createAugeas("/code/nef/etc/augeas-lenses");

//
// Example of asyncronous interface
//
aug.loadFile({lens: "Corosync", file: "/etc/corosync/corosync.conf"},
             function (err, msg) {
    console.log(err, msg);
    aug.saveFile({lens: "Corosync", file: "/etc/corosync/corosync.conf"},
                 {K: 'V'}, function (err, msg) {
        console.log(err, msg);
    });
});
