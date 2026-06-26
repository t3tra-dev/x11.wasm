/*
 * saver.h --
 *
 *	Minimal MIT-SCREEN-SAVER protocol constants for x11.wasm.
 */

#ifndef _SAVER_H_
#define _SAVER_H_

#define ScreenSaverName "MIT-SCREEN-SAVER"
#define ScreenSaverPropertyName "_MIT_SCREEN_SAVER_ID"

#define ScreenSaverNotifyMask 0x00000001
#define ScreenSaverCycleMask 0x00000002

#define ScreenSaverMajorVersion 1
#define ScreenSaverMinorVersion 1

#define ScreenSaverOff 0
#define ScreenSaverOn 1
#define ScreenSaverCycle 2
#define ScreenSaverDisabled 3

#define ScreenSaverBlanked 0
#define ScreenSaverInternal 1
#define ScreenSaverExternal 2

#define ScreenSaverNotify 0
#define ScreenSaverNumberEvents 1

#endif /* _SAVER_H_ */
