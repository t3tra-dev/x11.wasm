declare const LibraryManager: {
  library: Record<string, unknown>;
};

declare function mergeInto(
  target: Record<string, unknown>,
  source: Record<string, unknown>,
): void;

interface X11WasmResource {
  kind: "root" | "window" | "pixmap";
  mapped: boolean;
  pendingMapped: boolean;
  x: number;
  y: number;
  width: number;
  height: number;
  pendingX: number;
  pendingY: number;
  pendingWidth: number;
  pendingHeight: number;
  borderWidth?: number;
  pendingBorderWidth?: number;
  element: HTMLElement | null;
  canvas?: HTMLCanvasElement | null;
  windowClass?: number;
  title?: string;
  iconName?: string;
  transientFor?: number;
  command?: string;
  clientMachine?: string;
  wmClassName?: string;
  wmClass?: string;
  alpha?: number;
  fullscreen?: boolean;
  topmost?: boolean;
  zoomed?: boolean;
  managerGeometryOwned?: boolean;
  wmState?: "" | "normal" | "iconic" | "withdrawn";
  wmHints?: X11WasmWmHints | null;
  iconPhotoCount?: number;
  iconPhotoSizes?: string;
  iconPhotoHref?: string;
  iconPhotoWidth?: number;
  iconPhotoHeight?: number;
  iconSizeCount?: number;
  iconSizes?: string;
  protocols?: string[];
  windowTypes?: string[];
  closeRequestCount?: number;
  lastCloseRequestTime?: number;
  closeRequestStatus?: string;
  colormapWindows?: string[];
  standardColormaps?: Record<string, unknown[]>;
  sizeHints?: X11WasmSizeHints | null;
  zoomHints?: X11WasmSizeHints | null;
}

interface X11WasmSizeHints {
  flags: number;
  x: number;
  y: number;
  width: number;
  height: number;
  minWidth: number;
  minHeight: number;
  maxWidth: number;
  maxHeight: number;
  widthInc: number;
  heightInc: number;
  minAspectX: number;
  minAspectY: number;
  maxAspectX: number;
  maxAspectY: number;
  baseWidth: number;
  baseHeight: number;
  winGravity: number;
}

interface X11WasmWmHints {
  flags: number;
  input: number;
  initialState: number;
  iconPixmap: number;
  iconWindow: number;
  iconX: number;
  iconY: number;
  iconMask: number;
  windowGroup: number;
}

interface X11WasmHostAccessEntry {
  family: number;
  length: number;
  address: string;
}

interface X11WasmClipRect {
  x: number;
  y: number;
  width: number;
  height: number;
}

interface X11WasmPointerWarp {
  count: number;
  rootX: number;
  rootY: number;
  targetWindow: number;
  time: number;
}

interface X11WasmSelectionOwner {
  owner: number;
  time: number;
}

interface X11WasmPreeditStyle {
  context: number;
  hasSpot?: boolean;
  spotX: number;
  spotY: number;
  fontPixelSize: number;
  fontFamily: string;
  foreground: number;
  background: number;
}

