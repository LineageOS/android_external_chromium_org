# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from future import Future


class ObjectStore(object):
  '''A class for caching picklable objects.
  '''
  def Get(self, key):
    '''Gets a |Future| with the value of |key| in the object store, or None
    if |key| is not in the object store.
    '''
    multi_get_future = self.GetMulti((key,))
    return Future(callback=lambda: multi_get_future.Get().get(key))

  def GetMulti(self, keys):
    '''Gets a |Future| with values mapped to |keys| from the object store, with
    any keys not in the object store omitted.
    '''
    raise NotImplementedError(self.__class__)

  def Set(self, key, value):
    '''Sets key -> value in the object store.
    '''
    self.SetMulti({ key: value })

  def SetMulti(self, items):
    '''Atomically sets the mapping of keys to values in the object store.
    '''
    raise NotImplementedError(self.__class__)

  def Del(self, key):
    '''Deletes a key from the object store.
    '''
    self.DelMulti([key])

  def DelMulti(self, keys):
    '''Deletes |keys| from the object store.
    '''
    raise NotImplementedError(self.__class__)
