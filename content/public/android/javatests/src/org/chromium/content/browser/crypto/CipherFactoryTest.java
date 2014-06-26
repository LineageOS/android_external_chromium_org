// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.crypto;

import android.os.Bundle;
import android.test.InstrumentationTestCase;

import java.io.IOException;
import java.security.GeneralSecurityException;
import java.util.Arrays;

import javax.crypto.Cipher;

/**
 * Functional tests for the {@link CipherFactory}. Confirms that saving and restoring data works, as
 * well as that {@link Cipher} instances properly encrypt and decrypt data.
 *
 * Tests that confirm that the class is thread-safe would require putting potentially flaky hooks
 * throughout the class to simulate artificial blockages.
 */
public class CipherFactoryTest extends InstrumentationTestCase {
    private static final byte[] INPUT_DATA = {1, 16, 84};

    /** Generates non-random byte[] for testing. */
    private static class DeterministicParameterGenerator extends ByteArrayGenerator {
        @Override
        public byte[] getBytes(int numBytes) throws IOException, GeneralSecurityException {
            return getBytes(numBytes, (byte) 0);
        }

        /**
         * Generates a linearly-increasing byte[] sequence that wraps around after 0xFF.
         * @param numBytes Length of the byte[] to create.
         * @param startByte Byte to start at.
         * @return The completed byte[].
         */
        public byte[] getBytes(int numBytes, byte startByte) {
            byte[] bytes = new byte[numBytes];
            for (int i = 0; i < numBytes; ++i) {
                bytes[i] = (byte) (startByte + i);
            }
            return bytes;
        }
    }
    private DeterministicParameterGenerator mNumberProvider;

    /**
     * Overrides the {@link ByteArrayGenerator} used by the {@link CipherFactory} to ensure
     * deterministic results.
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mNumberProvider = new DeterministicParameterGenerator();
        CipherFactory.getInstance().setRandomNumberProviderForTests(mNumberProvider);
    }

    /**
     * {@link Cipher} instances initialized using the same parameters work in exactly the same way.
     */
    public void testCipherUse() throws Exception {
        // Check encryption.
        Cipher aEncrypt = CipherFactory.getInstance().getCipher(Cipher.ENCRYPT_MODE);
        Cipher bEncrypt = CipherFactory.getInstance().getCipher(Cipher.ENCRYPT_MODE);
        byte[] output = sameOutputDifferentCiphers(INPUT_DATA, aEncrypt, bEncrypt);

        // Check decryption.
        Cipher aDecrypt = CipherFactory.getInstance().getCipher(Cipher.DECRYPT_MODE);
        Cipher bDecrypt = CipherFactory.getInstance().getCipher(Cipher.DECRYPT_MODE);
        byte[] decrypted = sameOutputDifferentCiphers(output, aDecrypt, bDecrypt);
        assertTrue(Arrays.equals(decrypted, INPUT_DATA));
    }

    /**
     * Restoring a {@link Bundle} containing the same parameters already in use by the
     * {@link CipherFactory} should keep the same keys.
     */
    public void testSameBundleRestoration() throws Exception {
        // Create two bundles with the same saved state.
        Bundle aBundle = new Bundle();
        Bundle bBundle = new Bundle();

        byte[] sameIv = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 100);
        aBundle.putByteArray(CipherFactory.BUNDLE_IV, sameIv);
        bBundle.putByteArray(CipherFactory.BUNDLE_IV, sameIv);

