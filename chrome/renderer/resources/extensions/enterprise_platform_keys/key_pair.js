// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var utils = require('utils');
var intersect = require('enterprise.platformKeys.utils').intersect;
var keyModule = require('enterprise.platformKeys.Key');
var Key = keyModule.Key;
var KeyType = keyModule.KeyType;
var KeyUsage = keyModule.KeyUsage;

/**
 * Implementation of WebCrypto.KeyPair used in enterprise.platformKeys.
 * @param {ArrayBuffer} publicKeySpki The Subject Public Key Info in DER
 *   encoding.
 * @param {KeyAlgorithm} algorithm The algorithm identifier.
 * @param {KeyUsage[]} usages The allowed key usages.
 * @constructor
 */
var KeyPairImpl = function(publicKeySpki, algorithm, usages) {
  this.publicKey = new Key(KeyType.public,
                           publicKeySpki,
                           algorithm,
                           intersect([KeyUsage.verify], usages),
                           true /* extractable */);
  this.privateKey = new Key(KeyType.private,
                            publicKeySpki,
                            algorithm,
                            intersect([KeyUsage.sign], usages),
                            false /* not extractable */);
};

exports.KeyPair = utils.expose('KeyPair',
                               KeyPairImpl,
                               {readonly:['publicKey', 'privateKey']});