interface X11WasmBackend {
  windowManagerEnabled: boolean;
  setWindowManagerEnabled(enabled: boolean): void;
  resources: Map<number, X11WasmResource>;
  dirtyResources: Set<X11WasmResource>;
  flushScheduled: boolean;
  rootWindow: number;
  requestSerial: number;
  flushSerial: number;
  syncSerial: number;
  synchronizeEnabled: boolean;
  focusedWindow: number;
  inputContextFocused: boolean;
  inputContextWindow: number;
  inputContextHasSpot: boolean;
  inputContextSpotX: number;
  inputContextSpotY: number;
  pointerWindow: number;
  pointerState: number;
  pointerCaptureWindow: number;
  pointerWarpCount: number;
  lastWarpPointer: X11WasmPointerWarp | null;
  screenSaverResetCount: number;
  lastScreenSaverMode: number;
  lastUserActivityTime: number;
  screenSaverTimeout: number;
  screenSaverInterval: number;
  screenSaverPreferBlanking: number;
  screenSaverAllowExposures: number;
  screenSaverSuspended: boolean;
  screenWakeLockStatus: string;
  screenWakeLockError: string;
  screenWakeLockType: string;
  fullscreenRequestStatus: string;
  fullscreenRequestError: string;
  fullscreenRequestWindow: number;
  fullscreenRequestCount: number;
  fullscreenExitCount: number;
  fullscreenChangeCount: number;
  fullscreenErrorCount: number;
  recordUserActivity(): void;
  getUserIdleTime(): number;
  hostAccessEnabled: boolean;
  hostAccessList: string;
  hostAccessHosts: X11WasmHostAccessEntry[];
  serverGrabDepth: number;
  pointerGrabWindow: number;
  pointerGrabOwnerEvents: boolean;
  pointerGrabEventMask: number;
  pointerGrabConfineWindow: number;
  pointerGrabCursorShape: number;
  pointerGrabPointerMode: number;
  pointerGrabKeyboardMode: number;
  keyboardGrabWindow: number;
  keyboardGrabOwnerEvents: boolean;
  keyboardGrabPointerMode: number;
  keyboardGrabKeyboardMode: number;
  clipboardReadStatus: string;
  clipboardReadError: string;
  clipboardReadCount: number;
  selectionOwners: Map<string, X11WasmSelectionOwner>;
  flush(): void;
  syncExternalClipboardText(): Promise<number>;
  createWindow(
    id: number,
    parent: number,
    x: number,
    y: number,
    width: number,
    height: number,
    depth: number,
    eventMask: number,
    windowClass?: number,
  ): number;
  createPixmap(
    id: number,
    drawable: number,
    width: number,
    height: number,
    depth: number,
  ): number;
  mapWindow(id: number): number;
  unmapWindow(id: number): number;
  configureWindow(
    id: number,
    x: number,
    y: number,
    width: number,
    height: number,
    borderWidth: number,
    valueMask: number,
  ): number;
  setWindowAttributes(...args: unknown[]): number;
  setWindowStyle(...args: unknown[]): number;
  setPointerGrabCursor(enabled: boolean, cursor: number): number;
  clearWindow(id: number): number;
  clearWindowArea(
    id: number,
    x: number,
    y: number,
    width: number,
    height: number,
  ): number;
  setClipRect(
    enabled: boolean,
    x: number,
    y: number,
    width: number,
    height: number,
  ): number;
  setClipRects(enabled: boolean, rects: X11WasmClipRect[]): number;
  setDrawStyle(...args: unknown[]): number;
  drawRectangle(...args: unknown[]): number;
  drawLine(...args: unknown[]): number;
  fillPolygon(...args: unknown[]): number;
  drawText(...args: unknown[]): number;
  copyArea(...args: unknown[]): number;
  copyPlane(...args: unknown[]): number;
  putImage(...args: unknown[]): number;
  getImage(...args: unknown[]): number[];
  setWindowWMHints(
    id: number,
    flags: number,
    input: number,
    initialState: number,
    iconPixmap: number,
    iconWindow: number,
    iconX: number,
    iconY: number,
    iconMask: number,
    windowGroup: number,
  ): number;
  setScreenSaverSettings(
    timeout: number,
    interval: number,
    preferBlanking: number,
    allowExposures: number,
  ): number;
  setHostAccessControl(
    enabled: boolean,
    hosts: string,
    count: number,
  ): number;
  setServerGrabState(depth: number): number;
  cancelComposition(window?: number): number;
  setInputContext(
    focused: boolean,
    window: number,
    hasSpot: boolean,
    spotRootX: number,
    spotRootY: number,
    preedit?: X11WasmPreeditStyle,
  ): number;
  setRequestState(
    requestSerial: number,
    flushSerial: number,
    syncSerial: number,
  ): number;
  setSynchronizeState(enabled: boolean): number;
  setPointerGrabState(
    enabled: boolean,
    window: number,
    ownerEvents: boolean,
    eventMask: number,
    confineWindow: number,
    cursor: number,
    pointerMode: number,
    keyboardMode: number,
  ): number;
  setKeyboardGrabState(
    enabled: boolean,
    window: number,
    ownerEvents: boolean,
    pointerMode: number,
    keyboardMode: number,
  ): number;
  setPointerState(
    rootX: number,
    rootY: number,
    window: number,
    state: number,
    captureWindow: number,
  ): number;
  setSelectionOwner(selection: string, owner: number, time: number): number;
  clearWindowMetadata(id: number, updatesDocument?: boolean): number;
  setWindowStandardColormaps(
    id: number,
    property: string,
    maps: string,
    count: number,
  ): number;
  setWindowProtocols(id: number, protocols: string): number;
  setWindowTypes(id: number, types: string): number;
  dispatchWindowProtocol(id: number, protocol: string, time?: number): number;
  requestWindowClose(id: number, time?: number): number;
  setWindowIconSizes(id: number, sizes: string, count: number): number;
  setWindowWmState(id: number, state: number): number;
  setWindowState(
    id: number,
    topmost: number,
    zoomed: number,
    fullscreen: number,
  ): number;
  setWindowSizeHints(...args: unknown[]): number;
  setWindowZoomHints(
    id: number,
    flags: number,
    x: number,
    y: number,
    width: number,
    height: number,
    minWidth: number,
    minHeight: number,
    maxWidth: number,
    maxHeight: number,
    widthInc: number,
    heightInc: number,
    minAspectX: number,
    minAspectY: number,
    maxAspectX: number,
    maxAspectY: number,
    baseWidth: number,
    baseHeight: number,
    winGravity: number,
  ): number;
}

