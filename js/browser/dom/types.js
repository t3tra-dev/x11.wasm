// @ts-check
/*
 * types.js --
 *
 *	Shared JSDoc typedefs for the x11.wasm DOM backend shards. Names
 *	use the X11WasmDOM prefix to avoid colliding with public test and
 *	downstream interfaces from js/types/.
 */

/** @typedef {"root" | "window" | "pixmap"} X11WasmDOMResourceKind */
/** @typedef {"" | "normal" | "iconic" | "withdrawn"} X11WasmDOMWmState */

/**
 * @typedef {object} X11WasmDOMDamage
 * @property {number} x
 * @property {number} y
 * @property {number} width
 * @property {number} height
 */

/**
 * @typedef {object} X11WasmDOMClipRect
 * @property {number} x
 * @property {number} y
 * @property {number} width
 * @property {number} height
 */

/**
 * @typedef {object} X11WasmDOMClipMask
 * @property {number} pixmap
 * @property {number} xOrigin
 * @property {number} yOrigin
 */

/**
 * @typedef {object} X11WasmDOMClipMaskSnapshot
 * @property {Uint8ClampedArray} data
 * @property {number} width
 * @property {number} height
 * @property {number} xOrigin
 * @property {number} yOrigin
 * @property {boolean} hasBits
 */

/**
 * @typedef {object} X11WasmDOMSizeHints
 * @property {number} flags
 * @property {number} x
 * @property {number} y
 * @property {number} width
 * @property {number} height
 * @property {number} minWidth
 * @property {number} minHeight
 * @property {number} maxWidth
 * @property {number} maxHeight
 * @property {number} widthInc
 * @property {number} heightInc
 * @property {number} minAspectX
 * @property {number} minAspectY
 * @property {number} maxAspectX
 * @property {number} maxAspectY
 * @property {number} baseWidth
 * @property {number} baseHeight
 * @property {number} winGravity
 */

/**
 * @typedef {object} X11WasmDOMIconBitmap
 * @property {string} href
 * @property {number} width
 * @property {number} height
 */

/**
 * @typedef {object} X11WasmDOMWmHints
 * @property {number} flags
 * @property {number} input
 * @property {number} initialState
 * @property {number} iconPixmap
 * @property {number} iconWindow
 * @property {number} iconX
 * @property {number} iconY
 * @property {number} iconMask
 * @property {number} windowGroup
 */

/**
 * @typedef {object} X11WasmDOMHostAccessEntry
 * @property {number} family
 * @property {number} length
 * @property {string} address
 */

