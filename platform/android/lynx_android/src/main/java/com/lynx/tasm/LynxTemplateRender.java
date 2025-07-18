// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static android.view.View.MeasureSpec;

import android.app.Activity;
import android.content.Context;
import android.graphics.Canvas;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.SparseArray;
import android.view.InputEvent;
import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.AnyThread;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.annotation.StringDef;
import com.lynx.BuildConfig;
import com.lynx.devtoolwrapper.LynxDevtool;
import com.lynx.devtoolwrapper.LynxDevtoolGlobalHelper;
import com.lynx.jsbridge.JSModule;
import com.lynx.jsbridge.LynxAccessibilityModule;
import com.lynx.jsbridge.LynxExposureModule;
import com.lynx.jsbridge.LynxExtensionModule;
import com.lynx.jsbridge.LynxFetchModule;
import com.lynx.jsbridge.LynxIntersectionObserverModule;
import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.jsbridge.LynxResourceModule;
import com.lynx.jsbridge.LynxSetModule;
import com.lynx.jsbridge.LynxTextInfoModule;
import com.lynx.jsbridge.LynxUIMethodModule;
import com.lynx.jsbridge.RuntimeLifecycleListener;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.CleanupReference;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.LynxPageLoadListener;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import com.lynx.tasm.behavior.ImageInterceptor;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxIntersectionObserverManager;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.herotransition.HeroTransitionManager;
import com.lynx.tasm.behavior.shadow.ChoreographerLayoutTick;
import com.lynx.tasm.behavior.shadow.LayoutTick;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.ViewLayoutTick;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.core.JSProxy;
import com.lynx.tasm.core.LynxEngineProxy;
import com.lynx.tasm.core.LynxLayoutProxy;
import com.lynx.tasm.core.resource.LynxResourceLoader;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.PerformanceController;
import com.lynx.tasm.performance.TimingOption;
import com.lynx.tasm.performance.longtasktiming.LynxLongTaskMonitor;
import com.lynx.tasm.performance.timing.TimingConstants;
import com.lynx.tasm.provider.*;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.resourceprovider.template.TemplateProviderResult;
import com.lynx.tasm.service.ILynxExtensionService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.service.security.ILynxSecurityService;
import com.lynx.tasm.service.security.SecurityResult;
import com.lynx.tasm.theme.LynxTheme;
import com.lynx.tasm.utils.CallStackUtil;
import com.lynx.tasm.utils.ContextUtils;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.LynxViewBuilderProperty;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.Runnable;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import org.json.JSONObject;

// TODO(heshan): merge the init logics.
public class LynxTemplateRender implements ILynxEngine, ILynxErrorReceiver {
  private static final String TAG = "LynxTemplateRender";

  private final TemplateAssembler mTemplateAssembler = new TemplateAssembler();

  private InnerPageLoadListener mPageLoadListener;
  private ViewLayoutTick mViewLayoutTick;
  private int mPreWidthMeasureSpec;
  private int mPreHeightMeasureSpec;

  private LynxContext mLynxContext;
  private LynxBackgroundRuntime mRuntime;

  private Map<String, String> mOriginLynxViewConfig;

  private String mUrl;
  private volatile boolean reload = false;
  private LynxSSRHelper mSSRHelper;
  private boolean mHasDestroy = false;

  private boolean mHasPageStart;

  private ThreadStrategyForRendering mThreadStrategyForRendering;
  private LynxGenericInfo mGenericInfo;
  private final LynxViewClientGroup mClient = new LynxViewClientGroup();
  private final LynxViewClientGroupV2 mClientV2 = new LynxViewClientGroupV2();
  private LynxViewBuilder mLynxViewBuilder;
  private LynxBackgroundRuntimeOptions mLynxRuntimeOptions;
  protected LynxModuleFactory mModuleFactory;
  private LynxIntersectionObserverManager mIntersectionObserverManager;
  private boolean mHasEnvPrepared;
  private boolean mWillContentSizeChange;
  @Nullable protected UIBodyView mBodyView;
  private boolean mAsyncRender;
  private LynxTheme mTheme;
  private TemplateData globalProps;
  private LynxResourceLoader mResourceLoader;
  private long mFirstMeasureTime = -1;
  private List<TemplateData> updatedDataList = new CopyOnWriteArrayList<>();

  private Context mContext;
  @Keep private LynxDevtool mDevTool;

  private long mInitStart;
  private long mInitEnd;

  protected boolean mEnableBytecode = false;
  protected String mBytecodeSourceUrl;
  private boolean mEnablePendingJsTask = false;
  private boolean mEnableGenericResourceFetcher = false;

  // Event lynx_open_page
  private static final String EVENT_NAME_LYNX_OPEN_PAGE = "lynxsdk_open_page";

  // Relayout should be triggered after screen size is changed,
  private boolean mShouldUpdateViewport = true;
  // for fontscale
  private float mFontScale = 1.0f;
  private boolean mAutoConcurrency;
  private LynxBooleanOption mLongTaskMonitorEnabled = LynxBooleanOption.UNSET;
  private boolean mEnableUIFlush = true;

  private List<Map<String, Object>> componentsData;
  private boolean mVsyncAlignedFlushEnabled = true;
  private static final boolean VSYNC_ALIGNED_FLUSH_EXP_SWITCH =
      LynxEnv.getBooleanFromExternalEnv(LynxEnvKey.VSYNC_ALIGNED_FLUSH_EXP_KEY, false);

  private ILynxUIRenderer mLynxUIRender;

  private LynxInfoReportHelper mReportHelper = new LynxInfoReportHelper();

  private PerformanceController mPerformanceController = new PerformanceController();

  // LynxView render phase consts
  @Retention(RetentionPolicy.SOURCE)
  @StringDef({RENDER_PHASE_SETUP, RENDER_PHASE_UPDATE})
  public @interface RenderPhaseName {}
  public static final String RENDER_PHASE_SETUP = "setup";
  public static final String RENDER_PHASE_UPDATE = "update";

  // Render phase of current LynxView with available value setup/update
  @NonNull private volatile String mRenderPhase = RENDER_PHASE_SETUP;

  private AbsTemplateProvider mTemplateProvider;

  private boolean mEnableSyncFlush;

  private boolean mForceLayoutOnBackgroundThread =
      LynxEnv.inst().shouldForceLayoutOnBackgroundThread();

  private boolean mEnableJSRuntime;

  private boolean mEnableAirStrictMode;

  private JSProxy mJSProxy;
  private LynxGroup mGroup;
  private LynxEngineProxy mEngineProxy;

  private LynxLayoutProxy mLayoutProxy;

  private Map<Double, PlatformCallBack> platformCallBackMap = new HashMap<>();

  private AtomicBoolean mIsDestroyed = new AtomicBoolean(true);
  private long mNativeLifecycle;
  // Destory mNativeLifecycle when no reference to LynxTemplateRender.
  private CleanupReference mCleanupReference = null;

  private NativeFacade mNativeFacade;
  private long mNativePtr = 0;

  @Nullable private LynxResourceLoader mLoader;

  private AtomicInteger mLynxGetDataCounter = new AtomicInteger(0);
  private SparseArray<LynxGetDataCallback> mCallbackSparseArray = new SparseArray<>();

  private boolean mEnableReuseEngine;
  private boolean mIsEngineFromReuse;
  private boolean mAutoFallbackEngineAfterDraw = false;
  private TemplateData mTemplateData;
  @Nullable private LynxEngine mLynxEngineRef;

  @Keep
  public LynxTemplateRender(Context context, UIBodyView bodyView, LynxViewBuilder builder) {
    init(context, bodyView, builder);
  }
  // async render

  LynxTemplateRender(Context context, LynxViewBuilder builder) {
    init(context, null, builder);
  }
  // async render

  LynxTemplateRender(LynxViewBuilder builder) {
    init(null, null, builder);
  }

  /**
   * Cast BodyView to LynxView because DevTool and SecurityService still need it.
   * TODO(zhoupeng.z): review this methde after decoupling LynxView from DevTool and SecurityService
   */
  private LynxView getLynxView() {
    return mBodyView instanceof LynxView ? (LynxView) mBodyView : null;
  }

  private String formatLynxMessage(String action) {
    // $timestamp create/renderTemplate/reset/reload/update/destory LynxView $sessionid
    // <$schema(optional for create lynxview without schema)>
    long timestamp = System.currentTimeMillis();
    StringBuilder builder = new StringBuilder();
    builder.append(String.valueOf(timestamp)).append(" ");
    builder.append(action).append(" LynxView ");
    if (mLynxContext != null && !TextUtils.isEmpty(mLynxContext.getLynxSessionID())) {
      builder.append(mLynxContext.getLynxSessionID()).append(" ");
    }
    if (mGenericInfo != null && mGenericInfo.getPropValueRelativePath() != null) {
      builder.append(mGenericInfo.getPropValueRelativePath());
    }
    return builder.toString();
  }

  private boolean checkEnableGenericResourceFetcher(LynxBooleanOption enable) {
    if (enable == LynxBooleanOption.UNSET) {
      return LynxEnv.inst().enableGenericResourceFetcher();
    }
    return enable == LynxBooleanOption.TRUE;
  }

  private void init(@Nullable Context context, UIBodyView bodyView, LynxViewBuilder builder) {
    TraceEvent.beginSection(TraceEventDef.TEMPLATE_RENDER_INIT);
    mInitStart = System.currentTimeMillis();
    mContext = context;
    mBodyView = bodyView;
    mLynxViewBuilder = builder;

    mEnableReuseEngine = EmbeddedMode.isEnginePoolEnable(mLynxViewBuilder.embeddedMode)
        && mLynxViewBuilder.templateBundle != null;
    if (mEnableReuseEngine) {
      reuseLynxEngine();
    } else {
      mLynxUIRender = lynxUIRenderer();
    }

    mRuntime = builder.lynxBackgroundRuntime;
    mTemplateProvider = builder.templateProvider;
    mEnableSyncFlush = builder.enableSyncFlush;
    mEnableJSRuntime = builder.enableJSRuntime;
    mEnableGenericResourceFetcher =
        checkEnableGenericResourceFetcher(builder.enableGenericResourceFetcher);
    mEnableAirStrictMode = builder.enableAirStrictMode;
    builder.lynxBackgroundRuntime = null;

    if (mBodyView != null) {
      mBodyView.setTimingCollector(mPerformanceController);
      mPerformanceController.setEnableController(
          !EmbeddedMode.isBaseModeEnable(mLynxViewBuilder.embeddedMode));
    }
    mGenericInfo = new LynxGenericInfo();
    mLynxRuntimeOptions = builder.lynxRuntimeOptions;
    mAutoConcurrency = builder.enableAutoConcurrency;

    // force using MULTI_THREADS by default for enableAutoConcurrency
    mThreadStrategyForRendering =
        mAutoConcurrency ? ThreadStrategyForRendering.MULTI_THREADS : builder.threadStrategy;

    mAsyncRender = (mThreadStrategyForRendering == ThreadStrategyForRendering.MULTI_THREADS
        || mThreadStrategyForRendering == ThreadStrategyForRendering.MOST_ON_TASM);

    // force disabling mForceLayoutOnBackgroundThread when mAutoConcurrency is enabled.
    if (mAutoConcurrency) {
      mForceLayoutOnBackgroundThread = false;
    }

    // Force enabling layout thread when mForceLayoutOnBackgroundThread is enabled.
    if (mForceLayoutOnBackgroundThread) {
      mThreadStrategyForRendering = mAsyncRender ? ThreadStrategyForRendering.MULTI_THREADS
                                                 : ThreadStrategyForRendering.PART_ON_LAYOUT;
    }

    mHasEnvPrepared = LynxEnv.inst().isNativeLibraryLoaded();
    mVsyncAlignedFlushEnabled = VSYNC_ALIGNED_FLUSH_EXP_SWITCH
        && LynxEnv.inst().getVsyncAlignedFlushGlobalSwitch()
        && isThreadStrategySupportVsyncAlignedFlush();
    mFontScale = builder.fontScale;
    mOriginLynxViewConfig = builder.lynxViewConfig;
    // To support modify `bytecode` settings for LynxView reuse,
    // these properties may differ from themself in LynxBackgroundRuntimeOptions,
    // thus store them as properties of LynxTemplateRender
    mEnableBytecode = mLynxRuntimeOptions.isEnableUserBytecode();
    mBytecodeSourceUrl = mLynxRuntimeOptions.getBytecodeSourceUrl();
    mEnablePendingJsTask = builder.enablePendingJsTask;

    // Caller may to set up virtual screen metrics, so we have to get screen_metrics with
    // lynxcontent.mVirtualScreenMetric(). The Global screen_metrics cannot be deleted for
    // compatibility, some caller still using its related interface.
    DisplayMetricsHolder.updateOrInitDisplayMetrics(context, builder.densityOverride);
    DisplayMetrics screenMetrics = DisplayMetricsHolder.getScreenDisplayMetrics();
    if (builder.screenWidth != DisplayMetricsHolder.UNDEFINE_SCREEN_SIZE_VALUE
        && builder.screenHeight != DisplayMetricsHolder.UNDEFINE_SCREEN_SIZE_VALUE) {
      screenMetrics.widthPixels = builder.screenWidth;
      screenMetrics.heightPixels = builder.screenHeight;
    }

    // Prepare for env
    mLynxContext =
        new LynxContext(context != null ? context : LynxEnv.inst().getAppContext(), screenMetrics) {
          @Override
          public void handleException(Exception e) {
            onExceptionOccurred(LynxSubErrorCode.E_EXCEPTION_PLATFORM, e, null);
          }

          @Override
          public void handleException(Exception e, JSONObject userDefinedInfo) {
            onExceptionOccurred(LynxSubErrorCode.E_EXCEPTION_PLATFORM, e, userDefinedInfo);
          }

          @Deprecated
          @RestrictTo(RestrictTo.Scope.LIBRARY)
          @Override
          public void handleException(Exception e, int errCode) {
            onExceptionOccurred(errCode, e, null);
          }

          @Deprecated
          @RestrictTo(RestrictTo.Scope.LIBRARY)
          @Override
          public void handleException(Exception e, int errCode, JSONObject userDefinedInfo) {
            onExceptionOccurred(errCode, e, userDefinedInfo);
          }

          @Override
          @RestrictTo(RestrictTo.Scope.LIBRARY)
          public void handleLynxError(LynxError error) {
            onErrorOccurred(error);
          }
        };
    mTemplateAssembler.setLynxContext(mLynxContext);

    mLynxContext.setEmbeddedMode(builder.embeddedMode);

    mLynxContext.setUIBodyView(mBodyView);
    mLynxContext.setForceDarkAllowed(builder.forceDarkAllowed);
    mLynxContext.setContextData(mLynxViewBuilder.getContextData());
    if (mLynxViewBuilder.getImageCustomParam() != null) {
      mLynxContext.setImageCustomParam(mLynxViewBuilder.getImageCustomParam());
    }
    mLynxContext.setEnableAutoConcurrency(mAutoConcurrency);
    // TODO(chenyouhui): Move this function call to a more appropriate place.
    ILynxExtensionService extensionService =
        LynxServiceCenter.inst().getService(ILynxExtensionService.class);
    if (extensionService != null) {
      extensionService.onLynxViewSetup(
          mLynxContext, mLynxRuntimeOptions.getLynxGroup(), mLynxViewBuilder.behaviorRegistry);
    }
    LynxEnv.inst().initNativeUIThread();
    init(context);

    // TODO: update viewport only if the builder has preset measure spec
    int widthMeasureSpec = builder.presetWidthMeasureSpec;
    int heightMeasureSpec = builder.presetHeightMeasureSpec;
    // for auto concurrency or layout on background thread,
    // if width measure spec and height measure spec have not been preset,
    // use screen width as width measure spec by default
    // in order to avoid relayout when onMeasure in the maximum extent.
    // TODO(heshan): consider use screen width by default to avoid relayout
    if ((mAutoConcurrency || mForceLayoutOnBackgroundThread) && widthMeasureSpec == 0
        && heightMeasureSpec == 0) {
      widthMeasureSpec = View.MeasureSpec.makeMeasureSpec(
          mLynxContext.getResources().getDisplayMetrics().widthPixels, View.MeasureSpec.EXACTLY);
    }

    if (!mEnableReuseEngine || !mIsEngineFromReuse) {
      // When the Engine is successfully reused, it is unnecessary to update the viewport during the
      // LynxView initialization process.
      updateViewport(widthMeasureSpec, heightMeasureSpec);
    }

    mClient.addClient(LynxEnv.inst().getLynxViewClient());
    mClient.addClient(new LogLynxViewClient());

    setupReusedEngineConfig();

    mInitEnd = System.currentTimeMillis();
    setMsTiming(TimingHandler.CREATE_LYNX_START, mInitStart, null);
    setMsTiming(TimingHandler.CREATE_LYNX_END, mInitEnd, null);
    LLog.i(TAG, formatLynxMessage("create"));
    TraceEvent.endSection(TraceEventDef.TEMPLATE_RENDER_INIT);
  }

  private void reuseLynxEngine() {
    // Try to get an engine from the cache pool.
    mLynxEngineRef =
        LynxEnginePool.getInstance().pollEngineFromPool(mLynxViewBuilder.templateBundle);

    // FIXME(huangweiwu): This is temporary code to ensure there is no runtime.
    mLynxViewBuilder.enablePendingJsTask = true;

    if (mLynxEngineRef == null) {
      // Failed to get from the pool, create a new one.
      // This wrapper won't be cached because it doesn't have the Element/UI Tree context yet.
      mLynxEngineRef = new LynxEngine(mLynxViewBuilder.templateBundle, this);
      mLynxUIRender = lynxUIRenderer();
      // Cache the platform-level reuse object.
      mLynxEngineRef.setLynxUIRenderer(mLynxUIRender);
      mIsEngineFromReuse = false;
    } else {
      // Successfully retrieved an engine from the pool.
      mLynxEngineRef.setLynxEngineState(LynxEngine.LynxEngineState.ON_REUSING);
      mLynxUIRender = mLynxEngineRef.getLynxUIRenderer();
      mLynxUIRender.attachBodyView(mBodyView, mLynxContext, mContext);
      mIsEngineFromReuse = true;
    }

    // Setup for reused or new engine wrapper.
    mLynxEngineRef.attachCurrentTemplateRender(this);
  }

