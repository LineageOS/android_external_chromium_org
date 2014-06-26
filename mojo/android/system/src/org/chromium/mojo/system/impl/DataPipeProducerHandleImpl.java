// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.system.impl;

import org.chromium.mojo.system.DataPipe.ProducerHandle;
import org.chromium.mojo.system.DataPipe.WriteFlags;

import java.nio.ByteBuffer;

/**
 * Implementation of {@link ProducerHandle}.
 */
class DataPipeProducerHandleImpl extends HandleBase implements ProducerHandle {

    /**
     * @see HandleBase#HandleBase(CoreImpl, int)
     */
    DataPipeProducerHandleImpl(CoreImpl core, int mojoHandle) {
        super(core, mojoHandle);
    }

    /**
     * @see HandleBase#HandleBase(HandleBase)
     */
    DataPipeProducerHandleImpl(UntypedHandleImpl handle) {
        super(handle);
    }

    /**
     * @see ProducerHandle#writeData(ByteBuffer, WriteFlags)
     */
    @Override
    public int writeData(ByteBuffer elements, WriteFlags flags) {
        return mCore.writeData(this, elements, flags);
    }

    /**
     * @see ProducerHandle#beginWriteData(int, WriteFlags)
     */
    @Override
    public ByteBuffer beginWriteData(int numBytes, WriteFlags flags) {
        return mCore.beginWriteData(this, numBytes, flags);
    }

    /**
     * @see ProducerHandle#endWriteData(int)
     */
    @Override
    public void endWriteData(int numBytesWritten) {
        mCore.endWriteData(this, numBytesWritten);
    }

}