/**
 * @typedef {object} X11WasmDOMResource
 * @property {number} id
 * @property {X11WasmDOMResourceKind} kind
 * @property {number} parent
 * @property {number} x
 * @property {number} y
 * @property {number} width
 * @property {number} height
 * @property {number} borderWidth
 * @property {number} pendingX
 * @property {number} pendingY
 * @property {number} pendingWidth
 * @property {number} pendingHeight
 * @property {number} pendingBorderWidth
 * @property {number} depth
 * @property {number} windowClass
 * @property {number} eventMask
 * @property {number} doNotPropagateMask
 * @property {number} colormap
 * @property {boolean} overrideRedirect
 * @property {boolean} saveUnder
 * @property {number} bitGravity
 * @property {number} winGravity
 * @property {number} backingStore
 * @property {number} backingPlanes
 * @property {number} backingPixel
 * @property {boolean} mapped
 * @property {boolean} pendingMapped
 * @property {boolean} geometryDirty
 * @property {boolean=} nativeGeometry
 * @property {boolean=} managerGeometryOwned
 * @property {boolean=} syncingNativeGeometry
 * @property {{ x: number, y: number, width: number, height: number }=} normalGeometry
 * @property {boolean} dirty
 * @property {X11WasmDOMDamage | null} damage
 * @property {number} damageSerial
 * @property {HTMLElement | null} element
 * @property {HTMLCanvasElement | OffscreenCanvas | null} canvas
 * @property {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D | null} context
 * @property {string | null} backgroundColor
 * @property {number} backgroundPixmap
 * @property {string | null} borderColor
 * @property {number} borderPixmap
 * @property {string | null} cursor
 * @property {string} title
 * @property {string} iconName
 * @property {number} transientFor
 * @property {string} command
 * @property {string} clientMachine
 * @property {string} wmClassName
 * @property {string} wmClass
 * @property {number} alpha
 * @property {boolean} topmost
 * @property {boolean} zoomed
 * @property {boolean} fullscreen
 * @property {X11WasmDOMWmState} wmState
 * @property {X11WasmDOMWmHints | null} wmHints
 * @property {number} iconPhotoCount
 * @property {string} iconPhotoSizes
 * @property {string} iconPhotoHref
 * @property {number} iconPhotoWidth
 * @property {number} iconPhotoHeight
 * @property {string} iconBitmapHref
 * @property {number} iconBitmapWidth
 * @property {number} iconBitmapHeight
 * @property {number} iconSizeCount
 * @property {string} iconSizes
 * @property {string[]} protocols
 * @property {string[]} windowTypes
 * @property {number} closeRequestCount
 * @property {number} lastCloseRequestTime
 * @property {string} closeRequestStatus
 * @property {boolean} closeEventListenerInstalled
	   * @property {string[]} colormapWindows
	   * @property {Record<string, unknown[]>} standardColormaps
	   * @property {X11WasmDOMSizeHints | null} sizeHints
	   * @property {X11WasmDOMSizeHints | null} zoomHints
	   */

/**
 * @typedef {object} X11WasmDOMDrawStyle
 * @property {number} functionId
 * @property {number} lineWidth
 * @property {number} lineStyle
 * @property {number} capStyle
 * @property {number} joinStyle
 * @property {number} dashOffset
 * @property {number} dashLength
 * @property {number[]} dashPattern
 * @property {number} fillStyle
 * @property {number} stipple
 * @property {number} tile
 * @property {number} tsOriginX
 * @property {number} tsOriginY
 * @property {number} backgroundPixel
 * @property {number} fontPixelSize
 * @property {string} fontFamily
 * @property {number} planeMask
 */

/**
 * @typedef {object} X11WasmDOMPreeditStyle
 * @property {number} context
 * @property {boolean=} hasSpot
 * @property {number} spotX
 * @property {number} spotY
 * @property {number} fontPixelSize
 * @property {string} fontFamily
 * @property {number} foreground
 * @property {number} background
 */

/**
 * @typedef {object} X11WasmDOMBackendOptions
 * @property {string | HTMLElement=} root
 * @property {Document=} document
 * @property {any=} module
 * @property {boolean=} windowManager
 */

/**
 * @typedef {object} X11WasmDOMManagerIdentity
 * @property {number} frameId
 * @property {number} metadataId
 * @property {number} protocolId
 */

/**
 * @typedef {object} X11WasmDOMManagerFrame
 * @property {X11WasmDOMManagerIdentity} identity
 * @property {HTMLElement} element
 * @property {HTMLElement} titlebar
 * @property {HTMLElement} title
 * @property {HTMLImageElement} icon
 * @property {HTMLButtonElement} close
 * @property {HTMLButtonElement} minimize
 * @property {HTMLButtonElement} maximize
 * @property {HTMLElement[]} handles
 * @property {boolean=} lastZoomed
 * @property {boolean=} lastFullscreen
 * @property {string=} lastBounds
 * @property {string=} lastGeometry
 */

/**
 * @typedef {object} X11WasmDOMManagerGesture
 * @property {number} id
 * @property {number} pointerId
 * @property {HTMLElement} capture
 * @property {string} edge
 * @property {number} startX
 * @property {number} startY
 * @property {number} x
 * @property {number} y
 * @property {number} width
 * @property {number} height
 * @property {string=} lastCandidate
 */