  private void setupReusedEngineConfig() {
    if (!mEnableReuseEngine) {
      return;
    }
    if (mLynxEngineRef != null) {
      PageConfig.attachPageConfig(mLynxEngineRef.getPageConfig(), mLynxContext, mLynxUIRender);
    }
  }

  public boolean enableAirStrictMode() {
    return mEnableAirStrictMode;
  }

  void showErrorMessage(final LynxError error) {
    if (mDevTool != null) {
      mDevTool.showErrorMessage(error);
    }
  }

  public LynxContext getLynxContext() {
    return mLynxContext;
  }

  public PerformanceController getPerformanceController() {
    return mPerformanceController;
  }

  public UIGroup<UIBody.UIBodyView> getLynxRootUI() {
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    return (lynxUIRenderer != null) ? lynxUIRenderer.getLynxRootUI() : null;
  }

  public LynxDevtool getDevTool() {
    return mDevTool;
  }

  public void onEnterForeground() {
    onEnterForeground(true);
  }

  public void onEnterBackground() {
    onEnterBackground(true);
  }

  public void updateScreenMetrics(int width, int height) {
    LynxContext context = mLynxContext;
    if (mNativePtr == 0 || context == null) {
      return;
    }
    if (width != context.getScreenMetrics().widthPixels
        || height != context.getScreenMetrics().heightPixels) {
      mShouldUpdateViewport = true;
      context.updateScreenSize(width, height);
      nativeUpdateScreenMetrics(mNativePtr, mNativeLifecycle, width, height,
          context.getScreenMetrics().density, lynxUIRenderer().getUIDelegatePtr());
      if (mDevTool != null) {
        mDevTool.updateScreenMetrics(width, height, context.getScreenMetrics().density);
      }
    }
  }

  public void addLynxViewClient(LynxViewClient client) {
    if (client == null)
      return;
    mClient.addClient(client);
  }

  public void addLynxViewClientV2(LynxViewClientV2 client) {
    mClientV2.addClient(client);
  }

  public void removeLynxViewClient(LynxViewClient client) {
    if (client == null)
      return;
    if (mClient != null) {
      mClient.removeClient(client);
    }
  }

  public void removeLynxViewClientV2(LynxViewClientV2 client) {
    mClientV2.removeClient(client);
  }

  private void setMsTiming(final String key, final long msTimestamp, final String pipelineID) {
    mPerformanceController.setMsTiming(key, msTimestamp, pipelineID);
  }

  public void setExtraTiming(TimingHandler.ExtraTimingInfo extraTiming) {
    String eventName = "LynxTemplateRender.setExtraTiming";
    onTraceEventBegin(eventName);
    mPerformanceController.setExtraTiming(extraTiming);
    onTraceEventEnd(eventName);
  }

  public void setFluencyTracerEnabled(LynxBooleanOption enabled) {
    if (mLynxContext != null) {
      mLynxContext.getFluencyTraceHelper().setEnabledBySampling(enabled);
    }
  }

  public void setLongTaskMonitorEnabled(LynxBooleanOption enabled) {
    UIThreadUtils.assertOnUiThread();

    mLongTaskMonitorEnabled = enabled;
    if (mNativePtr != 0) {
      nativeSetLongTaskMonitorDisabled(
          mNativePtr, mNativeLifecycle, enabled == LynxBooleanOption.FALSE);
    }
  }
  public void putExtraParamsForReportingEvents(final Map<String, Object> params) {
    String eventName = "LynxTemplateRender.putExtraParamsForReportEvents";
    onTraceEventBegin(eventName);
    if (mLynxContext != null && mLynxContext.enableEventReporter()) {
      int instanceId = mLynxContext.getInstanceId();
      LynxEventReporter.putExtraParams(params, instanceId);
    }
    onTraceEventEnd(eventName);
  }

  public HashMap<String, Object> getAllTimingInfo() {
    if (mNativePtr != 0) {
      return nativeGetAllTimingInfo(mNativePtr, mNativeLifecycle);
    }
    return null;
  }

  /**
   * Get render phase of current LynxView
   * @return render phase of current LynxView
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  @AnyThread
  @RenderPhaseName
  public String getRenderPhase() {
    return mRenderPhase;
  }

  public void setImageInterceptor(ImageInterceptor interceptor) {
    if (mLynxContext != null) {
      mLynxContext.setImageInterceptor(interceptor);
    }
  }

  public void setAsyncImageInterceptor(ImageInterceptor interceptor) {
    if (mLynxContext != null) {
      mLynxContext.setAsyncImageInterceptor(interceptor);
    }
  }

  private synchronized void reloadAndInit() {
    if (mHasDestroy) {
      return;
    }
    if (reload) {
      onTraceEventBegin(TraceEventDef.TEMPLATE_RENDER_RELOAD_AND_INIT);
      mHasPageStart = false;
      mSSRHelper = null;
      mRenderPhase = RENDER_PHASE_SETUP;

      // When reload the page, we need send disexposure events before JSRuntime destroyed to ensure
      // the front end can receive events.
      if (mLynxContext != null) {
        mLynxContext.clearExposure();
      }

      // for async render, may reuse LynxView in async thread
      // need post removeAllViews to ui thread
      UIThreadUtils.runOnUiThreadImmediately(() -> {
        ILynxUIRenderer uiRenderer = lynxUIRenderer();
        if (uiRenderer != null) {
          uiRenderer.onReloadAndInitUIThreadPart();
        }
        if (mBodyView != null) {
          mBodyView.removeAllViews();
        }
      });

      if (globalProps != null) {
        globalProps = globalProps.deepClone();
      }
      int lastInstanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;
      if (mNativePtr != 0) {
        if (mLynxContext != null && mLynxContext.enableEventReporter()) {
          lastInstanceId = mLynxContext.getInstanceId();
          LynxEventReporter.removeGenericInfo(lastInstanceId);
        }
        destroyLynxEngine();
      }

      int tempPreWidthMeasureSpec = mPreWidthMeasureSpec;
      int tempPreHeightMeasureSpec = mPreHeightMeasureSpec;
      mPreWidthMeasureSpec = 0;
      mPreHeightMeasureSpec = 0;
      ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
      if (lynxUIRenderer != null) {
        lynxUIRenderer.onReloadAndInitAnyThreadPart();
      }
      if (mLynxContext != null) {
        mLynxContext.reset();
      }
      createLynxEngine(lastInstanceId);
      updateViewport(tempPreWidthMeasureSpec, tempPreHeightMeasureSpec);

      setMsTiming(TimingHandler.CREATE_LYNX_START, mInitStart, null);
      setMsTiming(TimingHandler.CREATE_LYNX_END, mInitEnd, null);
      onTraceEventEnd(TraceEventDef.TEMPLATE_RENDER_RELOAD_AND_INIT);
    } else {
      reload = true;
    }
  }

  /**
   * Get template's version. Should be called after loadTemplate, return empty string otherwise.
   */
  public String getPageVersion() {
    return mLynxContext == null ? "" : mLynxContext.getPageVersion();
  }

