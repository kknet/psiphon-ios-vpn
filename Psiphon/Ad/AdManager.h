/*
 * Copyright (c) 2017, Psiphon Inc.
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#import <Foundation/Foundation.h>
#import <mopub-ios-sdk/MPInterstitialAdController.h>
#import <mopub-ios-sdk/MoPub.h>
#import <mopub-ios-sdk/MPRewardedVideo.h>

@class RACUnit;
@class RACSignal<__covariant ValueType>;
@class RACSubject<ValueType>;
@class RACReplaySubject<ValueType>;
@class InterstitialAdControllerWrapper;
@class RewardedAdControllerWrapper;

NS_ASSUME_NONNULL_BEGIN

#pragma mark - AdControllerWrapperProtocol definition

/**
 * Ad controller tag type.
 * @note Values must be unique.
 */
typedef NSString * AdControllerTag NS_STRING_ENUM;

/**
 * AdPresentationStatus used by implementors of `AdControllerWrapperProtocol`.
 * Represents the status of the ad being presented.
 */
typedef NS_ENUM(NSInteger, AdPresentation) {
    /*! @const AdPresentationWillAppear Ad view controller will appear. This is not a terminal state. */
    AdPresentationWillAppear = 1,
    /*! @const AdPresentationDidAppear Ad view controller did appear. This is not a terminal state. */
    AdPresentationDidAppear,
    /*! @const AdPresentationWillDisappear Ad view controller will disappear. This is not a terminal state. */
    AdPresentationWillDisappear,
    /*! @const AdPresentationDidDisappear Ad view controller did disappear. This is a terminal state. */
    AdPresentationDidDisappear,

    // Ad presentation error states:
    /*! @const AdPresentationErrorInappropriateState The app is not in the appropriate state to present t
     * a particular ad. This is a terminal state.*/
    AdPresentationErrorInappropriateState,
    /*! @const AdPresentationErrorNoAdsLoaded No ads are loaded. This is a terminal state. */
    AdPresentationErrorNoAdsLoaded,
    /*! @const AdPresentationErrorFailedToPlay Rewarded video ad failed to play. This is a terminal state. */
    AdPresentationErrorFailedToPlay,
    /*! @const AdPresentationErrorCustomDataNotSet Rewarded video ad custom data not set. This is a terminal state. */
    AdPresentationErrorCustomDataNotSet
};

FOUNDATION_EXPORT NSErrorDomain const AdControllerWrapperErrorDomain;

/**
 * AdControllerWrapperErrorCode are terminating error emissions from signal returned by ad controller `-loadAd` method.
 */
typedef NS_ERROR_ENUM(AdControllerWrapperErrorDomain, AdControllerWrapperErrorCode) {
    /*! @const AdControllerWrapperErrorAdExpired Ad controller's pre-fetched ad has expired. Once emitted by `-loadAd`,
     * AdManager will load a new ad. */
    AdControllerWrapperErrorAdExpired = 1000,
    /*! @const AdControllerWrapperErrorAdFailedToLoad Ad controller failed to load ad. Once emitted by `-loadAd`,
     * AdManager will load a new ad `AD_LOAD_RETRY_COUNT` times. */
    AdControllerWrapperErrorAdFailedToLoad,
};

/**
 * AdControllerWrapperProtocol is the protocol used by AdManager to interface with different Ad SDKs or types.
 * A wrapper class implementing this protocol should be created for each Ad type or SDK.
 */
@protocol AdControllerWrapperProtocol

@required

@property (nonatomic, readonly) AdControllerTag tag;

// Should be TRUE if ad is ready to be displayed, FALSE otherwise.
// The value should not change while the ad is being presented, and should only be set to FALSE after
// the ad has been dismissed.
// To avoid unnecessary computation for observers of this property, implementations of this protocol
// should check the current value before setting it.
@property (nonatomic, readonly) BOOL ready;

// adPresented is hot infinite signal - emits RACUnit whenever an ad is shown.
@property (nonatomic, readonly) RACSubject<RACUnit *> *adPresented;

// presentationStatus is hot infinite signal - emits items of type @(AdPresentation).
@property (nonatomic, readonly) RACSubject<NSNumber *> *presentationStatus;

// Loads ad if none is already loaded. Property `ready` should be TRUE after ad has been loaded (whether or not it
// has already been pre-fetched by the SDK).
// Implementations should handle multiple subscriptions to the returned signal without
// side-effects (even if the ad has already been loaded).
// Returned signal is expected to terminate with an error when an ad expires or fails to load, with the appropriate
// `AdControllerWrapperErrorCode` error code.
- (RACSignal<AdControllerTag> *)loadAd;

