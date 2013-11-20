// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.favicon;

import android.graphics.Bitmap;
import android.graphics.Color;

import org.chromium.base.CalledByNative;
import org.chromium.chrome.browser.profiles.Profile;

/**
 * This is a helper class to use favicon_service.cc's functionality.
 *
 * You can request a favicon image by web page URL. Note that an instance of this class should be
 * created & used & destroyed (by destroy()) in the same thread due to the C++ CancelableTaskTracker
 * class requirement.
 */
public class FaviconHelper {

    // Please keep in sync with favicon_types.h's IconType.
    public static final int INVALID_ICON = 0;
    public static final int FAVICON = 1 << 0;
    public static final int TOUCH_ICON = 1 << 1;
    public static final int TOUCH_PRECOMPOSED_ICON = 1 << 2;

    private long mNativeFaviconHelper;

    /**
     * Callback interface for getting the result from getLocalFaviconImageForURL method.
     */
    public interface FaviconImageCallback {
        /**
         * This method will be called when the result favicon is ready.
         * @param image   Favicon image.
         * @param iconUrl Favicon image's icon url.
         */
        @CalledByNative("FaviconImageCallback")
        public void onFaviconAvailable(Bitmap image, String iconUrl);
    }

    /**
     * Allocate and initialize the C++ side of this class.
     */
    public FaviconHelper() {
        mNativeFaviconHelper = nativeInit();
    }

    @Override
    protected void finalize() {
        // Ensure that destroy() was called.
        assert mNativeFaviconHelper == 0;
    }

    /**
     * Clean up the C++ side of this class. After the call, this class instance shouldn't be used.
     */
    public void destroy() {
        nativeDestroy(mNativeFaviconHelper);
        mNativeFaviconHelper = 0;
    }

    /**
     * Get Favicon bitmap for the requested arguments. Retrieves favicons only for pages the user
     * has visited on the current device.
     * @param profile               Profile used for the FaviconService construction.
     * @param pageUrl               The target Page URL to get the favicon.
     * @param iconTypes             One of the IconType class values.
     * @param desiredSizeInDip      The size of the favicon in dip we want to get.
     * @param faviconImageCallback  A method to be called back when the result is available.
     *                              Note that this callback is not called if this method returns
     *                              false.
     * @return                      True if GetLocalFaviconImageForURL is successfully called.
     */
    public boolean getLocalFaviconImageForURL(
            Profile profile, String pageUrl, int iconTypes,
            int desiredSizeInDip, FaviconImageCallback faviconImageCallback) {
        assert mNativeFaviconHelper != 0;
        return nativeGetLocalFaviconImageForURL(mNativeFaviconHelper, profile, pageUrl, iconTypes,
                desiredSizeInDip, faviconImageCallback);
    }

    /**
     * Return the dominant color of a given bitmap in {@link Color} format.
     * @param image The bitmap image to find the dominant color for.
     * @return The dominant color in {@link Color} format.
     */
    public int getDominantColorForBitmap(Bitmap image) {
        return nativeGetDominantColorForBitmap(mNativeFaviconHelper, image);
    }

    /**
     * Get 16x16 Favicon bitmap for the requested arguments. Only retrives favicons in synced
     * session storage. (e.g. favicons synced from other devices).
     * TODO(apiccion): provide a way to obtain higher resolution favicons.
     * @param profile   Profile used for the FaviconService construction.
     * @param pageUrl   The target Page URL to get the favicon.
     *
     * @return          16x16 favicon Bitmap corresponding to the pageUrl.
     */
    public Bitmap getSyncedFaviconImageForURL(Profile profile, String pageUrl) {
        assert mNativeFaviconHelper != 0;
        return nativeGetSyncedFaviconImageForURL(mNativeFaviconHelper, profile, pageUrl);
    }

    private static native long nativeInit();
    private static native void nativeDestroy(long nativeFaviconHelper);
    private static native boolean nativeGetLocalFaviconImageForURL(long nativeFaviconHelper,
            Profile profile, String pageUrl, int iconTypes, int desiredSizeInDip,
            FaviconImageCallback faviconImageCallback);
    private static native Bitmap nativeGetSyncedFaviconImageForURL(long nativeFaviconHelper,
            Profile profile, String pageUrl);
    private static native int nativeGetDominantColorForBitmap(long nativeFaviconHelper,
            Bitmap image);
}