  public void pauseRootLayoutAnimation() {
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      lynxUIRenderer.pauseRootLayoutAnimation();
    }
  }
  public void resumeRootLayoutAnimation() {
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      lynxUIRenderer.resumeRootLayoutAnimation();
    }
  }

  private void createLynxEngine(final int lastInstanceId) {
    if (!checkIfEnvPrepared()) {
      return;
    }
    if (!mIsDestroyed.compareAndSet(true, false)) {
      return;
    }
    TraceEvent.beginSection(TraceEventDef.TEMPLATE_RENDER_CREATE_TASM);
    // recreate
    LayoutTick layoutTick;

    if (mThreadStrategyForRendering == ThreadStrategyForRendering.ALL_ON_UI) {
      mViewLayoutTick = new ViewLayoutTick(mBodyView);
      layoutTick = mViewLayoutTick;
    } else {
      layoutTick = new ChoreographerLayoutTick();
    }

    if (mEnableGenericResourceFetcher) {
      mLynxContext.setGenericResourceFetcher(
          mLynxViewBuilder.lynxRuntimeOptions.genericResourceFetcher);
      mLynxContext.setMediaResourceFetcher(
          mLynxViewBuilder.lynxRuntimeOptions.mediaResourceFetcher);
      mLynxContext.setTemplateResourceFetcher(
          mLynxViewBuilder.lynxRuntimeOptions.templateResourceFetcher);
    }

    mPageLoadListener = new InnerPageLoadListener();
    mGroup = mLynxRuntimeOptions.getLynxGroup();
    mLoader = new LynxResourceLoader(null, mLynxViewBuilder.fetcher, this,
        mLynxContext.getTemplateResourceFetcher(), mLynxContext.getGenericResourceFetcher());
    mLynxContext.setEnableAutoExpose(mLynxViewBuilder.enableAutoExpose);
    mNativeFacade = new NativeFacade(mLynxViewBuilder.enableJSRuntime());
    DisplayMetrics screenMetrics = mLynxContext.getScreenMetrics();
    long runtimeWrapperPtr = (mRuntime == null) ? 0 : mRuntime.getNativePtr();
    long whiteBoardPtr = (mGroup == null) ? 0 : mGroup.getWhiteBoardPtr();

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    lynxUIRenderer.onCreateTemplateRenderer(mLynxContext, mPageLoadListener,
        mThreadStrategyForRendering, mLynxViewBuilder.behaviorRegistry, layoutTick);

    boolean enableVSyncAligned = mLynxViewBuilder.enableVSyncAlignedMessageLoop
        || LynxEnv.inst().enableVSyncAlignedMessageLoopGlobal();
    mNativePtr = nativeCreate(runtimeWrapperPtr, mNativeFacade,
        mPerformanceController.isEnableController() ? mPerformanceController : null, mLoader,
        mThreadStrategyForRendering.id(), mLynxViewBuilder.enableLayoutSafepoint,
        mLynxViewBuilder.enableLayoutOnly, screenMetrics.widthPixels, screenMetrics.heightPixels,
        screenMetrics.density, LynxEnv.inst().getLocale(), mLynxViewBuilder.enableJSRuntime(),
        mLynxViewBuilder.enableMultiAsyncThread, mLynxViewBuilder.enablePreUpdateData,
        mAutoConcurrency, enableVSyncAligned, mLynxViewBuilder.enableAsyncHydration,
        mGroup != null && mGroup.enableJSGroupThread(), getJSGroupThreadNameIfNeed(),
        new TasmPlatformInvoker(mNativeFacade), whiteBoardPtr, lynxUIRenderer.getUIDelegatePtr(),
        lynxUIRenderer.useInvokeUIMethod(), mLongTaskMonitorEnabled == LynxBooleanOption.FALSE,
        mForceLayoutOnBackgroundThread, mLynxViewBuilder.enableUnifiedPipeline,
        mLynxViewBuilder.embeddedMode, mLynxEngineRef == null ? 0 : mLynxEngineRef.getNativePtr());
    lynxUIRenderer.attachNativeFacade(mNativeFacade);
    mNativeLifecycle = nativeLifecycleCreate();
    mCleanupReference = new CleanupReference(this, new CleanupOnUiThread(mNativeLifecycle), true);
    mLynxContext.setListNodeInfoFetcher(new ListNodeInfoFetcher(this));
    mLynxContext.setEnableVSyncAligned(enableVSyncAligned);

    if (mDevTool != null) {
      mDevTool.onTemplateAssemblerCreated(mNativePtr);
    }

    LynxProviderRegistry providerRegistry = new LynxProviderRegistry();
    for (Map.Entry<String, LynxResourceProvider> global :
        LynxEnv.inst().getResourceProvider().entrySet()) {
      providerRegistry.addLynxResourceProvider(global.getKey(), global.getValue());
    }
    for (Map.Entry<String, LynxResourceProvider> local :
        mLynxRuntimeOptions.getAllResourceProviders()) {
      providerRegistry.addLynxResourceProvider(local.getKey(), local.getValue());
    }
    mLynxContext.setProviderRegistry(providerRegistry);

    mLynxContext.setFontLoader(mLynxViewBuilder.fontLoader);
    mLynxContext.setImageFetcher(mLynxViewBuilder.imageFetcher);
    mLynxContext.setLynxExtraData(mLynxViewBuilder.lynxModuleExtraData);

    mNativeFacade.setLynxContext(mLynxContext);
    int instanceId = nativeGetInstanceId(mNativePtr, mNativeLifecycle);
    if (instanceId >= 0) {
      mLynxContext.setInstanceId(instanceId);
    }

    if (mBodyView != null) {
      mBodyView.setInstanceId(mLynxContext.getInstanceId());
      mClient.setInstanceId(mLynxContext.getInstanceId());
      mClientV2.setInstanceId(mLynxContext.getInstanceId());
    }

    if (mLynxContext != null && mLynxContext.enableEventReporter()) {
      LynxEventReporter.updateGenericInfo(LynxEventReporter.PROP_NAME_THREAD_MODE,
          mThreadStrategyForRendering.id(), mLynxContext.getInstanceId());
      LynxEventReporter.moveExtraParams(lastInstanceId, mLynxContext.getInstanceId());
    }

    if (!"none".equals(BuildConfig.JS_ENGINE_TYPE)
        && (null != mLynxContext && !mLynxContext.isEmbeddedModeOn())) {
      if (mRuntime != null) {
        mModuleFactory = mRuntime.getModuleFactory();
        mModuleFactory.setContext(mLynxContext);
      } else {
        mModuleFactory = new LynxModuleFactory(mLynxContext);
      }
      mModuleFactory.setLynxModuleExtraData(mLynxViewBuilder.lynxModuleExtraData);
      mModuleFactory.addModuleParamWrapper(mLynxRuntimeOptions.getWrappers());
      mModuleFactory.registerModule(
          LynxIntersectionObserverModule.NAME, LynxIntersectionObserverModule.class, null);
      mModuleFactory.registerModule(LynxUIMethodModule.NAME, LynxUIMethodModule.class, null);
      mModuleFactory.registerModule(LynxTextInfoModule.NAME, LynxTextInfoModule.class, null);
      mModuleFactory.registerModule(
          LynxAccessibilityModule.NAME, LynxAccessibilityModule.class, null);
      mModuleFactory.registerModule(LynxSetModule.NAME, LynxSetModule.class, null);
      mModuleFactory.registerModule(LynxResourceModule.NAME, LynxResourceModule.class, null);
      mModuleFactory.registerModule(LynxExposureModule.NAME, LynxExposureModule.class, null);
      mModuleFactory.registerModule(LynxFetchModule.NAME, LynxFetchModule.class, null);

      mResourceLoader = new LynxResourceLoader(mLynxRuntimeOptions, mLynxViewBuilder.fetcher, this,
          mLynxContext.getTemplateResourceFetcher(), mLynxContext.getGenericResourceFetcher());
      if (mRuntime != null) {
        // In LynxbakcgroundRuntime Standalone, we create and init a LynxRuntime without LynxShell.
        // During LynxTemplateRender creation, this runtime is used to create LynxShell.
        // Here we don't need to init the LynxRuntime which is already inited, but we
        // we need to bind it to LynxShell.
        attachPiper(mRuntime, mModuleFactory);

        // Destruction of Runtime inside wrapper will be handled by LynxShell. Since after
        // attachement, user cannot use LynxBackgroundRuntime, we can safely release its reference.
        mRuntime = null;

      } else {
        initPiper(mModuleFactory, mResourceLoader, mLynxRuntimeOptions.useQuickJSEngine(), false,
            mEnableBytecode, mBytecodeSourceUrl, mEnablePendingJsTask, lynxUIRenderer);
      }
      // extension dependent on piper, should init after piper init.
      setUpExtensionModules();
      if (mDevTool != null) {
        mDevTool.onRegisterModule(mModuleFactory);
      }
      mLynxContext.setEventEmitter(new LynxEventEmitter(mEngineProxy));
      mLynxContext.setJSProxy(mJSProxy);
    }
    mIntersectionObserverManager = new LynxIntersectionObserverManager(mLynxContext, mJSProxy);
    mLynxContext.setIntersectionObserverManager(mIntersectionObserverManager);
    EventEmitter eventEmitter = mLynxContext.getEventEmitter();
    if (eventEmitter != null) {
      eventEmitter.addObserver(mIntersectionObserverManager);
      eventEmitter.registerEventReporter(mNativeFacade);
    }

    setThemeInternal(mTheme);

    updateGlobalPropsInternal(globalProps);

    if (mFontScale != 1.0f) {
      nativeSetFontScale(mNativePtr, mNativeLifecycle, mFontScale);
    }

    TraceEvent.endSection(TraceEventDef.TEMPLATE_RENDER_CREATE_TASM);
  }

  private void setUpExtensionModules() {
    if (mBodyView == null || !mLynxViewBuilder.enableJSRuntime()) {
      return;
    }
    Map<String, LynxExtensionModule> modules = mLynxContext.getExtensionModules();
    for (String key : modules.keySet()) {
      LynxExtensionModule module = modules.get(key);
      long delegatePtr = module.getExtensionDelegatePtr();
      if (delegatePtr == 0) {
        LLog.e(TAG, "Fail to get extension delegate");
        continue;
      }
      nativeSetExtensionDelegate(mNativePtr, mNativeLifecycle, delegatePtr);
      module.setUp();
    }
  }

  private void init(Context context) {
    TraceEvent.beginSection(TraceEventDef.TEMPLATE_RENDER_INIT_WITH_CONTEXT);
    reload = false;
    mHasPageStart = false;
    mHasDestroy = false;
    mLynxContext.setImageInterceptor(mClient); // for compatible with the old ImageInterceptor
    // TODO(zhoupeng.z): Currently the LynxViewClient in LynxContext is only used to report errors,
    // make it compatible with LynxViewClientV2
    mLynxContext.setLynxViewClient(mClient);

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    lynxUIRenderer.onInitLynxTemplateRender(
        mLynxContext, mLynxViewBuilder.behaviorRegistry, mBodyView, mLongTaskMonitorEnabled);
    if (LynxEnv.inst().isLynxDebugEnabled()) {
      // TODO(zhoupeng.z): DevTool should accept UIBodyView
      if (mRuntime != null) {
        mDevTool = mRuntime.getDevtool();
        mDevTool.attach(getLynxView(), this);
      } else {
        mDevTool = new LynxDevtool(getLynxView(), this, mLynxViewBuilder.debuggable);
      }
      mDevTool.attachLynxUIOwner(lynxUIRenderer.lynxUIOwner());
      WeakReference<LynxTemplateRender> mWeakRef = new WeakReference<>(this);
    }

    createLynxEngine(LynxEventReporter.INSTANCE_ID_UNKNOWN);

    if (context == null) {
      // Disable UIFlush before attaching lynxView
      lynxUIRenderer.setContextFree(true);
      setEnableUIFlush(false);
    }

    TraceEvent.endSection(TraceEventDef.TEMPLATE_RENDER_INIT_WITH_CONTEXT);
  }

  /**
   * Render template with url from remote through template provider.
   *
   * @param templateUrl
   * @param templateData
   */
  public void renderTemplateUrl(@NonNull String templateUrl, final TemplateData templateData) {
    renderTemplateUrlInternal(templateUrl, new InnerLoadedCallback(templateUrl, templateData));
  }

  public void renderTemplateUrl(@NonNull String templateUrl, final String jsonData) {
    renderTemplateUrlInternal(templateUrl, new InnerLoadedCallback(templateUrl, jsonData));
  }

  public void renderTemplateUrl(@NonNull String templateUrl, final Map<String, Object> data) {
    renderTemplateUrlInternal(templateUrl, new InnerLoadedCallback(templateUrl, data));
  }

  public void renderSSRUrl(@NonNull String ssrUrl, final Map<String, Object> injectedData) {
    renderSSRUrlInternal(
        ssrUrl, new InnerSSRLoadedCallback(ssrUrl, TemplateData.fromMap(injectedData)));
  }

  public void renderSSRUrl(@NonNull String ssrUrl, final TemplateData templateData) {
    renderSSRUrlInternal(ssrUrl, new InnerSSRLoadedCallback(ssrUrl, templateData));
  }

  // only for ssr hydrate.
  private void TryHydrateSSRPage() {
    if (mSSRHelper != null && mSSRHelper.isHydratePending()) {
      reload = false;
      mSSRHelper.onHydrateBegan();
    }
  }

  public void ssrHydrateUrl(@NonNull String hydrateUrl, final Map<String, Object> data) {
    TryHydrateSSRPage();
    renderTemplateUrl(hydrateUrl, data);
  }

  public void ssrHydrateUrl(@NonNull String hydrateUrl, final TemplateData data) {
    TryHydrateSSRPage();
    renderTemplateUrl(hydrateUrl, data);
  }

  public JSModule getJSModule(String module) {
    if (mLynxContext != null) {
      return mLynxContext.getJSModule(module);
    }
    return null;
  }

  public void sendGlobalEvent(String name, JavaOnlyArray params) {
    // When SSR hydrate status is pending、beginning or failed, a global event will be sent to SSR
    // runtime to be consumed. But this global event will also be cached so that when runtimeReady
    // it behaves as normal global event.
    JavaOnlyArray finalParams = params;
    if (mSSRHelper != null && mSSRHelper.shouldSendEventToSSR()) {
      // Send global event to SSR
      if (checkIfEnvPrepared() && mNativePtr != 0) {
        ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(params);
        nativeSendSsrGlobalEvent(
            mNativePtr, mNativeLifecycle, name, buffer, buffer == null ? 0 : buffer.position());
      }
      // process params
      finalParams = mSSRHelper.processEventParams(params);
    }

    if (checkIfEnvPrepared() && mLynxContext != null) {
      mLynxContext.sendGlobalEvent(name, finalParams);
    } else {
      LLog.e(TAG, "sendGlobalEvent error, can't get GlobalEventEmitter in " + this.toString());
    }
  }

  public void sendGlobalEventToLepus(String name, List<Object> params) {
    if (checkIfEnvPrepared() && mNativePtr != 0) {
      ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(params);
      nativeSendGlobalEventToLepus(
          mNativePtr, mNativeLifecycle, name, buffer, buffer == null ? 0 : buffer.position());
    } else {
      LLog.e(TAG, "sendGlobalEventToLepus error, Env not prepared in " + this.toString());
    }
  }

  public void triggerEventBus(String name, List<Object> params) {
    if (checkIfEnvPrepared() && mNativePtr != 0) {
      ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(params);
      nativeTriggerEventBus(
          mNativePtr, mNativeLifecycle, name, buffer, buffer == null ? 0 : buffer.position());
    } else {
      LLog.e(TAG, "triggerEventBus error, Env not prepared in " + this.toString());
    }
  }

  public void updateGlobalProps(TemplateData props) {
    LLog.d(TAG, "updateGlobalProps with url: " + getTemplateUrl());
    onTraceEventBegin(TraceEventDef.TEMPLATE_RENDER_SET_GLOBAL_PROPS);
    if (checkIfEnvPrepared() && (mNativePtr != 0) && props != null) {
      internalMergeGlobalPropsSafely(props);
      updateGlobalPropsInternal(globalProps);
    }
    onTraceEventEnd(TraceEventDef.TEMPLATE_RENDER_SET_GLOBAL_PROPS);
  }

  private void renderSSRUrlInternal(@NonNull String templateUrl, InnerSSRLoadedCallback callback) {
    if (!checkIfEnvPrepared()) {
      onErrorOccurred(LynxSubErrorCode.E_APP_BUNDLE_LOAD_ENV_NOT_READY,
          "LynxEnv has not been prepared successfully!");
      return;
    }
    onTraceEventBegin("LynxTemplateRender.renderSSRUrlInternal");
    String[] urls = processUrl(templateUrl);
    setUrl(urls[0]);
    updateGenericInfoURL(urls[0]);
    LLog.i(TAG, formatLynxMessage("renderTemplate"));
    LynxTemplateResourceFetcher templateFetcher = mLynxContext.getTemplateResourceFetcher();
    if (templateFetcher != null) {
      LynxResourceRequest request = new LynxResourceRequest(
          templateUrl, LynxResourceRequest.LynxResourceType.LynxResourceTypeTemplate);
      templateFetcher.fetchSSRData(request, new LynxResourceCallback<byte[]>() {
        @Override
        public void onResponse(LynxResourceResponse<byte[]> response) {
          switch (response.getState()) {
            case SUCCESS:
              callback.onSuccess(response.getData());
              break;
            case FAILED:
              callback.onFailed(response.getError().getMessage());
              break;
          }
        }
      });
    } else {
      legacyLoadTemplateWithProvider(templateUrl, callback);
    }
    onTraceEventEnd("LynxTemplateRender.renderSSRUrlInternal");
  }

  private void renderTemplateUrlInternal(
      @NonNull String templateUrl, InnerLoadedCallback callback) {
    if (!checkIfEnvPrepared()) {
      onErrorOccurred(LynxSubErrorCode.E_APP_BUNDLE_LOAD_ENV_NOT_READY,
          "LynxEnv has not been prepared successfully!");
      return;
    }
    onTraceEventBegin("LynxTemplateRender.renderTemplate");
    String[] urls = processUrl(templateUrl);
    setUrl(urls[0]);

    if (mDevTool != null) {
      TemplateData initialData =
          callback.metaData != null ? callback.metaData.initialData : callback.mTemplateData;
      mDevTool.onLoadFromURL(urls[0], urls[1], initialData, callback.mData, callback.mJsonData);
    }

    if (mLynxContext == null) {
      LLog.e(TAG, "renderTemplate error, can't get LynxContext in " + this.toString());
      return;
    }

    if (mLynxContext.getTemplateResourceFetcher() != null) {
      loadTemplateWithGenericResourceFetcher(templateUrl, callback);
    } else {
      legacyLoadTemplateWithProvider(templateUrl, callback);
    }
    LLog.i(TAG, formatLynxMessage("renderTemplate"));
    onTraceEventEnd("LynxTemplateRender.renderTemplate");
  }

  private void loadTemplateWithGenericResourceFetcher(
      @NonNull String templateUrl, InnerLoadedCallback callback) {
    LynxTemplateResourceFetcher templateFetcher = mLynxContext.getTemplateResourceFetcher();
    if (templateFetcher != null) {
      LynxResourceRequest request = new LynxResourceRequest(
          templateUrl, LynxResourceRequest.LynxResourceType.LynxResourceTypeTemplate);
      templateFetcher.fetchTemplate(request, new LynxResourceCallback<TemplateProviderResult>() {
        @Override
        public void onResponse(LynxResourceResponse<TemplateProviderResult> response) {
          switch (response.getState()) {
            case SUCCESS:
              TemplateBundle templateBundle = response.getData().getTemplateBundle();
              if (templateBundle != null && templateBundle.isValid()) {
                callback.onSuccess(templateBundle);
              } else {
                callback.onSuccess(response.getData().getTemplateBinary());
              }
              break;
            case FAILED:
              callback.onFailed(response.getError().getMessage());
              break;
            default:
              break;
          }
        }
      });
    }
  }

  private void legacyLoadTemplateWithProvider(
      @NonNull String templateUrl, AbsTemplateProvider.Callback callback) {
    if (TextUtils.isEmpty(templateUrl)) {
      throw new IllegalArgumentException(
          TAG + " template url is null or TemplateProvider is not init");
    }
    dispatchOnPageStart(mUrl);
    setMsTiming(TimingHandler.PREPARE_TEMPLATE_START, System.currentTimeMillis(), null);
    mTemplateProvider.loadTemplate(mUrl, callback);
  }

  private boolean checkIfEnvPrepared() {
    return mHasEnvPrepared;
  }

  /**
   * Render given template using a base scheme url.
   *
   * @param template
   * @param templateData
   * @param baseUrl
   */
  public void renderTemplateWithBaseUrl(
      byte[] template, TemplateData templateData, String baseUrl) {
    if (mDevTool != null) {
      mDevTool.onLoadFromLocalFile(template, templateData, baseUrl);
    }

    setUrl(baseUrl);
    renderTemplate(template, templateData);
    LLog.i(TAG, formatLynxMessage("renderTemplate"));
  }

  public void renderTemplateWithBaseUrl(byte[] template, Map<String, Object> data, String baseUrl) {
    TemplateData templateData = TemplateData.fromMap(data);
    templateData.markReadOnly();
    if (mDevTool != null) {
      mDevTool.onLoadFromLocalFile(template, templateData, baseUrl);
    }

    setUrl(baseUrl);
    renderTemplate(template, templateData);
    LLog.i(TAG, formatLynxMessage("renderTemplate"));
  }

  public void renderTemplateWithBaseUrl(byte[] template, String stringData, String baseUrl) {
    TemplateData templateData = TemplateData.fromString(stringData);
    templateData.markReadOnly();
    if (mDevTool != null) {
      mDevTool.onLoadFromLocalFile(template, templateData, baseUrl);
    }

    setUrl(baseUrl);
    renderTemplate(template, templateData);
    LLog.i(TAG, formatLynxMessage("renderTemplate"));
  }

  public void ssrHydrateWithBaseUrl(
      @NonNull byte[] template, final Map<String, Object> data, @NonNull String hydrateUrl) {
    TryHydrateSSRPage();
    renderTemplateWithBaseUrl(template, data, hydrateUrl);
  }

  public void ssrHydrateWithBaseUrl(
      @NonNull byte[] template, final TemplateData data, @NonNull String hydrateUrl) {
    TryHydrateSSRPage();
    renderTemplateWithBaseUrl(template, data, hydrateUrl);
  }

  private void setUrl(String url) {
    mUrl = url;
    LynxEnv.inst().setLastUrl(mUrl);
    if (mLynxContext != null) {
      mLynxContext.setTemplateUrl(mUrl);
    }
  }

  private void prepareLynxEngineIfNeeded() {
    if (!checkIfEnvPrepared()) {
      onErrorOccurred(LynxSubErrorCode.E_APP_BUNDLE_LOAD_ENV_NOT_READY,
          "LynxEnv has not been prepared successfully!");
      return;
    }

    mWillContentSizeChange = true;
    if (mNativePtr != 0) {
      nativeMarkDirty(mNativePtr, mNativeLifecycle);
    }
    // client will return perf
    reloadAndInit();

    // Update the url info to generic info after the shell is rebuilt, because the rebuilt shell
    // generates a new instance ID.
    updateGenericInfoURL(mUrl);
    if (mPerformanceController != null) {
      mPerformanceController.setPerformanceObserver(mClientV2);
    }
    if (mNativeFacade != null) {
      mNativeFacade.setTemplateLoadClient(mClient);
      dispatchOnPageStart(mUrl);
    }
  }

  private void updateGenericInfoURL(String url) {
    if (mLynxContext == null || !mLynxContext.enableEventReporter()) {
      return;
    }
    if (url != null) {
      int instanceId = mLynxContext.getInstanceId();
      if (mGenericInfo != null) {
        mGenericInfo.updateLynxUrl(mLynxContext, url);
        String relativePath = mGenericInfo.getPropValueRelativePath();
        if (relativePath != null) {
          // create hashmap only when both url and relativePath not null
          HashMap<String, Object> propMap = new HashMap<String, Object>();
          propMap.put(LynxEventReporter.PROP_NAME_URL, url);
          propMap.put(LynxEventReporter.PROP_NAME_RELATIVE_PATH, relativePath);
          LynxEventReporter.updateGenericInfo(propMap, instanceId);
          if (mReportHelper != null) {
            mReportHelper.reportLynxCrashContext(
                LynxInfoReportHelper.KEY_LAST_LYNX_URL, relativePath);
          }
        } else {
          LynxEventReporter.updateGenericInfo(LynxEventReporter.PROP_NAME_URL, url, instanceId);
        }
      }
    }
  }

  public void renderTemplate(final byte[] template, final Map<String, Object> initData) {
    if (mHasDestroy) {
      return;
    }
    if ((!mAsyncRender || reload) && !UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          renderTemplate(template, initData);
        }
      });
      return;
    }

    TimingOption timingOption = new TimingOption(TimingConstants.LOAD_BUNDLE);
    long currentTimeMillis = System.currentTimeMillis();
    timingOption.setTiming(TimingConstants.PIPELINE_START, currentTimeMillis);
    timingOption.setTiming(TimingConstants.LOAD_BUNDLE_START, currentTimeMillis);
    this.prepareLynxEngineIfNeeded();
    if (mNativePtr != 0) {
      loadTemplate(template, initData, getTemplateUrl(), new TASMCallback(), timingOption);
    }
  }

  public void renderTemplate(final byte[] template, final TemplateData templateData) {
    if (mHasDestroy) {
      return;
    }
    if ((!mAsyncRender || reload) && !UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          renderTemplate(template, templateData);
        }
      });
      return;
    }

    TimingOption timingOption = new TimingOption(TimingConstants.LOAD_BUNDLE);
    long currentTimeMillis = System.currentTimeMillis();
    timingOption.setTiming(TimingConstants.PIPELINE_START, currentTimeMillis);
    timingOption.setTiming(TimingConstants.LOAD_BUNDLE_START, currentTimeMillis);
    this.prepareLynxEngineIfNeeded();
    if (mNativePtr != 0) {
      loadTemplate(template, templateData, getTemplateUrl(), new TASMCallback(), timingOption);
    }
    postRenderOrUpdateData(templateData);
  }

  private void renderTemplate(
      final byte[] template, final String initData, TimingOption timingOption) {
    if (mHasDestroy) {
      return;
    }
    if ((!mAsyncRender || reload) && !UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          renderTemplate(template, initData, timingOption);
        }
      });
      return;
    }

    this.prepareLynxEngineIfNeeded();
    if (mNativePtr != 0) {
      loadTemplate(template, initData, getTemplateUrl(), new TASMCallback(), timingOption);
    }
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void renderTemplateBundle(
      final TemplateBundle bundle, final TemplateData templateData, String baseUrl) {
    if (mHasDestroy) {
      return;
    }
    if (mDevTool != null) {
      mDevTool.onLoadFromBundle(bundle, templateData, baseUrl);
    }

    if ((!mAsyncRender || reload) && !UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          renderTemplateBundle(bundle, templateData, baseUrl);
        }
      });
      return;
    }

    mTemplateData = templateData;
    if (mEnableReuseEngine && isEngineReuseHasLoaded()) {
      if (mLynxEngineRef.isRunOnCurrentTemplateRender(this)) {
        mLynxContext.getUIBody().attachUIBodyView(mBodyView);
        updateData(templateData);
        tryRegisterLynxEngineReused();
        return;
      } else {
        fallbackNewEngine(false);
      }
    }

    TimingOption timingOption = new TimingOption(TimingConstants.LOAD_BUNDLE);
    long currentTimeMillis = System.currentTimeMillis();
    timingOption.setTiming(TimingConstants.PIPELINE_START, currentTimeMillis);
    timingOption.setTiming(TimingConstants.LOAD_BUNDLE_START, currentTimeMillis);
    setUrl(baseUrl);
    this.prepareLynxEngineIfNeeded();
    LLog.i(TAG, formatLynxMessage("renderTemplate"));
    if (mNativePtr != 0) {
      loadTemplateBundle(
          bundle, baseUrl, templateData, false, false, new TASMCallback(), timingOption);
    }
    postRenderOrUpdateData(templateData);
  }

  /**
   * Load LynxTemplate with a wrapped load metaInfo data structure
   * Load metaInfo data structure contains a pre-decoded template.js/lynx initData
   *
   * @param metaData LynxLoadMeta data structure
   */
  @AnyThread
  public void loadTemplate(final LynxLoadMeta metaData) {
    if (mHasDestroy) {
      return;
    }
    String eventName = "LynxTemplateRender.loadTemplateWithMeta";
    onTraceEventBegin(eventName);
    if ((!mAsyncRender || reload) && !UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          loadTemplate(metaData);
        }
      });
      onTraceEventEnd(eventName);
      return;
    }

    if (metaData.getLoadMode() == LynxLoadMode.PRE_PAINTING) {
      mLynxContext.setInPreLoad(true);
    }

    TimingOption timingOption = new TimingOption(TimingConstants.LOAD_BUNDLE);
    long currentTimeMillis = System.currentTimeMillis();
    timingOption.setTiming(TimingConstants.PIPELINE_START, currentTimeMillis);
    timingOption.setTiming(TimingConstants.LOAD_BUNDLE_START, currentTimeMillis);
    setUrl(metaData.getUrl());
    renderWithLoadMeta(metaData, timingOption);
    LLog.i(TAG, formatLynxMessage("renderTemplate"));
    if (metaData.initialData != null) {
      postRenderOrUpdateData(metaData.initialData);
    }
    onTraceEventEnd(eventName);
  }

  // init lynxEngine with info in LynxLoadMeta
  private void initLynxEngineWithLoadMeta(LynxLoadMeta loadMeta) {
    if (mNativePtr != 0) {
      if (loadMeta.enableProcessLayout()) {
        setEnableUIFlush(false);
      }

      // Inject platform config
      Map<String, String> lynxViewConfig = loadMeta.lynxViewConfig;
      if (lynxViewConfig == null || lynxViewConfig.isEmpty()) {
        // Default platform config from LynxView Builder.
        lynxViewConfig = mOriginLynxViewConfig;
      }

      if (lynxViewConfig != null) {
        String platformConfig =
            lynxViewConfig.get(LynxViewBuilderProperty.PLATFORM_CONFIG.getKey());
        if (platformConfig != null) {
          nativeSetPlatformConfig(mNativePtr, mNativeLifecycle, platformConfig);
        }
      }

      // update GlobalProps
      if (loadMeta.globalProps != null) {
        this.updateGlobalProps(loadMeta.globalProps);
      }
    }
  }

  private void renderWithLoadMeta(final LynxLoadMeta metaData, TimingOption timingOption) {
    LynxLoadMode loadMode = metaData.loadMode;
    boolean isPrePainting =
        LynxLoadMode.PRE_PAINTING == loadMode || LynxLoadMode.PRE_PAINTING_DRAW == loadMode;
    if (metaData.isBundleValid()) {
      if (mDevTool != null) {
        mDevTool.onLoadFromBundle(metaData.bundle, metaData.initialData, metaData.url);
      }
      this.prepareLynxEngineIfNeeded();
      this.initLynxEngineWithLoadMeta(metaData);
      boolean enableDumpElementTree = metaData.enableDumpElementTree();
      LLog.i(TAG,
          "LoadMeta with bundle, pre-painting: " + isPrePainting
              + " ,pre-painting with draw:" + (LynxLoadMode.PRE_PAINTING_DRAW == loadMode)
              + " enableDumpElementTree: " + enableDumpElementTree);
      loadTemplateBundle(metaData.bundle, metaData.url, metaData.initialData, isPrePainting,
          enableDumpElementTree, new TASMCallback(), timingOption);
    } else if (metaData.isBinaryValid()) {
      if (mDevTool != null) {
        mDevTool.onLoadFromLocalFile(metaData.binaryData, metaData.initialData, metaData.url);
      }
      this.prepareLynxEngineIfNeeded();
      this.initLynxEngineWithLoadMeta(metaData);
      boolean enableRecycleTemplateBundle = metaData.enableRecycleTemplateBundle();
      LLog.i(TAG,
          "LoadMeta with binary, pre-painting: " + isPrePainting
              + " ,pre-painting with draw:" + (LynxLoadMode.PRE_PAINTING_DRAW == loadMode)
              + " enableRecycleTemplateBundle: " + enableRecycleTemplateBundle);
      loadTemplate(metaData.binaryData, metaData.initialData, metaData.url, isPrePainting,
          enableRecycleTemplateBundle, new TASMCallback(), timingOption);
    } else {
      LLog.i(TAG,
          "LoadMeta with url, pre-painting: " + isPrePainting + " ,pre-painting with draw:"
              + (LynxLoadMode.PRE_PAINTING_DRAW == loadMode) + " url: " + metaData.url);
      renderTemplateUrlInternal(metaData.url, new InnerLoadedCallback(metaData.url, metaData));
    }
  }

  public void renderSSR(final byte[] ssr, final String url, final TemplateData templateData) {
    if (mHasDestroy) {
      return;
    }
    if (mDevTool != null) {
      mDevTool.onLoadFromLocalFile(ssr, templateData, url);
    }
    if ((!mAsyncRender || reload) && !UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          renderSSR(ssr, url, templateData);
        }
      });
      return;
    }
    prepareForRenderSSR(ssr, url);
    if (mNativePtr != 0) {
      loadSSRData(ssr, templateData, new TASMCallback());
    }
    postRenderOrUpdateData(templateData);
  }

  public void renderSSR(final byte[] ssr, final String url, final Map<String, Object> data) {
    TemplateData templateData = TemplateData.fromMap(data);
    templateData.markReadOnly();
    renderSSR(ssr, url, templateData);
  }

  public void prepareForRenderSSR(final byte[] ssr, final String url) {
    setUrl(url);
    updateGenericInfoURL(url);
    LLog.i(TAG, formatLynxMessage("renderTemplate"));
    if (!checkIfEnvPrepared()) {
      onErrorOccurred(
          LynxSubErrorCode.E_SSR_LOAD_UNINITIALIZED, "LynxEnv has not been prepared successfully!");
      return;
    }

    mWillContentSizeChange = true;
    if (mNativePtr != 0) {
      nativeMarkDirty(mNativePtr, mNativeLifecycle);
    }

    reloadAndInit();
    // waiting for hydrate

    mSSRHelper = new LynxSSRHelper();
    mSSRHelper.onLoadSSRDataBegan(url);
    if (mLynxContext != null && mLynxContext.enableEventReporter()) {
      LynxEventReporter.updateGenericInfo(
          LynxEventReporter.PROP_NAME_ENABLE_SSR, true, mLynxContext.getInstanceId());
    }
    if (mNativePtr != 0) {
      nativeSetSSRTimingData(mNativePtr, mNativeLifecycle, url, ssr.length);
    }
  }

  @Nullable
  public String getTemplateUrl() {
    return mUrl == null ? "" : mUrl;
  }

  public void setTheme(LynxTheme theme) {
    if (theme == null) {
      return;
    }

    if (mTheme == null) {
      mTheme = theme;
    } else {
      mTheme.replaceWithTheme(theme);
    }

    if (!checkIfEnvPrepared() || (mNativePtr == 0)) {
      return;
    }

    setThemeInternal(theme);
  }

  public void setTheme(@NonNull ByteBuffer rawTheme) {
    if (!checkIfEnvPrepared() || mNativePtr == 0) {
      return;
    }

    nativeUpdateConfig(mNativePtr, mNativeLifecycle, rawTheme, rawTheme.position());
  }

  public LynxTheme getTheme() {
    return mTheme;
  }

  public void updateData(String json, String processorName) {
    TemplateData templateData = TemplateData.fromString(json);
    templateData.markState(processorName);
    templateData.markReadOnly();
    this.updateData(templateData);
  }

  public void updateMetaData(LynxUpdateMeta meta) {
    LynxContext context = mLynxContext;
    if (context == null) {
      return;
    }
    if (context.isInPreLoad()) {
      LLog.i(TAG, "updateData after pre load, url:" + mUrl);
      context.setInPreLoad(false);
    }

    TemplateData updatedGlobalProps = null;
    if (meta.getUpdatedGlobalProps() != null) {
      // globalProps in merged in platform.
      // if passing globalProps is nil means globalProps should not be updated, pass nil downwards.
      // if passing globalProps is not nil means globalProps should be updated, pass merged
      // globalProps downwards.
      internalMergeGlobalPropsSafely(meta.getUpdatedGlobalProps());
      updatedGlobalProps = globalProps;
    }
    if (mNativePtr != 0) {
      updateMetaDataInternal(meta.getUpdatedData(), updatedGlobalProps);
    }
  }

  /**
   * The business logic may call loadTemplate in the sub-thread, and the Merge and Recycle methods
   * of GlobalProps will be triggered in different threads, leading to thread-safe issues. The
   * design of TemplateData itself is not thread-safe, so we only protect the operations of internal
   * GlobalProps here.
   */
  private synchronized void internalMergeGlobalPropsSafely(TemplateData newGlobalProps) {
    if (globalProps == null) {
      globalProps = TemplateData.fromMap(new HashMap<String, Object>());
    }
    globalProps.updateWithTemplateData(newGlobalProps);
    if (mDevTool != null) {
      mDevTool.onGlobalPropsChanged(globalProps);
    }
  }

  /**
   * The business logic may call loadTemplate in the sub-thread, and the Merge and Recycle methods
   * of GlobalProps will be triggered in different threads, leading to thread-safe issues. The
   * design of TemplateData itself is not thread-safe, so we only protect the operations of internal
   * GlobalProps here.
   */
  private synchronized void recycleGlobalPropsSafely() {
    if (globalProps != null) {
      globalProps.recycle();
      globalProps = null;
    }
  }

  // called before updateData.
  private boolean prepareUpdateData(TemplateData data) {
    if (!checkIfEnvPrepared() || mNativePtr == 0) {
      return false;
    }

    if (data == null) {
      LLog.e(TAG, "updateData with null TemplateData");
      return false;
    }

    data.flush();
    if (data.getNativePtr() == 0) {
      LLog.e(TAG, "updateData with TemplateData after flush is nullptr");
      return false;
    }

    if (mDevTool != null) {
      mDevTool.onUpdate(data);
    }
    mWillContentSizeChange = true;
    if (mNativePtr != 0) {
      nativeMarkDirty(mNativePtr, mNativeLifecycle);
    }
    return true;
  }

  // called after renderTemplate or updateData.
  private void postRenderOrUpdateData(TemplateData data) {
    if (data != null) {
      // add to updatedList and recycle it manually at destroy period.
      updatedDataList.add(data);
    }
  }

  public void updateData(TemplateData data) {
    String eventName = "LynxTemplateRender.updateData";
    onTraceEventBegin(eventName);
    LynxContext context = mLynxContext;
    if (context == null) {
      return;
    }
    if (context.isInPreLoad()) {
      LLog.i(TAG, "updateData after pre load, url:" + mUrl);
      context.setInPreLoad(false);
    }
    if (prepareUpdateData(data)) {
      data.markConsumed();
      nativeUpdateDataByPreParsedData(mNativePtr, mNativeLifecycle, data.getNativePtr(),
          data.processorName(), data.isReadOnly(), data);
    }
    postRenderOrUpdateData(data);
    onTraceEventEnd(eventName);
  }

  public void resetData(TemplateData data) {
    String eventName = "LynxTemplateRender.resetData";
    onTraceEventBegin(eventName);
    if (prepareUpdateData(data)) {
      nativeResetDataByPreParsedData(mNativePtr, mNativeLifecycle, data.getNativePtr(),
          data.processorName(), data.isReadOnly(), data);
    }
    postRenderOrUpdateData(data);
    LLog.i(TAG, formatLynxMessage("reset"));
    onTraceEventEnd(eventName);
  }

  public void reloadTemplate(TemplateData data, TemplateData newGlobalProps) {
    LLog.d(TAG, "reloadTemplate with url: " + getTemplateUrl());
    String eventName = "LynxTemplateRender.reloadTemplate";
    onTraceEventBegin(eventName);

    TimingOption timingOption = new TimingOption(TimingConstants.RELOAD_BUNDLE_FROM_NATIVE);
    long currentTimeMillis = System.currentTimeMillis();
    timingOption.setTiming(TimingConstants.PIPELINE_START, currentTimeMillis);
    timingOption.setTiming(TimingConstants.RELOAD_BUNDLE_START, currentTimeMillis);
    if (prepareUpdateData(data)) {
      if (newGlobalProps != null) {
        globalProps = newGlobalProps;
      }
      LynxViewClientV2.LynxPipelineInfo pipelineInfo = new LynxViewClientV2.LynxPipelineInfo(mUrl);
      pipelineInfo.addPipelineOrigin(
          LynxViewClientV2.LynxPipelineInfo.LynxPipelineOrigin.LYNX_RELOAD);
      mClientV2.onPageStarted(getLynxView(), pipelineInfo);
      long propsNativePtr = 0;
      if (newGlobalProps != null) {
        newGlobalProps.flush();
        propsNativePtr = newGlobalProps.getNativePtr();
      }
      /**
       * Empty props should overwrite the original global props and null props should do nothing,
       * but these two kinds of props' nativePtr are both 0. So have to pass the object to let
       * native know what kind of props Java is passing
       */
      timingOption.setTiming(TimingConstants.FFI_START, System.currentTimeMillis());
      nativeReloadTemplate(mNativePtr, mNativeLifecycle, data.getNativePtr(), propsNativePtr,
          data.processorName(), data.isReadOnly(), newGlobalProps, data,
          timingOption.toJavaOnlyMap());
    }
    postRenderOrUpdateData(data);
    LLog.i(TAG, formatLynxMessage("reload"));
    onTraceEventEnd(eventName);
  }

  public void preloadLazyBundles(String[] urls) {
    nativePreloadLazyBundles(mNativePtr, mNativeLifecycle, urls);
  }

  public void getCurrentData(LynxGetDataCallback callback) {
    if (!checkIfEnvPrepared() || mNativePtr == 0) {
      callback.onFail("LynxView Not Initialized Yet");
      return;
    }

    int tag = mLynxGetDataCounter.incrementAndGet();
    mCallbackSparseArray.put(tag, callback);
    nativeGetDataAsync(mNativePtr, mNativeLifecycle, tag);
  }

  public Map<String, Object> getPageDataByKey(String[] keys) {
    if (!checkIfEnvPrepared() || mNativePtr == 0) {
      return null;
    }

    Object pageData = nativeGetPageDataByKey(mNativePtr, mNativeLifecycle, keys);
    Map<String, Object> result = new HashMap<>();
    if (pageData instanceof Map) {
      result.putAll((Map<String, Object>) pageData);
    }
    return result;
  }

  @Keep
  public void updateData(Map<String, Object> data, String processorName) {
    TemplateData templateData = TemplateData.fromMap(data);
    templateData.markState(processorName);
    templateData.markReadOnly();
    this.updateData(templateData);
    LLog.i(TAG, formatLynxMessage("update"));
  }

  public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    onTraceEventBegin(TraceEventDef.LYNX_TEMPLATE_RENDER_MEASURE);
    if (mLynxEngineRef != null && !mLynxEngineRef.isRunOnCurrentTemplateRender(this)) {
      fallbackNewEngine(false);
      renderTemplateBundle(mLynxViewBuilder.templateBundle, mTemplateData, mUrl);
    }
    boolean needLongTaskMonitor = false;
    if (mLynxContext != null) {
      needLongTaskMonitor = LynxLongTaskMonitor.willProcessTask(
          "LynxTemplateRender.Measure", mLynxContext.getInstanceId(), mLongTaskMonitorEnabled);
    }
    long startTime = 0;
    if (mFirstMeasureTime == -1) {
      startTime = System.currentTimeMillis();
    }

    if (mForceLayoutOnBackgroundThread && !mAsyncRender) {
      maybeSyncLayoutResultDuringLayoutOnBackgroundThread(widthMeasureSpec, heightMeasureSpec);
    } else {
      if (mEnableSyncFlush) {
        syncFlush();
      }

      updateViewport(widthMeasureSpec, heightMeasureSpec);
      // if not has first screen means first layout not finish
      // need pending for layout finish avoid first screen show nothing
      if (mThreadStrategyForRendering == ThreadStrategyForRendering.PART_ON_LAYOUT
          && mNativePtr != 0 && mWillContentSizeChange) {
        nativeSyncFetchLayoutResult(mNativePtr, mNativeLifecycle);
        mWillContentSizeChange = false;
      }

      // Trigger layout if needs to notify that android layout is starting
      if (mViewLayoutTick != null) {
        mViewLayoutTick.triggerLayout();
      }

      // TODO(heshan):merge syncFlush, syncFetchLayoutResult and flush as one method.
      if (getEnableVsyncAlignedFlush() || (mAutoConcurrency && mWillContentSizeChange)) {
        nativeFlush(mNativePtr, mNativeLifecycle);
        mWillContentSizeChange = false;
      }
    }

    if (mFirstMeasureTime == -1) {
      mFirstMeasureTime = System.currentTimeMillis() - startTime;
    }
    if (needLongTaskMonitor) {
      LynxLongTaskMonitor.didProcessTask();
    }

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      lynxUIRenderer.performInnerMeasure(widthMeasureSpec, heightMeasureSpec);
    }
    onTraceEventEnd(TraceEventDef.LYNX_TEMPLATE_RENDER_MEASURE);
  }

  private void maybeSyncLayoutResultDuringLayoutOnBackgroundThread(
      int widthMeasureSpec, int heightMeasureSpec) {
    if (!mWillContentSizeChange) {
      updateViewport(widthMeasureSpec, heightMeasureSpec);

      if (getEnableVsyncAlignedFlush()) {
        nativeFlush(mNativePtr, mNativeLifecycle);
      }
      return;
    }

    // If the viewport does not need to be updated,
    // retrieve the layout result from the background thread.
    if (mPreWidthMeasureSpec == widthMeasureSpec && mPreHeightMeasureSpec == heightMeasureSpec
        && !mShouldUpdateViewport) {
      // TODO(heshan): Optimize the logic of SyncFetchLayoutResult.
      nativeSyncFetchLayoutResult(mNativePtr, mNativeLifecycle);
      mWillContentSizeChange = false;
      return;
    }

    mShouldUpdateViewport = false;

    // If the viewport needs to be updated, it implies that a reflow is required.
    // In this case, update the viewport on the current thread and relayout.
    enforceRelayoutOnCurrentThreadWithUpdatedViewport(widthMeasureSpec, heightMeasureSpec);

    mWillContentSizeChange = false;
  }

  private void enforceRelayoutOnCurrentThreadWithUpdatedViewport(
      int widthMeasureSpec, int heightMeasureSpec) {
    // TODO(heshan): Merge with the similar logic of updateViewport
    int widthMode = MeasureMode.fromMeasureSpec(widthMeasureSpec);
    int width = MeasureSpec.getSize(widthMeasureSpec);
    int heightMode = MeasureMode.fromMeasureSpec(heightMeasureSpec);
    int height = MeasureSpec.getSize(heightMeasureSpec);

    nativeEnforceRelayoutOnCurrentThreadWithUpdatedViewport(
        mNativePtr, mNativeLifecycle, width, widthMode, height, heightMode);

    mPreWidthMeasureSpec = widthMeasureSpec;
    mPreHeightMeasureSpec = heightMeasureSpec;
  }

  public void onLayout(boolean changed, int left, int top, int right, int bottom) {
    onTraceEventBegin(TraceEventDef.LYNX_TEMPLATE_RENDER_LAYOUT);
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      lynxUIRenderer.onLayout(changed, left, top, right, bottom);
    }
    onTraceEventEnd(TraceEventDef.LYNX_TEMPLATE_RENDER_LAYOUT);
  }

  private void onTraceEventBegin(String eventName) {
    if (!TraceEvent.isTracingStarted()) {
      return;
    }
    HashMap map = new HashMap<String, String>();
    if (mLynxContext != null) {
      map.put(TraceEventDef.INSTANCE_ID, String.valueOf(mLynxContext.getInstanceId()));
    }
    TraceEvent.beginSection(TraceEvent.CATEGORY_VITALS, eventName, map);
  }

  private void onTraceEventEnd(String eventName) {
    if (!TraceEvent.isTracingStarted()) {
      return;
    }
    TraceEvent.endSection(TraceEvent.CATEGORY_VITALS, eventName);
  }

  private boolean getEnableVsyncAlignedFlush() {
    return mVsyncAlignedFlushEnabled && isVsyncAlignedFlushPageConfigEnabled();
  }

  private boolean isVsyncAlignedFlushPageConfigEnabled() {
    return mLynxContext != null && mLynxContext.getEnableVsyncAlignedFlush();
  }

  private boolean isThreadStrategySupportVsyncAlignedFlush() {
    return mThreadStrategyForRendering == ThreadStrategyForRendering.ALL_ON_UI
        || mThreadStrategyForRendering == ThreadStrategyForRendering.PART_ON_LAYOUT;
  }

  public void updateViewport(int widthMeasureSpec, int heightMeasureSpec) {
    if (!checkIfEnvPrepared() || mNativePtr == 0) {
      return;
    }

    if (mPreWidthMeasureSpec == widthMeasureSpec && mPreHeightMeasureSpec == heightMeasureSpec
        && !mShouldUpdateViewport) {
      return;
    }

    if (mShouldUpdateViewport) {
      mShouldUpdateViewport = false;
    }

    int widthMode = MeasureMode.fromMeasureSpec(widthMeasureSpec);
    int width = MeasureSpec.getSize(widthMeasureSpec);
    int heightMode = MeasureMode.fromMeasureSpec(heightMeasureSpec);
    int height = MeasureSpec.getSize(heightMeasureSpec);
    // TODO: sync and async mode / should post to thread if not work on ui thread
    nativeUpdateViewport(mNativePtr, mNativeLifecycle, width, widthMode, height, heightMode,
        mLynxContext.getScreenMetrics().density, lynxUIRenderer().getUIDelegatePtr());
    mPreWidthMeasureSpec = widthMeasureSpec;
    mPreHeightMeasureSpec = heightMeasureSpec;
  }

  public void updateFontScale(float scale) {
    if (!checkIfEnvPrepared() || mNativePtr == 0) {
      return;
    }
    nativeUpdateFontScale(mNativePtr, mNativeLifecycle, scale);
  }

  private void fallbackNewEngine(boolean enable_async) {
    if (!UIThreadUtils.isOnUiThread()) {
      LLog.e(TAG, "fallbackNewEngine should be called on ui thread, url: " + mUrl);
      return;
    }

    if (mHasDestroy) {
      return;
    }
    onTraceEventBegin(TraceEventDef.TEMPLATE_RENDER_FALLBACK_NEW_ENGINE);

    LLog.i(TAG, "call fallbackNewEngine:" + enable_async);
    reload = false;
    mHasPageStart = false;
    mSSRHelper = null;
    mRenderPhase = RENDER_PHASE_SETUP;

    if (mLynxContext != null) {
      mLynxContext.clearExposure();
    }

    if (globalProps != null) {
      globalProps = globalProps.deepClone();
    }

    int lastInstanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;
    if (mNativePtr != 0) {
      destroyLynxEngine();
    }

    int tempPreWidthMeasureSpec = mPreWidthMeasureSpec;
    int tempPreHeightMeasureSpec = mPreHeightMeasureSpec;
    mPreWidthMeasureSpec = 0;
    mPreHeightMeasureSpec = 0;

    mLynxEngineRef = null;
    mEnableReuseEngine = false;
    mIsEngineFromReuse = false;
    recreateLynxUIRenderer();
    if (mLynxUIRender != null) {
      mLynxUIRender.onInitLynxTemplateRender(
          mLynxContext, mLynxViewBuilder.behaviorRegistry, mBodyView, mLongTaskMonitorEnabled);
      if (mDevTool != null) {
        mDevTool.attachLynxUIOwner(mLynxUIRender.lynxUIOwner());
      }
    }
    if (mLynxContext != null) {
      mLynxContext.reset();
    }
    if (mBodyView != null) {
      // TODO(huangweiwu): This part of code will be removed after the Engine supports binding with
      // the original ViewTree.
      mBodyView.removeAllViews();
    }
    mThreadStrategyForRendering = enable_async ? ThreadStrategyForRendering.MOST_ON_TASM
                                               : ThreadStrategyForRendering.ALL_ON_UI;
    createLynxEngine(lastInstanceId);
    updateViewport(tempPreWidthMeasureSpec, tempPreHeightMeasureSpec);
    if (enable_async) {
      // TODO(huangweiwu): need the Engine supports binding with the original ViewTree.
    }
    onTraceEventEnd(TraceEventDef.TEMPLATE_RENDER_FALLBACK_NEW_ENGINE);
  }

  public void destroy() {
    String eventName = "LynxTemplateRender.destroy";
    onTraceEventBegin(eventName);

    // When destroy the page, we need send disexposure events before JSRuntime destroyed to ensure
    // the front end can receive events.
    if (mLynxContext != null) {
      mLynxContext.clearExposure();
    }
    recycleUpdatedDataList();
    destroyNative();
    onTraceEventBegin(TraceEventDef.CLIENT_REPORT_COMPONENT_INFO);
    mClient.onReportComponentInfo(new HashSet<>());
    onTraceEventEnd(TraceEventDef.CLIENT_REPORT_COMPONENT_INFO);
    recycleGlobalPropsSafely();
    ILynxExtensionService extensionService =
        LynxServiceCenter.inst().getService(ILynxExtensionService.class);
    if (extensionService != null) {
      extensionService.onLynxViewDestroy(mLynxContext);
    }
    if (mLynxContext != null && mLynxContext.getProviderRegistry() != null) {
      mLynxContext.getProviderRegistry().clear();
    }
    mLynxContext = null;
    if (mLynxUIRender != null) {
      mLynxUIRender.onDestroy();
    }
    LLog.i(TAG, formatLynxMessage("destroy"));
    onTraceEventEnd(eventName);
  }

  @Override
  protected void finalize() throws Throwable {
    super.finalize();
    destroyNative();
  }

  private void recycleUpdatedDataList() {
    for (TemplateData data : updatedDataList) {
      data.recycle();
    }
    updatedDataList.clear();
  }

  private void destroyNative() {
    if (mModuleFactory != null) {
      mModuleFactory.markLynxViewIsDestroying();
    }
    LLog.i(TAG, "destroyNative url " + getTemplateUrl() + " in " + this.toString());
    if (mDevTool != null) {
      mDevTool.destroy();
      mDevTool = null;
    }
    if (mNativePtr != 0) {
      // remove generic info of template instance before destroy.
      if (mLynxContext != null && mLynxContext.enableEventReporter()) {
        LynxEventReporter.clearCache(mLynxContext.getInstanceId());
      }
      destroyLynxEngine();
    }

    mHasDestroy = true;
  }

  public final ThreadStrategyForRendering getThreadStrategyForRendering() {
    return mThreadStrategyForRendering;
  }

  private void dispatchOnPageStart(final String url) {
    LLog.i(TAG, "dispatchOnPageStart url " + url + " in " + this.toString());
    if (mHasPageStart || null == mClient) {
      return;
    }
    mHasPageStart = true;
    if (mLynxContext != null && mLynxContext.enableEventReporter()) {
      LynxEventReporter.onEvent(EVENT_NAME_LYNX_OPEN_PAGE, null, mLynxContext.getInstanceId());
    }
    TraceEvent.instant(TraceEvent.CATEGORY_VITALS, TraceEventDef.TEMPLATE_RENDER_START_LOAD);

    onTraceEventBegin(TraceEventDef.CLIENT_ON_PAGE_START);
    mClient.onPageStart(url);
    LynxViewClientV2.LynxPipelineInfo pipelineInfo = new LynxViewClientV2.LynxPipelineInfo(url);
    pipelineInfo.addPipelineOrigin(
        LynxViewClientV2.LynxPipelineInfo.LynxPipelineOrigin.LYNX_FIRST_SCREEN);
    mClientV2.onPageStarted(getLynxView(), pipelineInfo);
    onTraceEventEnd(TraceEventDef.CLIENT_ON_PAGE_START);
  }

  private void dispatchLoadSuccess(int templateSize) {
    LLog.i(TAG, "dispatchLoadSuccess templateSize in " + this.toString());
    if (null == mClient) {
      return;
    }
    onTraceEventBegin(TraceEventDef.CLIENT_ON_LOAD_SUCCESS);
    mClient.onLoadSuccess();
    onTraceEventEnd(TraceEventDef.CLIENT_ON_LOAD_SUCCESS);
  }

  @Deprecated
  public void onErrorOccurred(int errCode, String errorMsg) {
    onErrorOccurred(new LynxError(errCode, errorMsg, null, LynxError.LEVEL_ERROR));
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  @Override
  public void onErrorOccurred(final LynxError error) {
    if (error == null || !error.isValid()) {
      LLog.e(TAG, "receive invalid error");
      return;
    }
    error.setTemplateUrl(mUrl);
    if (mLynxContext != null) {
      error.setCardVersion(mLynxContext.getPageVersion());
    }
    // show message in logbox
    showErrorMessage(error);
    if (mSSRHelper != null) {
      mSSRHelper.onErrorOccurred(error.getType(), error);
    }
    if (!error.isLogBoxOnly()) {
      if (mLynxContext != null && mLynxContext.enableEventReporter()) {
        // report error
        int instanceId = mLynxContext == null ? LynxEventReporter.INSTANCE_ID_UNKNOWN
                                              : mLynxContext.getInstanceId();
        LynxEventReporter.onEvent(LynxEventReporter.LYNX_SDK_ERROR_EVENT, instanceId,
            new LynxEventReporter.PropsBuilder() {
              @Override
              public Map<String, Object> build() {
                Map<String, Object> props = new HashMap<>();
                props.put("code", error.getErrorCode());
                props.put("level", error.getLevel() == null ? "" : error.getLevel());
                props.put("summary_message", error.getSummaryMessage());
                return props;
              }
            });
      }
      // dispatch to clients who listen
      dispatchError(error.getType(), error);
    }

    LLog.e(TAG,
        "LynxTemplateRender " + this.toString() + ": onErrorOccurred type " + error.getType()
            + ",errCode:" + error.getErrorCode() + ",detail:" + error.getMsg());
  }

  private void onExceptionOccurred(int errCode, Throwable throwable, JSONObject userDefineInfo) {
    if (throwable == null) {
      LLog.e(TAG, "receive null exception");
      return;
    }
    String stack = CallStackUtil.getStackTraceStringTrimmed(throwable);

    LynxError error = new LynxError(
        errCode, throwable.getMessage(), null, LynxError.LEVEL_ERROR, LynxError.JAVA_ERROR);
    error.setCallStack(stack);
    error.setUserDefineInfo(userDefineInfo);
    onErrorOccurred(error);
  }

  private void dispatchError(int type, LynxError lynxError) {
    // Compatible with old API
    onTraceEventBegin(TraceEventDef.TEMPLATE_RENDER_DISPATCH_ERROR);
    int errorCode = lynxError.getErrorCode();
    if (errorCode == LynxErrorBehavior.EB_APP_BUNDLE_LOAD) {
      mClient.onLoadFailed(lynxError.getMsg());
    } else {
      mClient.onReceivedError(lynxError.getMsg());
    }
    mClient.onReceivedError(lynxError);
    if (errorCode == LynxErrorBehavior.EB_BTS_RUNTIME_ERROR) {
      mClient.onReceivedJSError(lynxError);
    } else if (type == LynxError.NATIVE_ERROR) {
      mClient.onReceivedNativeError(lynxError);
    } else {
      mClient.onReceivedJavaError(lynxError);
    }
    onTraceEventEnd(TraceEventDef.TEMPLATE_RENDER_DISPATCH_ERROR);
  }

  private class InnerSSRLoadedCallback implements AbsTemplateProvider.Callback {
    private TemplateData mTemplateData;
    private String mUrl;

    public InnerSSRLoadedCallback(String url, TemplateData templateData) {
      this.mTemplateData = templateData;
      this.mUrl = url;
    }

    @Override
    public void onSuccess(byte[] template) {
      if (mDevTool != null) {
        mDevTool.attachToDebugBridge(mUrl);
      }

      if (mDevTool != null) {
        mDevTool.onTemplateLoadSuccess(template);
      }
      setMsTiming(TimingHandler.PREPARE_TEMPLATE_END, System.currentTimeMillis(), null);

      renderSSR(template, mUrl, mTemplateData);
    }

    @Override
    public void onFailed(String msg) {
      // indeed, there is no need to trigger onFailed on UIThread.
      // TODO(nihao.royal): remove post logic after lynxViewClientV2 in refined.
      final String stack =
          CallStackUtil.getStackTraceStringTrimmed(new Throwable("Fetch template resource failed"));
      UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
        @Override
        public void run() {
          String error_msg = "Error occurred while fetching app bundle resource";
          LynxError error =
              new LynxError(LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE, error_msg);
          error.setRootCause(msg);
          error.setCallStack(stack);
          onErrorOccurred(error);
        }
      });
    }
  }

  private class InnerLoadedCallback implements AbsTemplateProvider.Callback {
    private TemplateData mTemplateData;
    private String mJsonData;
    private final String mUrl;
    private Map<String, Object> mData;
    private LynxLoadMeta metaData;

    public InnerLoadedCallback(String url, LynxLoadMeta metaData) {
      this.mUrl = url;
      this.metaData = metaData;
    }

    public InnerLoadedCallback(String url, String jsonData) {
      mJsonData = jsonData;
      mUrl = url;
    }

    public InnerLoadedCallback(String url, TemplateData data) {
      mTemplateData = data;
      mUrl = url;
    }

    public InnerLoadedCallback(String url, Map<String, Object> data) {
      mData = data;
      mUrl = url;
    }

    @Override
    public void onSuccess(byte[] template) {
      // indeed, there is no need to trigger onFailed on UIThread.
      // TODO(nihao.royal): remove post logic later.
      UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
        @Override
        public void run() {
          if (template == null || template.length == 0) {
            onFailed("Source is null!");
            return;
          }
          if (mDevTool != null) {
            mDevTool.attachToDebugBridge(mUrl);
          }

          if (mDevTool != null) {
            mDevTool.onTemplateLoadSuccess(template);
          }
          setMsTiming(TimingHandler.PREPARE_TEMPLATE_END, System.currentTimeMillis(), null);

          if (metaData == null) {
            TemplateData templateData;
            if (mTemplateData != null) {
              templateData = mTemplateData;
            } else if (mData != null) {
              templateData = TemplateData.fromMap(mData);
            } else {
              templateData = TemplateData.fromString(mJsonData == null ? "" : mJsonData);
            }
            renderTemplate(template, templateData);
          } else {
            // if loading with LynxLoadMeta.
            TimingOption timingOption = new TimingOption(TimingConstants.LOAD_BUNDLE);
            long currentTimeMillis = System.currentTimeMillis();
            timingOption.setTiming(TimingConstants.PIPELINE_START, currentTimeMillis);
            timingOption.setTiming(TimingConstants.LOAD_BUNDLE_START, currentTimeMillis);
            metaData.binaryData = template;
            renderWithLoadMeta(metaData, timingOption);
          }
        }
      });
    }

    /**
     * Success with TemplateBundle, it's only used with genericResourceFetcher.
     */
    public void onSuccess(@NonNull TemplateBundle templateBundle) {
      if (!templateBundle.isValid()) {
        onFailed("template bundle is invalid.");
        return;
      }
      if (mDevTool != null) {
        mDevTool.attachToDebugBridge(mUrl);
      }

      if (mDevTool != null) {
        mDevTool.onLoadFromBundle(templateBundle, mTemplateData, mUrl);
      }
      setMsTiming(TimingHandler.PREPARE_TEMPLATE_END, System.currentTimeMillis(), null);

      if (metaData == null) {
        TemplateData templateData;
        if (mTemplateData != null) {
          templateData = mTemplateData;
        } else if (mData != null) {
          templateData = TemplateData.fromMap(mData);
        } else {
          templateData = TemplateData.fromString(mJsonData == null ? "" : mJsonData);
        }
        renderTemplateBundle(templateBundle, templateData, mUrl);
      } else {
        // if loading with LynxLoadMeta.
        TimingOption timingOption = new TimingOption(TimingConstants.LOAD_BUNDLE);
        long currentTimeMillis = System.currentTimeMillis();
        timingOption.setTiming(TimingConstants.PIPELINE_START, currentTimeMillis);
        timingOption.setTiming(TimingConstants.LOAD_BUNDLE_START, currentTimeMillis);
        metaData.bundle = templateBundle;
        renderWithLoadMeta(metaData, timingOption);
      }
    }

    @Override
    public void onFailed(String msg) {
      // indeed, there is no need to trigger onFailed on UIThread.
      // TODO(nihao.royal): remove post logic after lynxViewClientV2 in refined.
      final String stack =
          CallStackUtil.getStackTraceStringTrimmed(new Throwable("Fetch template resource failed"));
      UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
        @Override
        public void run() {
          String error_msg = "Error occurred while fetching app bundle resource";
          LynxError error =
              new LynxError(LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE, error_msg);
          error.setRootCause(msg);
          error.setCallStack(stack);
          onErrorOccurred(error);
        }
      });
    }
  }

  public class InnerPageLoadListener implements LynxPageLoadListener {
    @Override
    public void onFirstScreen() {
      LLog.i(TAG, "onFirstScreen");
      ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
      if (lynxUIRenderer != null) {
        lynxUIRenderer.setFirstLayout();
      }
      try {
        // run on ui thread.
        UIThreadUtils.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            if (mClient != null) {
              mClient.onFirstScreen();
            }
          }
        });
      } catch (Throwable e) {
        e.printStackTrace();
      }
    }

    @Override
    public void onPageUpdate() {
      try {
        // run on ui thread.
        LLog.i(TAG, "onPageUpdate");
        UIThreadUtils.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            if (mClient != null) {
              onTraceEventBegin(TraceEventDef.CLIENT_ON_PAGE_UPDATE);
              mClient.onPageUpdate();
              onTraceEventEnd(TraceEventDef.CLIENT_ON_PAGE_UPDATE);
            }
          }
        });
        if (mDevTool != null) {
          mDevTool.onPageUpdate();
        }
      } catch (Throwable e) {
        e.printStackTrace();
      }
    }

    @Override
    public void onUpdateDataWithoutChange() {
      try {
        // run on ui thread.
        UIThreadUtils.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            if (mClient != null) {
              onTraceEventBegin(TraceEventDef.CLIENT_ON_UPDATE_WITHOUT_CHANGE);
              mClient.onUpdateDataWithoutChange();
              onTraceEventEnd(TraceEventDef.CLIENT_ON_UPDATE_WITHOUT_CHANGE);
            }
          }
        });
      } catch (Throwable e) {
        e.printStackTrace();
      }
    }
  }

  public boolean dispatchTouchEvent(MotionEvent ev) {
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    return (lynxUIRenderer != null) ? lynxUIRenderer.onTouchEvent(ev, null) : false;
  }

  public boolean consumeSlideEvent(MotionEvent ev) {
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    return (lynxUIRenderer != null) ? lynxUIRenderer.consumeSlideEvent(ev) : false;
  }

  public boolean blockNativeEvent(MotionEvent ev) {
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    return (lynxUIRenderer != null) ? lynxUIRenderer.blockNativeEvent(ev) : false;
  }

  public void onInterceptTouchEvent(MotionEvent ev) {}

  public void onTouchEvent(MotionEvent ev) {}

  public void onRootViewDraw(Canvas canvas) {
    if (mLynxContext != null) {
      mLynxContext.onRootViewDraw(canvas);
    }
    if (mAutoFallbackEngineAfterDraw && mLynxEngineRef != null
        && !mLynxEngineRef.isRunOnCurrentTemplateRender(this)) {
      fallbackNewEngine(true);
      renderTemplateBundle(mLynxViewBuilder.templateBundle, mTemplateData, mUrl);
    }
  }

  public void onAttachedToWindow() {
    String eventName = "lynxview onAttachedToWindow " + this.toString();
    LLog.i(TAG, eventName);
    onTraceEventBegin(eventName);
    onEnterForeground(false);

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      lynxUIRenderer.onAttach();
    }

    if (mLynxContext != null) {
      mLynxContext.onAttachedToWindow();
    }
    onTraceEventEnd(eventName);
  }

  public void onDetachedFromWindow() {
    String eventName = "lynxview onDetachedFromWindow " + this.toString();
    LLog.i(TAG, eventName);
    onTraceEventBegin(eventName);
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      TraceEvent.beginSection(TraceEventDef.CLIENT_ON_REPORT_COMPONENT);
      mClient.onReportComponentInfo(new HashSet<>());
      TraceEvent.endSection(TraceEventDef.CLIENT_ON_REPORT_COMPONENT);
      lynxUIRenderer.onDetach();
    }

    onEnterBackground(false);
    onTraceEventEnd(eventName);
  }

  // when called onEnterForeground/onEnterBackground
  // directly by LynxView, force onShow/onHide,
  // else by onAttachedToWindow/onDetachedFromWindow,
  // need check autoExpose or not
  private void onEnterForeground(boolean forceChangeStatus) {
    LLog.i(TAG, "onEnterForeground. force: " + forceChangeStatus);
    if (mDevTool != null) {
      mDevTool.onEnterForeground();
    }

    if ((mNativePtr != 0) && (forceChangeStatus || getAutoExpose())) {
      nativeOnEnterForeground(mNativePtr, mNativeLifecycle);
    }

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      lynxUIRenderer.onEnterForegroundInternal();
    }
  }

  private void onEnterBackground(boolean forceChangeStatus) {
    LLog.i(TAG, "onEnterBackground. force: " + forceChangeStatus);
    if (mDevTool != null) {
      mDevTool.onEnterBackground();
    }

    if ((mNativePtr != 0) && (forceChangeStatus || getAutoExpose())) {
      nativeOnEnterBackground(mNativePtr, mNativeLifecycle);
    }

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      lynxUIRenderer.onEnterBackgroundInternal();
    }
  }

  private String[] processUrl(String url) {
    ArrayList<String> res = new ArrayList<String>();
    String template = url;
    String postUrl = "";
    String[] list = url.split("=|&");
    for (int i = 0; i + 1 < list.length; i += 2) {
      if (list[i].equalsIgnoreCase("compile_path") || list[i].equalsIgnoreCase("compilePath")) {
        template = list[i + 1];
      } else if (list[i].equalsIgnoreCase("post_url") || list[i].equalsIgnoreCase("postUrl")) {
        postUrl = list[i + 1];
      }
    }
    res.add(template);
    res.add(postUrl);
    return res.toArray(new String[res.size()]);
  }

  @Nullable
  public View findViewByName(String name) {
    LynxBaseUI uiByName = findUIByName(name);
    if (uiByName instanceof LynxUI) {
      return ((LynxUI) uiByName).getView();
    }
    return null;
  }

  @Nullable
  public LynxBaseUI findUIByName(String name) {
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    return (lynxUIRenderer != null) ? lynxUIRenderer.findLynxUIByName(name) : null;
  }

  @Nullable
  public View findViewByIdSelector(String id) {
    LynxBaseUI uiById = findUIByIdSelector(id);
    if (uiById instanceof LynxUI) {
      return ((LynxUI) uiById).getView();
    }
    return null;
  }

  @Nullable
  public LynxBaseUI findUIByIdSelector(String id) {
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    return (lynxUIRenderer != null) ? lynxUIRenderer.findLynxUIByIdSelector(id) : null;
  }

  public boolean attachLynxView(@NonNull final LynxView lynxView) {
    if (mBodyView != null) {
      LLog.e(TAG, "already attached " + lynxView);
      return false;
    }
    mContext = lynxView.getContext();
    Activity curActivity = ContextUtils.getActivity(mContext);
    if (TraceEvent.isTracingStarted()) {
      Map<String, String> traceProps = new HashMap<>();
      traceProps.put("curActivity", curActivity != null ? curActivity.toString() : "");
      TraceEvent.beginSection(TraceEventDef.TEMPLATE_RENDER_ATTACH_LYNX_VIEW, traceProps);
    }

    LLog.i(TAG, "LynxTemplateRender(" + this + ") is attached on lynxView:" + lynxView);
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    mBodyView = lynxView;
    mBodyView.setLynxUIRendererInternal(lynxUIRenderer);
    if (mDevTool != null) {
      mDevTool.attachContext(mContext);
    }

    if (mViewLayoutTick != null) {
      mViewLayoutTick.attach(mBodyView);
    }
    mBodyView.setTimingCollector(mPerformanceController);
    lynxUIRenderer.attachBodyView(lynxView, mLynxContext, mContext);
    if (curActivity != null) {
      lynxUIRenderer.setContextFree(false);
      nativeSetContextHasAttached(mNativePtr, mNativeLifecycle);
    }

    if (mDevTool != null) {
      mDevTool.attach(lynxView);
    }
    if (TraceEvent.isTracingStarted()) {
      TraceEvent.endSection(TraceEventDef.TEMPLATE_RENDER_ATTACH_LYNX_VIEW);
    }
    return true;
  }

  public long getFirstMeasureTime() {
    return mFirstMeasureTime;
  }

  public class TASMCallback implements NativeFacade.Callback {
    private static final String DEFAULT_ENTRY = "__Card__";

    public TASMCallback() {}

    @Override
    public void onLoaded(int templateSize) {
      HeroTransitionManager.inst().executeEnterAnim(mBodyView, null);
      if (mClient != null) {
        dispatchLoadSuccess(templateSize);
      }
      mRenderPhase = RENDER_PHASE_UPDATE;
      if (mDevTool != null) {
        mDevTool.onLoadFinished();
      }
    }

    @Override
    public void onSSRHydrateFinished() {
      if (mSSRHelper != null) {
        mSSRHelper.onHydrateFinished();
      }
    }

    @Override
    public void onRuntimeReady() {
      if (mClient != null) {
        onTraceEventBegin(TraceEventDef.CLIENT_ON_RUNTIME_READY);
        mClient.onRuntimeReady();
        onTraceEventEnd(TraceEventDef.CLIENT_ON_RUNTIME_READY);
      }
    }

    @Override
    public void onDataUpdated() {
      if (mClient != null) {
        onTraceEventBegin(TraceEventDef.CLIENT_ON_DATA_UPDATED);
        mClient.onDataUpdated();
        onTraceEventEnd(TraceEventDef.CLIENT_ON_DATA_UPDATED);
      }
    }

    @Override
    public void onPageChanged(boolean isFirstScreen) {
      if (isFirstScreen) {
        // TODO(heshan):remove PageLoadListener.
        mPageLoadListener.onFirstScreen();
      } else {
        mPageLoadListener.onPageUpdate();
      }
    }

    @Override
    public void onDynamicComponentPerfReady(HashMap<String, Object> perf) {
      if (mClient != null) {
        onTraceEventBegin(TraceEventDef.CLIENT_ON_DYNAMIC_COMPONENT_PERF);
        mClient.onDynamicComponentPerfReady(perf);
        onTraceEventEnd(TraceEventDef.CLIENT_ON_DYNAMIC_COMPONENT_PERF);
      }
    }

    @Override
    public void onErrorOccurred(LynxError error) {
      LynxTemplateRender.this.onErrorOccurred(error);
    }

    @Override
    public void onThemeUpdatedByJs(LynxTheme theme) {
      if (theme == null)
        return;

      if (mTheme == null) {
        mTheme = theme;
      } else {
        mTheme.replaceWithTheme(theme);
      }
    }

    @Override
    public String translateResourceForTheme(String resId, String themedKey) {
      ThemeResourceProvider provider = LynxEnv.inst().getThemeResourceProviderProvider();
      if (provider != null) {
        try {
          if (themedKey != null && themedKey.isEmpty()) {
            themedKey = null;
          }
          return provider.translateResourceForTheme(resId, mTheme, themedKey, getLynxView());
        } catch (Throwable e) {
          LLog.d(TAG, "translateResourceForTheme exception " + e.toString());
        }
      }
      return null;
    }

    // issue: #1510
    @Override
    public void onModuleFunctionInvoked(String module, String method, int error_code) {
      if (mClient != null) {
        onTraceEventBegin(TraceEventDef.CLIENT_ON_MODULE_FUNCTION);
        mClient.onModuleMethodInvoked(module, method, error_code);
        onTraceEventEnd(TraceEventDef.CLIENT_ON_MODULE_FUNCTION);
      }
    }

    @Override
    public void onPageConfigDecoded(PageConfig config) {
      if (mEnableReuseEngine && mLynxEngineRef != null) {
        mLynxEngineRef.setPageConfig(config);
      }
      PageConfig.attachPageConfig(config, mLynxContext, lynxUIRenderer());
    }

    @Override
    public void onJSBInvoked(Map<String, Object> jsbInfo) {
      if (mClient != null) {
        mClient.onJSBInvoked(jsbInfo);
      }
    }

    @Override
    public void onCallJSBFinished(Map<String, Object> timing) {
      if (mClient != null) {
        mClient.onCallJSBFinished(timing);
      }
      if (LynxDevtoolGlobalHelper.getInstance().isRemoteDebugAvailable()) {
        int instanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;
        if (mLynxContext != null) {
          instanceId = mLynxContext.getInstanceId();
        }
        LynxDevtoolGlobalHelper.getInstance().onPerfMetricsEvent(
            "lynxsdk_jsb_timing", new JSONObject(timing), instanceId);
      }
    }

    @Override
    public void onUpdateDataWithoutChange() {
      if (mClient != null) {
        onTraceEventBegin(TraceEventDef.CLIENT_ON_UPDATE_WITHOUT_CHANGE);
        mClient.onUpdateDataWithoutChange();
        onTraceEventEnd(TraceEventDef.CLIENT_ON_UPDATE_WITHOUT_CHANGE);
      }
    }

    @Override
    public void onTemplateBundleReady(TemplateBundle bundle) {
      if (mClient != null) {
        onTraceEventBegin(TraceEventDef.CLIENT_ON_TEMPLATE_BUNDLE_READY);
        mClient.onTemplateBundleReady(bundle);
        onTraceEventEnd(TraceEventDef.CLIENT_ON_TEMPLATE_BUNDLE_READY);
      }
    }

    @Override
    public void onReceiveMessageEvent(ReadableMap event) {
      if (mDevTool != null) {
        mDevTool.onReceiveMessageEvent(event);
      }
    }

    @Override
    public void onTASMFinishedByNative() {
      if (mClient != null) {
        onTraceEventBegin(TraceEventDef.CLIENT_ON_TASM_FINISHED_BY_NATIVE);
        mClient.onTASMFinishedByNative();
        onTraceEventEnd(TraceEventDef.CLIENT_ON_TASM_FINISHED_BY_NATIVE);
      }
    }

    @Override
    public void onUpdateI18nResource(String key, String bytes, int status) {
      nativeUpdateI18nResource(mNativePtr, mNativeLifecycle, key, bytes, status);
    }

    @Override
    public void onUIMethodInvoked(final int cb, JavaOnlyMap res) {
      if (enableAirStrictMode()) {
        if (mEngineProxy != null) {
          ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(res);
          mEngineProxy.invokeLepusApiCallback(cb, DEFAULT_ENTRY, buffer);
        }
      } else if (mJSProxy != null) {
        mJSProxy.callJSApiCallbackWithValue(cb, res);
      }
    }

    @Override
    public void onClearNativePipelineTimingInfo() {
      nativeClearPipelineTimingInfo(mNativePtr, mNativeLifecycle);
    }

    @Override
    public void onEventCapture(long targetID, boolean isCatch, long eventID) {
      if (mLynxContext != null && mLynxContext.getLynxUIOwner() != null) {
        EventTarget target = mLynxContext.getLynxUIOwner().findLynxUIBySign((int) targetID);
        if (target != null) {
          target.onEventCapture(isCatch, eventID);
        }
      }
    }

    @Override
    public void onEventBubble(long targetID, boolean isCatch, long eventID) {
      if (mLynxContext != null && mLynxContext.getLynxUIOwner() != null) {
        EventTarget target = mLynxContext.getLynxUIOwner().findLynxUIBySign((int) targetID);
        if (target != null) {
          target.onEventBubble(isCatch, eventID);
        }
      }
    }

    @Override
    public void onEventFire(long targetID, boolean isStop, long eventID) {
      if (mLynxContext != null && mLynxContext.getLynxUIOwner() != null) {
        EventTarget target = mLynxContext.getLynxUIOwner().findLynxUIBySign((int) targetID);
        if (target != null) {
          target.onEventFire(isStop, eventID);
        }
      }
    }
  }

  @Nullable
  public LynxBaseUI findUIByIndex(int index) {
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    return (lynxUIRenderer != null) ? lynxUIRenderer.findLynxUIByIndex(index) : null;
  }

  public void onDispatchInputEvent(InputEvent ev) {
    if (mDevTool != null) {
      mDevTool.onRootViewInputEvent(ev);
    }
  }

  public Map<String, Object> getAllJsSource() {
    return mNativePtr != 0 ? nativeGetAllJsSource(mNativePtr, mNativeLifecycle) : null;
  }

  public boolean enableJSRuntime() {
    return mEnableJSRuntime;
  }

  public void syncFlush() {
    String eventName = "LynxTemplateRender.syncFlush";
    onTraceEventBegin(eventName);
    UIThreadUtils.assertOnUiThread();
    if (mAsyncRender && !mIsDestroyed.get()) {
      LLog.i(TAG, "syncFlush wait layout finish");
      if (mNativePtr != 0) {
        nativeFlush(mNativePtr, mNativeLifecycle);
      }
    }
    onTraceEventEnd(eventName);
  }

  public void runOnTasmThread(Runnable runnable) {
    if (mEngineProxy == null) {
      LLog.i(TAG, "runOnTasmThread failed, engine proxy is null.");
      return;
    }

    mEngineProxy.dispatchTaskToLynxEngine(runnable);
  }

  public void startLynxRuntime() {
    if (mNativePtr != 0) {
      // Once LynxView starts its LynxRuntime, user expects that
      // LynxRuntime is always running, reload/load operations
      // on such LynxView shouldn't enable PendingJsTask.
      mEnablePendingJsTask = false;
      nativeStartRuntime(mNativePtr, mNativeLifecycle);
    }
  }

  @Deprecated
  public void processLayout(@NonNull String templateUrl, final TemplateData templateData) {
    setEnableUIFlush(false);
    renderTemplateUrl(templateUrl, templateData);
  }

  @Deprecated
  public void processLayoutWithSSRUrl(@NonNull String ssrUrl, final TemplateData templateData) {
    setEnableUIFlush(false);
    renderSSRUrl(ssrUrl, templateData);
  }

  @Deprecated
  public void processLayoutWithTemplateBundle(
      final TemplateBundle bundle, final TemplateData templateData, String baseUrl) {
    setEnableUIFlush(false);
    renderTemplateBundle(bundle, templateData, baseUrl);
  }

  public void setEnableUIFlush(boolean enableUIFlush) {
    if (mNativePtr != 0 && mEnableUIFlush != enableUIFlush) {
      mEnableUIFlush = enableUIFlush;
      nativeSetEnableUIFlush(mNativePtr, mNativeLifecycle, enableUIFlush);
    }
  }

  public void processRender() {
    onTraceEventBegin(TraceEventDef.TEMPLATE_RENDER_PROCESS_RENDER);
    if (mNativePtr != 0 && !mEnableUIFlush) {
      setEnableUIFlush(true);
      nativeProcessRender(mNativePtr, mNativeLifecycle);
    }
    onTraceEventEnd(TraceEventDef.TEMPLATE_RENDER_PROCESS_RENDER);
  }

  public void setEnableBytecode(boolean enableUserBytecode, String url) {
    if (mEnableBytecode == enableUserBytecode && Objects.equals(mBytecodeSourceUrl, url)) {
      return;
    }
    mEnableBytecode = enableUserBytecode;
    mBytecodeSourceUrl = url;
    if (mNativePtr != 0) {
      nativeSetEnableBytecode(mNativePtr, mNativeLifecycle, enableUserBytecode, url);
    }
  }

  public void setSessionStorageItem(String key, TemplateData data) {
    LLog.d(TAG, "setSessionStorageItem with key: " + key);
    if ((mNativePtr != 0) && (data != null) && (!TextUtils.isEmpty(key))) {
      data.flush();
      long ptr = data.getNativePtr();
      if (ptr == 0) {
        LLog.e(TAG, "setSessionStorageItem with zero data! key: " + key);
        return;
      }
      nativeSetSessionStorageItem(
          mNativePtr, mNativeLifecycle, key, data.getNativePtr(), data.isReadOnly());
    }
  }

  public void getSessionStorageItem(String key, PlatformCallBack callback) {
    LLog.d(TAG, "getSessionStorageItem with key: " + key);
    if ((!TextUtils.isEmpty(key)) && (mNativePtr != 0)) {
      nativeGetSessionStorageItem(mNativePtr, mNativeLifecycle, key, callback);
    }
  }

  public double subscribeSessionStorage(String key, PlatformCallBack callBack) {
    LLog.d(TAG, "subscribeSessionStorage with key: " + key);
    if ((!TextUtils.isEmpty(key)) && (mNativePtr != 0)) {
      double listenerId =
          nativeSubscribeSessionStorage(mNativePtr, mNativeLifecycle, key, callBack);
      platformCallBackMap.put(listenerId, callBack);
      return listenerId;
    }
    return PlatformCallBack.InvalidId;
  }

  public void unsubscribeSessionStorage(String key, double id) {
    LLog.d(TAG, "unsubscribeSessionStorage with key: " + key);
    if ((!TextUtils.isEmpty(key)) && (mNativePtr != 0) && (PlatformCallBack.InvalidId != id)) {
      nativeUnsubscribeSessionStorage(mNativePtr, mNativeLifecycle, key, id);
      platformCallBackMap.remove(id);
    }
  }

  public void attachEngineToUIThread() {
    if (mNativePtr != 0) {
      if (!UIThreadUtils.isOnUiThread()) {
        LLog.e(TAG, "attachEngineToUIThread should be called on ui thread, url: " + mUrl);
        return;
      }
      switch (mThreadStrategyForRendering) {
        case MOST_ON_TASM:
          mThreadStrategyForRendering = ThreadStrategyForRendering.ALL_ON_UI;
          break;
        case MULTI_THREADS:
          mThreadStrategyForRendering = ThreadStrategyForRendering.PART_ON_LAYOUT;
          break;
        default:
          return;
      }
      nativeAttachEngineToUIThread(mNativePtr, mNativeLifecycle);
      onThreadStrategyUpdated();
    }
  }

  public void detachEngineFromUIThread() {
    if (mNativePtr != 0) {
      if (!UIThreadUtils.isOnUiThread()) {
        LLog.e(TAG, "detachEngineFromUIThread should be called on ui thread, url: " + mUrl);
        return;
      }
      switch (mThreadStrategyForRendering) {
        case ALL_ON_UI:
          mThreadStrategyForRendering = ThreadStrategyForRendering.MOST_ON_TASM;
          break;
        case PART_ON_LAYOUT:
          mThreadStrategyForRendering = ThreadStrategyForRendering.MULTI_THREADS;
          break;
        default:
          return;
      }
      nativeDetachEngineFromUIThread(mNativePtr, mNativeLifecycle);
      onThreadStrategyUpdated();
    }
  }

  private void onThreadStrategyUpdated() {
    mAsyncRender = (mThreadStrategyForRendering == ThreadStrategyForRendering.MULTI_THREADS
        || mThreadStrategyForRendering == ThreadStrategyForRendering.MOST_ON_TASM);
    if (mLynxContext != null && mLynxContext.enableEventReporter()) {
      LynxEventReporter.updateGenericInfo(LynxEventReporter.PROP_NAME_THREAD_MODE,
          mThreadStrategyForRendering.id(), mLynxContext.getInstanceId());
    }
  }

  private void loadTemplate(byte[] template, TemplateData initData, String url,
      NativeFacade.Callback callback, TimingOption timingOption) {
    this.loadTemplate(template, initData, url, false, false, callback, timingOption);
  }

  private void loadTemplate(byte[] template, TemplateData initData, String url,
      boolean isPrePainting, boolean enableRecycleTemplateBundle, NativeFacade.Callback callback,
      TimingOption timingOption) {
    if (template == null) {
      LLog.e(TAG, "Load Template with null template");
      return;
    }
    if ((mNativeFacade == null) || (mNativePtr == 0)) {
      LLog.e(TAG, "Load Template before inited");
      return;
    }
    long nativePtr = 0;
    String processorName = null;
    boolean read_only = false;
    if (initData != null) {
      initData.flush();
      nativePtr = initData.getNativePtr();
      processorName = initData.processorName();
      read_only = initData.isReadOnly();
      initData.markConsumed();
    }
    if (nativePtr == 0) {
      LLog.e(TAG, "Load Template with zero template data");
    }
    mNativeFacade.setUrl(url);
    mNativeFacade.setCallback(callback);
    mNativeFacade.setSize(template.length);
    if (mDevTool != null) {
      mDevTool.attachToDebugBridge(url);
    }
    nativeLoadTemplate(url, template, isPrePainting ? 1 : 0, enableRecycleTemplateBundle, read_only,
        processorName, initData, timingOption);
  }

  private void loadTemplateBundle(TemplateBundle bundle, String url, TemplateData initData,
      boolean isPrePainting, boolean enableDumpElementTree, NativeFacade.Callback callback,
      TimingOption timingOption) {
    if ((mNativeFacade == null) || (mNativePtr == 0)) {
      LLog.e(TAG, "LoadTemplateBundle before inited");
      return;
    }
    if (bundle == null || !bundle.isValid()) {
      String errorMsg = "LoadTemplateBundle with null bundle or invalid bundle";
      LLog.e(TAG, errorMsg);
      LynxError lynxError = new LynxError(LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_BUNDLE, errorMsg);
      lynxError.setRootCause(bundle == null ? "bundle is null" : bundle.getErrorMessage());
      mNativeFacade.reportError(lynxError);
      return;
    }

    long nativePtr = 0;
    String processorName = null;
    boolean read_only = false;
    if (initData != null) {
      initData.flush();
      nativePtr = initData.getNativePtr();
      processorName = initData.processorName();
      read_only = initData.isReadOnly();
      initData.markConsumed();
    }

    if (nativePtr == 0) {
      LLog.e(TAG, "LoadTemplateBundle with zero templateData");
    }

    mNativeFacade.setUrl(url);
    mNativeFacade.setCallback(callback);
    mNativeFacade.setSize(bundle.getTemplateSize());
    if (mDevTool != null) {
      mDevTool.attachToDebugBridge(url);
    }
    PageConfig.attachPageConfig(bundle.getPageConfig(), mLynxContext, lynxUIRenderer());
    timingOption.setTiming(TimingConstants.FFI_START, System.currentTimeMillis());
    nativeLoadTemplateBundleByPreParsedData(mNativePtr, mNativeLifecycle, url,
        bundle.getNativePtr(), isPrePainting ? 1 : 0, nativePtr, read_only, processorName, initData,
        enableDumpElementTree, timingOption.toJavaOnlyMap());
    tryRegisterLynxEngineReused();
  }

  private void loadSSRData(byte[] ssr, TemplateData templateData, NativeFacade.Callback callback) {
    if ((mNativeFacade == null) || (mNativePtr == 0)) {
      LLog.e(TAG, "Load ssr data before inited");
      return;
    }
    if (ssr == null) {
      LLog.e(TAG, "Load ssr data  with null template");
      return;
    }
    long nativePtr = 0;
    String processorName = null;
    boolean readOnly = false;
    if (templateData != null) {
      templateData.flush();
      nativePtr = templateData.getNativePtr();
      processorName = templateData.processorName();
      readOnly = templateData.isReadOnly();
    }
    mNativeFacade.setCallback(callback);
    nativeLoadSSRDataByPreParsedData(
        mNativePtr, mNativeLifecycle, ssr, nativePtr, readOnly, processorName, templateData);
  }

  private void loadTemplate(byte[] template, String initData, String url,
      NativeFacade.Callback callback, TimingOption timingOption) {
    if ((mNativeFacade == null) || (mNativePtr == 0)) {
      LLog.e(TAG, "Load Template before inited");
      return;
    }
    if (template == null) {
      LLog.e(TAG, "Load Template with null template");
      return;
    }
    mNativeFacade.setUrl(url);
    mNativeFacade.setCallback(callback);
    mNativeFacade.setSize(template.length);
    TemplateData templateData = TemplateData.fromString(initData);
    templateData.flush();
    templateData.markConsumed();

    nativeLoadTemplate(url, template, 0, false, true, "", templateData, timingOption);
  }

  private void loadTemplate(byte[] template, Map<String, Object> initData, String url,
      NativeFacade.Callback callback, TimingOption timingOption) {
    if ((mNativeFacade == null) || (mNativePtr == 0)) {
      LLog.e(TAG, "Load Template before inited");
      return;
    }
    if (template == null) {
      LLog.e(TAG, "Load Template with null template");
      return;
    }
    mNativeFacade.setUrl(url);
    mNativeFacade.setCallback(callback);
    mNativeFacade.setSize(template.length);
    TemplateData templateData = TemplateData.fromMap(initData);
    templateData.flush();
    templateData.markConsumed();

    nativeLoadTemplate(url, template, 0, false, true, "", templateData, timingOption);
  }

  private void nativeLoadTemplate(String url, byte[] template, int isPrePainting,
      boolean enableRecycleTemplateBundle, boolean readOnly, String processorName,
      TemplateData templateData, TimingOption timingOption) {
    ILynxSecurityService securityService =
        LynxServiceCenter.inst().getService(ILynxSecurityService.class);
    if (securityService != null) {
      // Do Security Check;
      timingOption.setTiming(TimingConstants.VERIFY_TASM_START, System.currentTimeMillis());
      SecurityResult result = securityService.verifyTASM(
          getLynxView(), template, url, ILynxSecurityService.LynxTasmType.TYPE_TEMPLATE);
      timingOption.setTiming(TimingConstants.VERIFY_TASM_END, System.currentTimeMillis());
      if (!result.isVerified()) {
        mNativeFacade.reportError(new LynxError(
            LynxSubErrorCode.E_APP_BUNDLE_VERIFY_INVALID_SIGNATURE, result.getErrorMsg()));
        return;
      }
    }

    // SecurityService is null, or Verified;
    timingOption.setTiming(TimingConstants.FFI_START, System.currentTimeMillis());
    long nativePtr = templateData == null ? 0 : templateData.getNativePtr();
    nativeLoadTemplateByPreParsedData(mNativePtr, mNativeLifecycle, url, template, isPrePainting,
        enableRecycleTemplateBundle, nativePtr, readOnly, processorName, templateData,
        timingOption.toJavaOnlyMap());
  }

  public boolean registerLazyBundle(String url, TemplateBundle bundle) {
    String errorMsg = null;
    String originCause = null;
    if (TextUtils.isEmpty(url)) {
      errorMsg = "url is empty";
    } else if (bundle == null) {
      errorMsg = "bundle is null";
    } else if (!bundle.isValid()) {
      errorMsg = "bundle is invalid";
      originCause = bundle.getErrorMessage();
    } else if (!nativeRegisterLazyBundle(
                   mNativePtr, mNativeLifecycle, url, bundle.getNativePtr())) {
      errorMsg = "input bundle is not from a dynamic component template";
    }
    if (errorMsg != null) {
      LynxError lynxError = new LynxError(LynxSubErrorCode.E_LAZY_BUNDLE_LOAD_BAD_BUNDLE, errorMsg);
      lynxError.setRootCause(originCause);
      lynxError.addCustomInfo("component_url", url);
      if (mNativeFacade != null) {
        mNativeFacade.reportError(lynxError);
      }
      return false;
    }
    return true;
  }

  public synchronized void updateGlobalPropsInternal(TemplateData globalProps) {
    if (globalProps == null) {
      return;
    }
    globalProps.flush();
    long propPtr = globalProps.getNativePtr();
    if (propPtr == 0) {
      LLog.e(TAG, "updateGlobalProps with zero templateData");
      return;
    }
    nativeUpdateGlobalProps(mNativePtr, mNativeLifecycle, propPtr);
  }

  private void attachPiper(LynxBackgroundRuntime runtime, LynxModuleFactory moduleFactory) {
    mNativeFacade.setModuleFactory(moduleFactory);

    nativeAttachRuntime(mNativePtr, mNativeLifecycle, runtime.getNativePtr());
    String jsGroupThreadName = getJSGroupThreadNameIfNeed();
    WeakReference<LynxContext> weakContext = mNativeFacade.getLynxContext();
    mJSProxy = new JSProxy(mNativePtr, weakContext, jsGroupThreadName);
    mNativeFacade.setJSProxy(mJSProxy);
    LynxContext context = weakContext.get();
    if (context != null) {
      LLog.i(TAG, "set JSGroupThreadName to lynx context: " + jsGroupThreadName);
      context.setJSGroupThreadName(jsGroupThreadName);
    }
    mEngineProxy = new LynxEngineProxy(mNativePtr);
    mNativeFacade.setEngineProxy(mEngineProxy);
  }

  public LynxEngineProxy getEngineProxy() {
    return mEngineProxy;
  }

  private void initPiper(LynxModuleFactory moduleFactory, LynxResourceLoader resourceLoader,
      boolean useQuickJSEngine, boolean forceReloadJSCore, boolean enableUserBytecode,
      String bytecodeSourceUrl, boolean enablePendingJsTask, ILynxUIRenderer lynxUIRenderer) {
    TraceEvent.beginSection(TraceEventDef.TEMPLATE_RENDER_INIT_PIPER);
    initPiperInternal(moduleFactory, resourceLoader, useQuickJSEngine, forceReloadJSCore,
        enableUserBytecode, bytecodeSourceUrl, enablePendingJsTask, lynxUIRenderer);
    TraceEvent.endSection(TraceEventDef.TEMPLATE_RENDER_INIT_PIPER);
  }

  private void initPiperInternal(LynxModuleFactory moduleFactory, LynxResourceLoader resourceLoader,
      boolean useQuickJSEngine, boolean forceReloadJSCore, boolean enableUserBytecode,
      String bytecodeSourceUrl, boolean enablePendingJsTask, ILynxUIRenderer lynxUIRenderer) {
    mNativeFacade.setModuleFactory(moduleFactory);
    if (useQuickJSEngine) {
      LLog.i(TAG, "force use quick js engine");
    } else {
      LLog.i(TAG, "useQuickJSEngine is false");
    }

    final int runtimeFlags = LynxBackgroundRuntimeOptions.calcRuntimeFlags(
        forceReloadJSCore, useQuickJSEngine, enablePendingJsTask, enableUserBytecode, null, null);
    nativeInitRuntime(mNativePtr, resourceLoader, moduleFactory, getGroupID(), getPreloadJSPath(),
        bytecodeSourceUrl, runtimeFlags, lynxUIRenderer.getUIDelegatePtr());
    String jsGroupThreadName = getJSGroupThreadNameIfNeed();
    WeakReference<LynxContext> weakContext = mNativeFacade.getLynxContext();
    if (mNativeFacade.getEnableJSRuntime()) {
      mJSProxy = new JSProxy(mNativePtr, weakContext, jsGroupThreadName);
    }
    mNativeFacade.setJSProxy(mJSProxy);
    if (weakContext.get() != null) {
      LLog.i(TAG, "set JSGroupThreadName to lynx context: " + jsGroupThreadName);
      weakContext.get().setJSGroupThreadName(jsGroupThreadName);
    }
    mEngineProxy = new LynxEngineProxy(mNativePtr);
    mNativeFacade.setEngineProxy(mEngineProxy);

    mLayoutProxy = new LynxLayoutProxy(mNativePtr);
    mLynxContext.setLayoutProxy(mLayoutProxy);
  }

  // TODO(hexionghui): This interface will be deleted later. Since LynxSendCustomEventRunnable
  // relies on this interface, it cannot be deleted temporarily.
  public void sendCustomEvent(@NonNull LynxCustomEvent event) {
    LynxContext context = mLynxContext;
    if (context == null) {
      LLog.e(
          TAG, "sendCustomEvent event: " + event.getName() + " failed since mLynxContext is null.");
      return;
    }
    if (context.getEventEmitter() == null) {
      LLog.e(TAG,
          "sendCustomEvent event: " + event.getName()
              + " failed since mLynxContext.getEventEmitter() is null.");
      return;
    }
    context.getEventEmitter().sendCustomEvent(event);
  }

  public JavaOnlyMap getListPlatformInfo(int listSign) {
    return nativeGetListPlatformInfo(mNativePtr, mNativeLifecycle, listSign);
  }

  public void renderChild(int listSign, int index, long operationId) {
    nativeRenderChild(mNativePtr, mNativeLifecycle, listSign, index, operationId);
  }

  public void updateChild(int listSign, int oldSign, int newIndex, long operationId) {
    nativeUpdateChild(mNativePtr, mNativeLifecycle, listSign, oldSign, newIndex, operationId);
  }

  public void removeChild(int listSign, int childSign) {
    nativeRemoveChild(mNativePtr, mNativeLifecycle, listSign, childSign);
  }

  public int obtainChild(
      int listSign, int index, long operationId, boolean enableReuseNotification) {
    return nativeObtainChild(
        mNativePtr, mNativeLifecycle, listSign, index, operationId, enableReuseNotification);
  }

  public void recycleChild(int listSign, int childSign) {
    nativeRecycleChild(mNativePtr, mNativeLifecycle, listSign, childSign);
  }

  public void obtainChildAsync(int listSign, int index, long operationId) {
    nativeObtainChildAsync(mNativePtr, mNativeLifecycle, listSign, index, operationId);
  }

  public void recycleChildAsync(int listSign, int childSign) {
    nativeRecycleChildAsync(mNativePtr, mNativeLifecycle, listSign, childSign);
  }

  public void scrollByListContainer(
      int containerSign, float x, float y, float originalX, float originalY) {
    nativeScrollByListContainer(
        mNativePtr, mNativeLifecycle, containerSign, x, y, originalX, originalY);
  }

  public void scrollToPosition(
      int containerSign, int position, float offset, int align, boolean smooth) {
    nativeScrollToPosition(
        mNativePtr, mNativeLifecycle, containerSign, position, offset, align, smooth);
  }

  public void scrollStopped(int containerSign) {
    nativeScrollStopped(mNativePtr, mNativeLifecycle, containerSign);
  }

  private String getJSGroupThreadNameIfNeed() {
    if (mGroup != null && mGroup.enableJSGroupThread()) {
      return getGroupID();
    }
    return "";
  }

  private void destroyLynxEngine() {
    if (!mIsDestroyed.compareAndSet(false, true)) {
      return;
    }

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      if (mLynxEngineRef == null) {
        lynxUIRenderer.onDestroyTemplateRenderer();
      } else if (mLynxEngineRef != null && mLynxEngineRef.isRunOnCurrentTemplateRender(this)) {
        lynxUIRenderer.onDestroyTemplateRenderer();
        mLynxEngineRef.destroy();
      }
    }

    if (mNativeFacade != null) {
      mNativeFacade.destroyAnyThreadPart();
    }

    // need ensure destroy native on ui thread
    UIThreadUtils.runOnUiThreadImmediately(
        new DestroyTask(mNativePtr, mNativeLifecycle, this, mNativeFacade));

    if (platformCallBackMap != null) {
      platformCallBackMap.clear();
    }

    if (mJSProxy != null) {
      mJSProxy.destroy();
      mJSProxy = null;
    }

    if (mEngineProxy != null) {
      mEngineProxy.destroy();
    }

    if (mLayoutProxy != null) {
      mLayoutProxy.destroy();
    }
    mNativeFacade = null;
    mNativeLifecycle = 0;
    mNativePtr = 0;
  }

  private static class DestroyTask implements Runnable {
    private long mNativePtr;
    private long mNativeLifecycle;
    private LynxTemplateRender mRenderer;
    private NativeFacade mNativeFacade;

    public DestroyTask(long nativePtr, long nativeLifecycle, LynxTemplateRender renderer,
        NativeFacade nativeFacade) {
      mNativePtr = nativePtr;
      mNativeLifecycle = nativeLifecycle;
      mRenderer = ((nativeLifecycle != 0) && (nativePtr != 0)) ? renderer : null;
      mNativeFacade = nativeFacade;
    }

    @Override
    public void run() {
      if ((mNativeLifecycle != 0) && (mNativePtr != 0)) {
        if (nativeLifecycleTryTerminate(mNativeLifecycle)) {
          nativeDestroy(mNativePtr);
          mNativePtr = 0;
          mNativeLifecycle = 0;
          mRenderer = null;
        } else {
          // retry later
          UIThreadUtils.runOnUiThread(this, 1);
        }
      }
      if (mNativeFacade != null) {
        mNativeFacade.destroyUiThreadPart();
        mNativeFacade = null;
      }
    }
  }

  // Remember: this MUST be a static class, to avoid an implicit ref back to the
  // owning ReferredObject instance which would defeat GC of that object.
  private static class CleanupOnUiThread implements Runnable {
    private long mNativePtr;

    public CleanupOnUiThread(long nativePtr) {
      mNativePtr = nativePtr;
    }

    @Override
    public void run() {
      if (mNativePtr == 0) {
        return;
      }
      nativeLifecycleDestroy(mNativePtr);
      mNativePtr = 0;
    }
  }

  private boolean getAutoExpose() {
    return mLynxContext != null && mLynxContext.getAutoExpose();
  }

  private String getGroupID() {
    return mGroup != null ? mGroup.getID() : LynxGroup.SINGNLE_GROUP;
  }

  @Nullable
  private String[] getPreloadJSPath() {
    return mGroup != null ? mGroup.getPreloadJSPaths() : null;
  }

  private void recreateLynxUIRenderer() {
    mLynxUIRender = mLynxViewBuilder.uiRenderCreator.createLynxUIRender();
    mBodyView.setLynxUIRendererInternal(mLynxUIRender);
  }

  private boolean isEngineReuseOnCurrentLynxView() {
    if (mLynxEngineRef != null && mLynxEngineRef.isRunOnCurrentTemplateRender(this)) {
      return true;
    }
    return false;
  }

  private boolean isEngineReuseHasLoaded() {
    if (mLynxEngineRef != null && mLynxEngineRef.hasLoaded()) {
      return true;
    }
    return false;
  }

  public ILynxUIRenderer lynxUIRenderer() {
    if (mLynxUIRender == null) {
      if (mBodyView != null) {
        mLynxUIRender = mBodyView.getLynxUIRendererInternal();
      } else {
        // for context free.
        mLynxUIRender = mLynxViewBuilder.uiRenderCreator.createLynxUIRender();
      }
    }
    return mLynxUIRender;
  }

  void dispatchMessageEvent(ReadableMap event) {
    if (mNativePtr != 0) {
      nativeDispatchMessageEvent(mNativePtr, mNativeLifecycle, event);
    }
  }

  private void updateMetaDataInternal(TemplateData data, TemplateData globalProps) {
    if (data == null && globalProps == null) {
      LLog.e(TAG, "updateMetaData with null data and null globalProps.");
      return;
    }

    long dataPtr = 0;
    String processorName = null;
    boolean readOnly = false;
    if (data != null) {
      data.flush();
      dataPtr = data.getNativePtr();
      processorName = data.processorName();
      readOnly = data.isReadOnly();
    }

    long globalPropsPtr = 0;
    if (globalProps != null) {
      globalProps.flush();
      globalPropsPtr = globalProps.getNativePtr();
    }

    nativeUpdateMetaData(
        mNativePtr, mNativeLifecycle, dataPtr, processorName, readOnly, data, globalPropsPtr);
  }

  private void setThemeInternal(LynxTheme theme) {
    if (theme == null) {
      return;
    }

    HashMap<String, HashMap> configMap = new HashMap<String, HashMap>();
    theme.addToHashMap(configMap, "theme");

    ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(configMap);
    if (buffer != null) {
      nativeUpdateConfig(mNativePtr, mNativeLifecycle, buffer, buffer.position());
    }
  }

  private void tryRegisterLynxEngineReused() {
    if (!mEnableReuseEngine) {
      return;
    }
    if (mLynxEngineRef != null) {
      // When registering into the cache, we do not remove the reference or actively detach it to
      // support on-demand processing.
      mLynxEngineRef.registerLynxEngineReused();
    }
  }

  void detachLynxEngineWrapper() {
    if (!mEnableReuseEngine) {
      return;
    }
    if (mLynxEngineRef != null) {
      if (mNativePtr != 0) {
        nativeDetachLynxEngineWrapper(mNativePtr, mNativeLifecycle);
      }
    }
  }

  public void setAttachLynxPageUICallback(UIBody.UIBodyView.attachLynxPageUICallback callback) {
    if (mLynxContext != null && mLynxContext.getLynxUIOwner() != null) {
      mLynxContext.getLynxUIOwner().setAttachLynxPageUICallback(callback);
    }
  }

  @CalledByNative
  private void getDataBack(ByteBuffer buffer, int tag) {
    LynxGetDataCallback callback = mCallbackSparseArray.get(tag);
    Object object = LepusBuffer.INSTANCE.decodeMessage(buffer);
    if (object instanceof Map) {
      callback.onSuccess(JavaOnlyMap.from(((Map) object)));
    } else {
      callback.onFail("LynxView GetData Failed");
    }
  }

  @CalledByNative
  private static Object decodeByteBuffer(ByteBuffer buffer) {
    if (buffer != null) {
      return LepusBuffer.INSTANCE.decodeMessage(buffer);
    }
    return null;
  }

  @RestrictTo({RestrictTo.Scope.LIBRARY})
  public void addRuntimeLifecycleListener(@NonNull RuntimeLifecycleListener listener) {
    if (null == listener || null == mJSProxy) {
      LLog.w(TAG, "add a null lifecycle listener or js proxy is null.");
      return;
    }
    mJSProxy.addLifecycleListener(listener);
  }

  public static class LogLynxViewClient extends LynxViewClient {
    private long mStartLoadTime = 0;

    @Override
    public void onPageStart(String url) {
      mStartLoadTime = System.currentTimeMillis();
    }

    @Override
    public void onLoadSuccess() {
      LLog.d(TAG, "onLoadSuccess time: " + (System.currentTimeMillis() - mStartLoadTime));
    }

    @Override
    public void onFirstScreen() {
      LLog.d(TAG, "onFirstScreen time: " + (System.currentTimeMillis() - mStartLoadTime));
    }

    @Override
    public void onPageUpdate() {
      LLog.d(TAG, "onPageUpdate time:" + (System.currentTimeMillis() - mStartLoadTime));
    }

    @Override
    public void onDataUpdated() {
      LLog.d(TAG, "onDataUpdated time:" + (System.currentTimeMillis() - mStartLoadTime));
    }
  }

  private static native long nativeCreate(long runtimePtr, Object nativeFacade,
      Object performanceController, Object loader, int renderStrategy,
      boolean enableLayoutSafePoint, boolean enableLayoutOnly, int screenWidth, int screenHeight,
      float density, String locale, boolean enableJSRuntime, boolean enableMultiAsyncThread,
      boolean enablePreUpdateData, boolean enableAutoConcurrency,
      boolean enableVSyncAlignedMessageLoop, boolean enableAsyncHydration,
      boolean enableJSGroupThread, String jsGroupThreadName, Object tasmPlatformInvoker,
      long whiteboard, long uiDelegate, boolean useInvokeUIMethod, boolean longTaskMonitorDisabled,
      boolean forceLayoutOnBackgroundThread, boolean enableUnifiedPipeline, int embeddedMode,
      long enginePtr);

  private static native void nativeDestroy(long ptr);

  private static native long nativeLifecycleCreate();

  private static native boolean nativeLifecycleTryTerminate(long lifecycle);

  private static native void nativeLifecycleDestroy(long lifecycle);

  private static native int nativeGetInstanceId(long ptr, long lifecycle);

  private static native void nativeAttachRuntime(long ptr, long lifecycle, long backgroundRuntime);

  private static native void nativeInitRuntime(long ptr, LynxResourceLoader resourceLoader,
      LynxModuleFactory moduleFactory, String groupId, String[] preloadJSPaths,
      String bytecodeSourceUrl, int runtimeFlags, long uiDelegate);

  private static native void nativeStartRuntime(long ptr, long lifecycle);

  private static native void nativeProcessRender(long ptr, long lifecycle);

  private static native void nativeSetEnableUIFlush(
      long ptr, long lifecycle, boolean enableUIFlush);

  private static native void nativeOnEnterForeground(long ptr, long lifecycle);

  private static native void nativeOnEnterBackground(long ptr, long lifecycle);

  // load template && component
  // FIXME(songshourui.null): only use templateData later
  private static native void nativeLoadSSRDataByPreParsedData(long ptr, long lifecycle, byte[] temp,
      long data, boolean readOnly, String processorName, TemplateData templateData);

  // FIXME(songshourui.null): only use templateData later
  private static native void nativeLoadTemplateByPreParsedData(long ptr, long lifecycle, String url,
      byte[] temp, int isPrePainting, boolean enableRecycleTemplateBundle, long data,
      boolean readOnly, String processorName, TemplateData templateData, ReadableMap timingOption);

  // FIXME(songshourui.null): only use templateData later
  private static native void nativeLoadTemplateBundleByPreParsedData(long ptr, long lifecycle,
      String url, long bundlePtr, int isPrePainting, long data, boolean readOnly,
      String processorName, TemplateData templateData, boolean enableDumpElementTree,
      ReadableMap timingOption);

  private static native void nativePreloadLazyBundles(long ptr, long lifecycle, String[] urls);

  private static native boolean nativeRegisterLazyBundle(
      long ptr, long lifecycle, String url, long bundlePtr);

  // update data
  // FIXME(songshourui.null): only use templateData later
  private static native void nativeUpdateDataByPreParsedData(long ptr, long lifecycle, long dataPtr,
      String processorName, boolean readOnly, TemplateData templateData);

  private static native void nativeUpdateMetaData(long ptr, long lifecycle, long dataPtr,
      String processorName, boolean readOnly, TemplateData templateData, long globalPropsPtr);

  // FIXME(songshourui.null): only use templateData later
  private static native void nativeResetDataByPreParsedData(long ptr, long lifecycle, long dataPtr,
      String processorName, boolean readOnly, TemplateData templateData);

  // FIXME(songshourui.null): only use templateData later
  private static native void nativeReloadTemplate(long ptr, long lifecycle, long dataPtr,
      long propPtr, String dataProcessorName, boolean dataReadOnly, Object globalProps,
      TemplateData templateData, ReadableMap timingOption);

  private static native void nativeUpdateConfig(
      long ptr, long lifecycle, ByteBuffer buffer, int length);

  private static native void nativeUpdateGlobalProps(long ptr, long lifecycle, long data);

  private static native void nativeUpdateScreenMetrics(
      long ptr, long lifecycle, int width, int height, float scale, long uiDelegate);

  private static native void nativeSetFontScale(long ptr, long lifecycle, float scale);

  private static native void nativeSetPlatformConfig(long ptr, long lifecycle, String jsonString);

  private static native void nativeUpdateFontScale(long ptr, long lifecycle, float scale);

  // layout
  private static native void nativeUpdateViewport(long ptr, long lifecycle, int width,
      int widthMode, int height, int heightMode, float scale, long uiDelegate);

  private static native void nativeSyncFetchLayoutResult(long ptr, long lifecycle);

  private static native void nativeEnforceRelayoutOnCurrentThreadWithUpdatedViewport(
      long ptr, long lifecycle, int width, int widthMode, int height, int heightMode);

  private static native void nativeSendGlobalEventToLepus(
      long ptr, long lifecycle, String name, ByteBuffer buffer, int length);

  private static native void nativeSendSsrGlobalEvent(
      long ptr, long lifecycle, String name, ByteBuffer buffer, int length);

  private static native void nativeTriggerEventBus(
      long ptr, long lifecycle, String name, ByteBuffer buffer, int length);

  // fetch data in native
  private native void nativeGetDataAsync(long ptr, long lifecycle, int tag);

  private static native Object nativeGetPageDataByKey(long ptr, long lifecycle, String[] keys);

  private static native JavaOnlyMap nativeGetAllJsSource(long ptr, long lifecycle);

  // list methods
  private static native void nativeRenderChild(
      long ptr, long lifecycle, int listSign, int index, long operationId);

  private static native void nativeUpdateChild(
      long ptr, long lifecycle, int listSign, int oldSign, int newIndex, long operationId);

  private static native void nativeRemoveChild(
      long ptr, long lifecycle, int listSign, int childSign);

  private static native int nativeObtainChild(long ptr, long lifecycle, int listSign, int index,
      long operationId, boolean enableReuseNotification);

  private static native void nativeRecycleChild(
      long ptr, long lifecycle, int listSign, int childSign);

  private static native void nativeObtainChildAsync(
      long ptr, long lifecycle, int listSign, int index, long operationId);

  private static native void nativeRecycleChildAsync(
      long ptr, long lifecycle, int listSign, int childSign);

  private native void nativeScrollByListContainer(
      long ptr, long lifecycle, int sign, float x, float y, float originalX, float originalY);

  private native void nativeScrollToPosition(
      long ptr, long lifecycle, int sign, int position, float offset, int align, boolean smooth);

  private native void nativeScrollStopped(long ptr, long lifecycle, int sign);

  private static native JavaOnlyMap nativeGetListPlatformInfo(
      long ptr, long lifecycle, int listSign);

  private static native void nativeUpdateI18nResource(
      long ptr, long lifecycle, String key, String bytes, int status);

  private static native void nativeMarkDirty(long ptr, long lifecycle);

  private static native void nativeFlush(long ptr, long lifecycle);

  private static native void nativeSyncPackageExternalPath(long ptr, String path);

  private static native void nativeSetEnableBytecode(
      long ptr, long lifecycle, boolean enable, String url);

  private static native void nativeDispatchMessageEvent(
      long ptr, long lifecycle, ReadableMap event);

  private native void nativeSetSSRTimingData(long ptr, long lifecycle, String url, long data_size);

  private native JavaOnlyMap nativeGetAllTimingInfo(long ptr, long lifecycle);

  private native void nativeClearPipelineTimingInfo(long ptr, long lifecycle);

  private native void nativeSetLongTaskMonitorDisabled(long ptr, long lifecycle, boolean disabled);

  private native void nativeSetSessionStorageItem(
      long ptr, long lifecycle, String key, long value, boolean readonly);

  private native void nativeGetSessionStorageItem(
      long ptr, long lifecycle, String key, PlatformCallBack callback);

  private native double nativeSubscribeSessionStorage(
      long ptr, long lifecycle, String key, PlatformCallBack callBack);

  private native void nativeUnsubscribeSessionStorage(
      long ptr, long lifecycle, String key, double id);

  private native void nativeAttachEngineToUIThread(long ptr, long lifecycle);

  private native void nativeDetachEngineFromUIThread(long ptr, long lifecycle);

  private native void nativeSetExtensionDelegate(long ptr, long lifecycle, long delegatePtr);

  private native void nativeSetContextHasAttached(long ptr, long lifecycle);

  public native void nativeDetachLynxEngineWrapper(long ptr, long lifecycle);
}
