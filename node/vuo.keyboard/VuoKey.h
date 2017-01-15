/**
 * @file
 * VuoKey C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOKEY_H
#define VUOKEY_H

/// @{
typedef const struct VuoList_VuoKey_struct { void *l; } * VuoList_VuoKey;
#define VuoList_VuoKey_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoKey VuoKey
 * A key position on the keyboard. For example, the top left letter key on an English keyboard ('Q') and
 * the top left letter key on a French keyboard ('A') would have the same VuoKey value.
 *
 * @{
 */

/**
 * Each value of this type represents a key position on the keyboard.
 */
typedef enum {
	VuoKey_Any,
	VuoKey_A,
	VuoKey_S,
	VuoKey_D,
	VuoKey_F,
	VuoKey_H,
	VuoKey_G,
	VuoKey_Z,
	VuoKey_X,
	VuoKey_C,
	VuoKey_V,
	VuoKey_B,
	VuoKey_Q,
	VuoKey_W,
	VuoKey_E,
	VuoKey_R,
	VuoKey_Y,
	VuoKey_T,
	VuoKey_1,
	VuoKey_2,
	VuoKey_3,
	VuoKey_4,
	VuoKey_6,
	VuoKey_5,
	VuoKey_Equal,
	VuoKey_9,
	VuoKey_7,
	VuoKey_Minus,
	VuoKey_8,
	VuoKey_0,
	VuoKey_RightBracket,
	VuoKey_O,
	VuoKey_U,
	VuoKey_LeftBracket,
	VuoKey_I,
	VuoKey_P,
	VuoKey_L,
	VuoKey_J,
	VuoKey_Quote,
	VuoKey_K,
	VuoKey_Semicolon,
	VuoKey_Backslash,
	VuoKey_Comma,
	VuoKey_Slash,
	VuoKey_N,
	VuoKey_M,
	VuoKey_Period,
	VuoKey_Grave,
	VuoKey_KeypadDecimal,
	VuoKey_KeypadMultiply,
	VuoKey_KeypadPlus,
	VuoKey_KeypadClear,
	VuoKey_KeypadDivide,
	VuoKey_KeypadEnter,
	VuoKey_KeypadMinus,
	VuoKey_KeypadEquals,
	VuoKey_Keypad0,
	VuoKey_Keypad1,
	VuoKey_Keypad2,
	VuoKey_Keypad3,
	VuoKey_Keypad4,
	VuoKey_Keypad5,
	VuoKey_Keypad6,
	VuoKey_Keypad7,
	VuoKey_Keypad8,
	VuoKey_Keypad9,
	VuoKey_Return,
	VuoKey_Tab,
	VuoKey_Space,
	VuoKey_Delete,
	VuoKey_Escape,
	VuoKey_Command,
	VuoKey_Shift,
	VuoKey_CapsLock,
	VuoKey_Option,
	VuoKey_Control,
	VuoKey_RightShift,
	VuoKey_RightOption,
	VuoKey_RightControl,
	VuoKey_Function,
	VuoKey_F17,
	VuoKey_VolumeUp,
	VuoKey_VolumeDown,
	VuoKey_Mute,
	VuoKey_F18,
	VuoKey_F19,
	VuoKey_F20,
	VuoKey_F5,
	VuoKey_F6,
	VuoKey_F7,
	VuoKey_F3,
	VuoKey_F8,
	VuoKey_F9,
	VuoKey_F11,
	VuoKey_F13,
	VuoKey_F16,
	VuoKey_F14,
	VuoKey_F10,
	VuoKey_F12,
	VuoKey_F15,
	VuoKey_Help,
	VuoKey_Home,
	VuoKey_PageUp,
	VuoKey_ForwardDelete,
	VuoKey_F4,
	VuoKey_End,
	VuoKey_F2,
	VuoKey_PageDown,
	VuoKey_F1,
	VuoKey_LeftArrow,
	VuoKey_RightArrow,
	VuoKey_DownArrow,
	VuoKey_UpArrow,
	VuoKey_Yen,
	VuoKey_Underscore,
	VuoKey_KeypadComma,
	VuoKey_Eisu,
	VuoKey_Kana
} VuoKey;

VuoKey VuoKey_makeFromJson(struct json_object * js);
struct json_object * VuoKey_getJson(const VuoKey value);
VuoList_VuoKey VuoKey_getAllowedValues(void);
char * VuoKey_getSummary(const VuoKey value);

char * VuoKey_getCharactersForMacVirtualKeyCode(unsigned short keyCode, unsigned long flags, unsigned int *deadKeyState);
VuoKey VuoKey_makeFromMacVirtualKeyCode(unsigned short keyCode);
bool VuoKey_doesMacVirtualKeyCodeMatch(unsigned short keyCode, VuoKey key);

/// @{
/**
 * Automatically generated function.
 */
VuoKey VuoKey_makeFromString(const char *str);
char * VuoKey_getString(const VuoKey value);
/// @}

/**
 * @}
*/

#endif