// Unloads ad if one is loaded. `ready` should be FALSE after the unloading is done.
// Implementations should emit the wrapper's tag after ad is unloaded and then complete.
- (RACSignal<AdControllerTag> *)unloadAd;

@optional

// For classes that conform to this protocol but don't implement this method, they should implement a similar
// method, but the same expected behaviour of the returned signal.
// Implementations should emit items of type @(AdPresentation), and complete when the ad has been dismissed.
// If there are no ads loaded, returned signal emits @(AdPresentationErrorNoAdsLoaded) and then completes.
- (RACSignal<NSNumber *> *)presentAdFromViewController:(UIViewController *)viewController;

@end


#pragma mark - AdManager class

// List of all AdControllerTag objects.
FOUNDATION_EXPORT AdControllerTag const AdControllerTagUntunneledInterstitial;
FOUNDATION_EXPORT AdControllerTag const AdControllerTagUntunneledRewardedVideo;
FOUNDATION_EXPORT AdControllerTag const AdControllerTagTunneledRewardedVideo;

@interface AdManager : NSObject

/**
 * Infinite signal that emits @(TRUE) if an is currently being displayed, @(FALSE) otherwise.
 * The subject may emit duplicate state.
 */
@property (nonatomic, readonly) RACReplaySubject<NSNumber *> *adIsShowing;

/**
 * Emits @(TRUE) when the untunneled interstitial is ready to be presented.
 * Emits @(FALSE) when app conditions are such that the ad cannot be presented, this is regardless of whether the
 * ad has been loaded.
 * Subject initially has default value @(FALSE).
 * @scheduler Events are delivered on the main thread.
 */
@property (nonatomic, readonly) RACReplaySubject<NSNumber *> *untunneledInterstitialCanPresent;

/**
 * Emits @(TRUE) when tunneled or untunneled rewarded video is ready to be presented.
 * Emits @(FALSE) when app conditions are such that the ad cannot be presented, this is regardless of whether the
 * ad has been loaded.
 * Subject initially has default value @(FALSE).
 * @scheduler Events are delivered on the main thread.
 */
@property (nonatomic, readonly) RACReplaySubject<NSNumber *> *rewardedVideoCanPresent;

+ (instancetype)sharedInstance;

/**
 * Initializes the Ads SDK.
 * This should be called during the apps difFinishLaunchingWithOptions: delegate callback.
 */
- (void)initializeAdManager;

/**
 * Presents untunneled interstitial if app is in the appropriate state, and an interstitial ad has already been loaded.
 *
 * If ad cannot be presented due to inappropriate app state, returned signal completes immediately.
 *
 * If the app state is appropriate for displaying an ad, but there's an underlying error,
 * one of the errors states of @(AdPresentation) will be emitted (enums starting with AdPresentationError_)
 * and then the signal will complete.
 *
 * If the add is ready to be presented, the signal will start by emitting the following states in order:
 *  AdPresentationWillAppear -> AdPresentationDidAppear -> AdPresentationWillDisappear -> AdPresentationDidDisappear
 * after which the signal will complete.
 */
- (RACSignal<NSNumber *> *)presentInterstitialOnViewController:(UIViewController *)viewController;

/**
 * Presents tunneled or untunneled rewarded video ad if app is in the appropriate state and the rewarded video
 * ad has been loaded.
 *
 * If ad cannot be presented due to inappropriate app state, returned signal completes immediately.
 *
 * If the app state is appropriate for displaying an ad, but there's an underlying error,
 * one of the errors states of @(AdPresentation) will be emitted (enums starting with AdPresentationError_)
 * and then the signal will complete.
 *
 * If the add is ready to be presented, the signal will start by emitting the following states in order:
 *  AdPresentationWillAppear -> AdPresentationDidAppear -> AdPresentationWillDisappear -> AdPresentationDidDisappear
 * after which the signal will complete.
 *
 * @param viewController View controller to display ad on top of.
 * @param customData Optional custom data to include in the ad service server-to-server callback.
 */
- (RACSignal<NSNumber *> *)presentRewardedVideoOnViewController:(UIViewController *)viewController
                                                 withCustomData:(NSString *_Nullable)customData;

@end

NS_ASSUME_NONNULL_END