type X11WasmBackendSnapshot = X11WasmBackend;

declare const Module: {
  x11WasmBackend?: X11WasmBackend;
  x11WasmCreateBackend?: (module: typeof Module) => unknown;
  x11WasmBridgeDiagnostics?: {
    missingBackendCount: number;
    missingMethodCount: number;
    lastMissingMethod: string;
    lastMissingReason: string;
    missingMethods: Record<string, number>;
    warnedMissing: Record<string, boolean>;
    requiredMethods: string[];
    missingRequiredMethods: string[];
    backendComplete: boolean;
    backendValidationCount: number;
  };
  x11WasmRoot?: string | HTMLElement;
  x11WasmWindowManager?: boolean;
  _X11WasmResizeRoot?: (width: number, height: number) => number;
  _X11WasmManagerWindowIdentity?: (window: number, field: number) => number;
  _X11WasmManagerCloseWindow?: (window: number, time: number) => number;
  _X11WasmManagerActivateWindow?: (window: number, time: number) => number;
  /** 0 normal: remove maximized atoms; 1 iconic: preserve zoom; 2 zoomed: add maximized atoms. Geometry is unchanged. */
  _X11WasmManagerSetWindowState?: (window: number, state: 0 | 1 | 2, time: number) => number;
  /** WM validation only; no snapping. Rejected requests leave geometry unchanged. */
  _X11WasmManagerConfigureWindow?: (window: number, x: number, y: number,
    width: number, height: number, mask: number) => number;
  _X11WasmConfigureWindow?: (
    window: number,
    x: number,
    y: number,
    width: number,
    height: number,
    valueMask: number,
  ) => number;
  _X11WasmQueueKeyEvent?: (
    type: number,
    window: number,
    keySym: number,
    state: number,
    time: number,
  ) => number;
  _X11WasmQueueKeyEventFields?: (
    type: number,
    window: number,
    keyCode: number,
    keySym: number,
    state: number,
    x: number,
    y: number,
    time: number,
  ) => number;
  _X11WasmQueueIMKeyEventFields?: (
    type: number,
    window: number,
    keyCode: number,
    state: number,
    x: number,
    y: number,
    time: number,
  ) => number;
  _X11WasmQueueTextEvent?: (
    window: number,
    codepoint: number,
    state: number,
    time: number,
  ) => number;
  _X11WasmQueueTextEventFields?: (
    type: number,
    window: number,
    codepoint: number,
    state: number,
    time: number,
  ) => number;
  _X11WasmQueueTextEventFieldsAtWindow?: (
    type: number,
    window: number,
    codepoint: number,
    state: number,
    time: number,
  ) => number;
  _X11WasmQueueCompositionEvent?: (
    phase: number,
    window: number,
    utf8: number,
    utf8Length: number,
  ) => number;
  _X11WasmSetExternalClipboardText?: (
    utf8: number,
    utf8Length: number,
  ) => number;
  _X11WasmQueueWindowProtocol?: (
    window: number,
    protocolUtf8: number,
    protocolUtf8Length: number,
    time: number,
  ) => number;
  _X11WasmQueueFullscreenState?: (
    window: number,
    fullscreen: number,
    time: number,
  ) => number;
  _X11WasmQueuePointerEvent?: (
    type: number,
    window: number,
    button: number,
    state: number,
    x: number,
    y: number,
    time: number,
  ) => number;
  _X11WasmQueueCrossingEvent?: (
    type: number,
    window: number,
    state: number,
    x: number,
    y: number,
    time: number,
  ) => number;
  _X11WasmQueueFocusEvent?: (
    type: number,
    window: number,
    time: number,
  ) => number;
  _X11WasmQueueWheelEvent?: (
    window: number,
    delta: number,
    state: number,
    x: number,
    y: number,
    time: number,
  ) => number;
  _X11WasmTestSetWindowIconSizes?: (
    window: number,
    enabled: number,
  ) => number;
  _X11WasmTestSetRootIconSizeAndWindowIcon?: () => number;
  _X11WasmTestSetScreenSaverSettings?: (enabled: number) => number;
  _X11WasmTestForceScreenSaverState?: () => number;
  _X11WasmTestSetScreenSaverWakeLock?: (enabled: number) => number;
  _X11WasmTestSuspendScreenSaver?: (suspended: number) => number;
  _X11WasmTestScreenSaverRegistration?: () => number;
  _X11WasmTestSetHostAccessControl?: (configured: number) => number;
  _X11WasmTestSetServerGrabDepth?: (depth: number) => number;
  _X11WasmTestSetPrimarySelectionOwner?: (
    window: number,
    enabled: number,
  ) => number;
  _X11WasmTestSetClipboardText?: (clear: number) => number;
  _X11WasmTestSyncClipboardSelectionProperty?: () => number;
  _X11WasmTestSetStandardColormaps?: (
    window: number,
    enabled: number,
  ) => number;
  _X11WasmTestSetZoomHints?: (window: number, enabled: number) => number;
  _X11WasmTestSetWindowMetadataProperties?: (
    window: number,
    enabled: number,
  ) => number;
  _X11WasmTestRotateWindowNameProperties?: (window: number) => number;
  _X11WasmTestGetWindowNamePropertiesDeletes?: (window: number) => number;
  _X11WasmTestGetWindowMetadataPropertiesDeletes?: (
    window: number,
  ) => number;
  _X11WasmTestGetWindowWMHintsDeletes?: (
    window: number,
    iconWindow: number,
    groupWindow: number,
  ) => number;
  _X11WasmTestGetWindowNormalHintsDeletes?: (window: number) => number;
  _X11WasmTestGetWindowCollectionPropertiesDeletes?: (
    window: number,
    colormapWindow: number,
  ) => number;
  _X11WasmTestGetWindowEwmhPropertiesDeletes?: (window: number) => number;
  _X11WasmTestSetWindowMetadataTargetProperty?: (
    window: number,
    target: number,
    isRootToplevel: number,
  ) => number;
  _X11WasmTestGetWindowMetadataTargetDeletes?: (
    window: number,
    target: number,
  ) => number;
  _X11WasmTestSendNetWmStateMessage?: (
    window: number,
    action: number,
    firstState: number,
    secondState: number,
  ) => number;
  _X11WasmTestMapWindowWithInitialState?: (initialState: number) => number;
  _XGrabPointer?: (
    display: number,
    grabWindow: number,
    ownerEvents: number,
    eventMask: number,
    pointerMode: number,
    keyboardMode: number,
    confineTo: number,
    cursor: number,
    time: number,
  ) => number;
  _XUngrabPointer?: (display: number, time: number) => number;
  _XGrabKeyboard?: (
    display: number,
    window: number,
    ownerEvents: number,
    pointerMode: number,
    keyboardMode: number,
    time: number,
  ) => number;
  _XUngrabKeyboard?: (display: number, time: number) => number;
  _XSetIconSizes?: (
    display: number,
    window: number,
    sizeList: number,
    count: number,
  ) => number;
  _XGetErrorText?: (
    display: number,
    code: number,
    buffer: number,
    length: number,
  ) => number;
  _XGetErrorDatabaseText?: (
    display: number,
    name: number,
    message: number,
    defaultString: number,
    buffer: number,
    length: number,
  ) => number;
  _XQueryExtension?: (
    display: number,
    name: number,
    majorOpcodeReturn: number,
    firstEventReturn: number,
    firstErrorReturn: number,
  ) => number;
  _XListExtensions?: (display: number, nextensionsReturn: number) => number;
  _X11WasmTestUnixEventFileHandlers?: () => number;
  _X11WasmTestWheelEventConstants?: () => number;
  _X11WasmTestCompositionEventPayload?: () => number;
  _X11WasmTestEventQueueAndKeycodeApis?: () => number;
  _X11WasmTestConfigureDomPointerMapping?: (
    window: number,
    remapped: number,
  ) => number;
  _X11WasmTestReadDomPointerMappingEvents?: (window: number) => number;
  _X11WasmTestClearAreaApis?: () => number;
  _X11WasmTestConfigureStackingApis?: () => number;
  _X11WasmTestConfigureRequestRedirectApis?: () => number;
  _X11WasmTestCirculateRequestRedirectApis?: () => number;
  _X11WasmTestResizeRequestRedirectApis?: () => number;
  _X11WasmTestVisibilityObscuredApis?: () => number;
  _X11WasmTestExposeNewlyVisibleApis?: () => number;
  _X11WasmTestSubstructureNotifyApis?: () => number;
  _X11WasmTestMapViewableExposeApis?: () => number;
  _X11WasmTestMapRequestRedirectApis?: () => number;
  _X11WasmTestXlibApis?: () => number;
  _X11WasmTestColormapApis?: () => number;
  _X11WasmTestBitmapFileApis?: () => number;
  _X11WasmTestResourceManagerApis?: () => number;
  _X11WasmTestXSendEventTargets?: (
    focusWindow: number,
    pointerWindow: number,
  ) => number;
  _X11WasmTestOpenDisplayNames?: () => number;
  _X11WasmTestXICLookupFocus?: () => number;
  _X11WasmTestXErrorHandler?: () => number;
  _malloc?: (size: number) => number;
  _free?: (pointer: number) => void;
};

declare function UTF8ToString(pointer: number, maxBytesToRead?: number): string;

declare const HEAPU8: Uint8Array;
declare const HEAP16: Int16Array;
declare const HEAPU16: Uint16Array;
declare const HEAP32: Int32Array;

declare const X11WasmBridge: any;
declare function X11WasmDecodeTextRun(utf8: number, length: number): {
  text: string;
  offsets: number[];
} | null;

interface Window {
  Module?: typeof Module;
}

declare var X11WasmDOM: {
  createBackend(options?: {
    root?: string | HTMLElement;
    document?: Document;
    module?: typeof Module;
    windowManager?: boolean;
  }): unknown;
  X11WasmDOMBackend: unknown;
};