        byte[] sameKey = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 200);
        aBundle.putByteArray(CipherFactory.BUNDLE_KEY, sameKey);
        bBundle.putByteArray(CipherFactory.BUNDLE_KEY, sameKey);

        // Restore using the first bundle, then the second. Both should succeed.
        assertTrue(CipherFactory.getInstance().restoreFromBundle(aBundle));
        Cipher aCipher = CipherFactory.getInstance().getCipher(Cipher.ENCRYPT_MODE);
        assertTrue(CipherFactory.getInstance().restoreFromBundle(bBundle));
        Cipher bCipher = CipherFactory.getInstance().getCipher(Cipher.ENCRYPT_MODE);

        // Make sure the CipherFactory instances are using the same key.
        sameOutputDifferentCiphers(INPUT_DATA, aCipher, bCipher);
    }

    /**
     * Restoring a {@link Bundle} containing a different set of parameters from those already in use
     * by the {@link CipherFactory} should fail. Any Ciphers created after the failed restoration
     * attempt should use the already-existing keys.
     */
    public void testDifferentBundleRestoration() throws Exception {
        // Restore one set of parameters.
        Bundle aBundle = new Bundle();
        byte[] aIv = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 50);
        byte[] aKey = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 100);
        aBundle.putByteArray(CipherFactory.BUNDLE_IV, aIv);
        aBundle.putByteArray(CipherFactory.BUNDLE_KEY, aKey);
        assertTrue(CipherFactory.getInstance().restoreFromBundle(aBundle));
        Cipher aCipher = CipherFactory.getInstance().getCipher(Cipher.ENCRYPT_MODE);

        // Restore using a different set of parameters.
        Bundle bBundle = new Bundle();
        byte[] bIv = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 150);
        byte[] bKey = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 200);
        bBundle.putByteArray(CipherFactory.BUNDLE_IV, bIv);
        bBundle.putByteArray(CipherFactory.BUNDLE_KEY, bKey);
        assertFalse(CipherFactory.getInstance().restoreFromBundle(bBundle));
        Cipher bCipher = CipherFactory.getInstance().getCipher(Cipher.ENCRYPT_MODE);

        // Make sure they're using the same (original) key by encrypting the same data.
        sameOutputDifferentCiphers(INPUT_DATA, aCipher, bCipher);
    }

    /**
     * Restoration from a {@link Bundle} missing data should fail.
     */
    public void testIncompleteBundleRestoration() throws Exception {
        // Make sure we handle the null case.
        assertFalse(CipherFactory.getInstance().restoreFromBundle(null));

        // Try restoring without the key.
        Bundle aBundle = new Bundle();
        byte[] iv = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 50);
        aBundle.putByteArray(CipherFactory.BUNDLE_IV, iv);
        assertFalse(CipherFactory.getInstance().restoreFromBundle(aBundle));

        // Try restoring without the initialization vector.
        Bundle bBundle = new Bundle();
        byte[] key = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 100);
        bBundle.putByteArray(CipherFactory.BUNDLE_KEY, key);
        assertFalse(CipherFactory.getInstance().restoreFromBundle(bBundle));
    }

    /**
     * Parameters should only be saved when they're needed by the {@link CipherFactory}. Restoring
     * parameters from a {@link Bundle} before this point should result in {@link Cipher}s using the
     * restored parameters instead of any generated ones.
     */
    public void testRestorationSucceedsBeforeCipherCreated() throws Exception {
        byte[] iv = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 50);
        byte[] key = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 100);
        Bundle bundle = new Bundle();
        bundle.putByteArray(CipherFactory.BUNDLE_IV, iv);
        bundle.putByteArray(CipherFactory.BUNDLE_KEY, key);

        // The keys should be initialized only after restoration.
        assertNull(CipherFactory.getInstance().getCipherData(false));
        assertTrue(CipherFactory.getInstance().restoreFromBundle(bundle));
        assertNotNull(CipherFactory.getInstance().getCipherData(false));
    }

    /**
     * If the {@link CipherFactory} has already generated parameters, restorations of different data
     * should fail. All {@link Cipher}s should use the generated parameters.
     */
    public void testRestorationDiscardsAfterOtherCipherAlreadyCreated() throws Exception {
        byte[] iv = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 50);
        byte[] key = mNumberProvider.getBytes(CipherFactory.NUM_BYTES, (byte) 100);
        Bundle bundle = new Bundle();
        bundle.putByteArray(CipherFactory.BUNDLE_IV, iv);
        bundle.putByteArray(CipherFactory.BUNDLE_KEY, key);

        // The keys should be initialized after creating the cipher, so the keys shouldn't match.
        Cipher aCipher = CipherFactory.getInstance().getCipher(Cipher.ENCRYPT_MODE);
        assertFalse(CipherFactory.getInstance().restoreFromBundle(bundle));
        Cipher bCipher = CipherFactory.getInstance().getCipher(Cipher.ENCRYPT_MODE);

        // B's cipher should use the keys generated for A.
        sameOutputDifferentCiphers(INPUT_DATA, aCipher, bCipher);
    }

    /**
     * Data saved out to the {@link Bundle} should match what is held by the {@link CipherFactory}.
     */
    public void testSavingToBundle() throws Exception {
        // Nothing should get saved out before Cipher data exists.
        Bundle initialBundle = new Bundle();
        CipherFactory.getInstance().saveToBundle(initialBundle);
        assertFalse(initialBundle.containsKey(CipherFactory.BUNDLE_IV));
        assertFalse(initialBundle.containsKey(CipherFactory.BUNDLE_KEY));

        // Check that Cipher data gets saved if it exists.
        CipherFactory.getInstance().getCipher(Cipher.ENCRYPT_MODE);
        Bundle afterBundle = new Bundle();
        CipherFactory.getInstance().saveToBundle(afterBundle);
        assertTrue(afterBundle.containsKey(CipherFactory.BUNDLE_IV));
        assertTrue(afterBundle.containsKey(CipherFactory.BUNDLE_KEY));

        // Confirm the saved keys match by restoring it.
        assertTrue(CipherFactory.getInstance().restoreFromBundle(afterBundle));
    }

    /**
     * Confirm that the two {@link Cipher}s are functionally equivalent.
     * @return The input after it has been operated on (e.g. decrypted or encrypted).
     */
    private byte[] sameOutputDifferentCiphers(byte[] input, Cipher aCipher, Cipher bCipher)
            throws Exception {
        assertNotNull(aCipher);
        assertNotNull(bCipher);
        assertNotSame(aCipher, bCipher);

        byte[] aOutput = aCipher.doFinal(input);
        byte[] bOutput = bCipher.doFinal(input);

        assertNotNull(aOutput);
        assertNotNull(bOutput);
        assertTrue(Arrays.equals(aOutput, bOutput));

        return aOutput;
    }
}
