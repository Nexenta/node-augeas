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

var libaugeas;
try {
        libaugeas = require('./build/Release/augeas');
} catch (e) {
        libaugeas = require('./augeas');
}

module.exports = libaugeas;
