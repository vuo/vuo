/**
 * @file
 * VuoKey implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>
#include <Carbon/Carbon.h>
#include "type.h"
#include "VuoKey.h"
#include "VuoList_VuoKey.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Key",
					  "dependencies" : [
						"VuoText",
						"VuoList_VuoKey",
						"Carbon.framework"
					  ],
				  });
#endif
/// @}

/**
 * @ingroup VuoKey
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoKey.
 */
VuoKey VuoKey_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoKey value = VuoKey_Any;

	if (! strcmp(valueAsString, "any")) {
		value = VuoKey_Any;
	} else if (! strcmp(valueAsString, "A")) {
		value = VuoKey_A;
	} else if (! strcmp(valueAsString, "S")) {
		value = VuoKey_S;
	} else if (! strcmp(valueAsString, "D")) {
		value = VuoKey_D;
	} else if (! strcmp(valueAsString, "F")) {
		value = VuoKey_F;
	} else if (! strcmp(valueAsString, "H")) {
		value = VuoKey_H;
	} else if (! strcmp(valueAsString, "G")) {
		value = VuoKey_G;
	} else if (! strcmp(valueAsString, "Z")) {
		value = VuoKey_Z;
	} else if (! strcmp(valueAsString, "X")) {
		value = VuoKey_X;
	} else if (! strcmp(valueAsString, "C")) {
		value = VuoKey_C;
	} else if (! strcmp(valueAsString, "V")) {
		value = VuoKey_V;
	} else if (! strcmp(valueAsString, "B")) {
		value = VuoKey_B;
	} else if (! strcmp(valueAsString, "Q")) {
		value = VuoKey_Q;
	} else if (! strcmp(valueAsString, "W")) {
		value = VuoKey_W;
	} else if (! strcmp(valueAsString, "E")) {
		value = VuoKey_E;
	} else if (! strcmp(valueAsString, "R")) {
		value = VuoKey_R;
	} else if (! strcmp(valueAsString, "Y")) {
		value = VuoKey_Y;
	} else if (! strcmp(valueAsString, "T")) {
		value = VuoKey_T;
	} else if (! strcmp(valueAsString, "1")) {
		value = VuoKey_1;
	} else if (! strcmp(valueAsString, "2")) {
		value = VuoKey_2;
	} else if (! strcmp(valueAsString, "3")) {
		value = VuoKey_3;
	} else if (! strcmp(valueAsString, "4")) {
		value = VuoKey_4;
	} else if (! strcmp(valueAsString, "6")) {
		value = VuoKey_6;
	} else if (! strcmp(valueAsString, "5")) {
		value = VuoKey_5;
	} else if (! strcmp(valueAsString, "Equal")) {
		value = VuoKey_Equal;
	} else if (! strcmp(valueAsString, "9")) {
		value = VuoKey_9;
	} else if (! strcmp(valueAsString, "7")) {
		value = VuoKey_7;
	} else if (! strcmp(valueAsString, "Minus")) {
		value = VuoKey_Minus;
	} else if (! strcmp(valueAsString, "8")) {
		value = VuoKey_8;
	} else if (! strcmp(valueAsString, "0")) {
		value = VuoKey_0;
	} else if (! strcmp(valueAsString, "RightBracket")) {
		value = VuoKey_RightBracket;
	} else if (! strcmp(valueAsString, "O")) {
		value = VuoKey_O;
	} else if (! strcmp(valueAsString, "U")) {
		value = VuoKey_U;
	} else if (! strcmp(valueAsString, "LeftBracket")) {
		value = VuoKey_LeftBracket;
	} else if (! strcmp(valueAsString, "I")) {
		value = VuoKey_I;
	} else if (! strcmp(valueAsString, "P")) {
		value = VuoKey_P;
	} else if (! strcmp(valueAsString, "L")) {
		value = VuoKey_L;
	} else if (! strcmp(valueAsString, "J")) {
		value = VuoKey_J;
	} else if (! strcmp(valueAsString, "Quote")) {
		value = VuoKey_Quote;
	} else if (! strcmp(valueAsString, "K")) {
		value = VuoKey_K;
	} else if (! strcmp(valueAsString, "Semicolon")) {
		value = VuoKey_Semicolon;
	} else if (! strcmp(valueAsString, "Backslash")) {
		value = VuoKey_Backslash;
	} else if (! strcmp(valueAsString, "Comma")) {
		value = VuoKey_Comma;
	} else if (! strcmp(valueAsString, "Slash")) {
		value = VuoKey_Slash;
	} else if (! strcmp(valueAsString, "N")) {
		value = VuoKey_N;
	} else if (! strcmp(valueAsString, "M")) {
		value = VuoKey_M;
	} else if (! strcmp(valueAsString, "Period")) {
		value = VuoKey_Period;
	} else if (! strcmp(valueAsString, "Grave")) {
		value = VuoKey_Grave;
	} else if (! strcmp(valueAsString, "KeypadDecimal")) {
		value = VuoKey_KeypadDecimal;
	} else if (! strcmp(valueAsString, "KeypadMultiply")) {
		value = VuoKey_KeypadMultiply;
	} else if (! strcmp(valueAsString, "KeypadPlus")) {
		value = VuoKey_KeypadPlus;
	} else if (! strcmp(valueAsString, "KeypadClear")) {
		value = VuoKey_KeypadClear;
	} else if (! strcmp(valueAsString, "KeypadDivide")) {
		value = VuoKey_KeypadDivide;
	} else if (! strcmp(valueAsString, "KeypadEnter")) {
		value = VuoKey_KeypadEnter;
	} else if (! strcmp(valueAsString, "KeypadMinus")) {
		value = VuoKey_KeypadMinus;
	} else if (! strcmp(valueAsString, "KeypadEquals")) {
		value = VuoKey_KeypadEquals;
	} else if (! strcmp(valueAsString, "Keypad0")) {
		value = VuoKey_Keypad0;
	} else if (! strcmp(valueAsString, "Keypad1")) {
		value = VuoKey_Keypad1;
	} else if (! strcmp(valueAsString, "Keypad2")) {
		value = VuoKey_Keypad2;
	} else if (! strcmp(valueAsString, "Keypad3")) {
		value = VuoKey_Keypad3;
	} else if (! strcmp(valueAsString, "Keypad4")) {
		value = VuoKey_Keypad4;
	} else if (! strcmp(valueAsString, "Keypad5")) {
		value = VuoKey_Keypad5;
	} else if (! strcmp(valueAsString, "Keypad6")) {
		value = VuoKey_Keypad6;
	} else if (! strcmp(valueAsString, "Keypad7")) {
		value = VuoKey_Keypad7;
	} else if (! strcmp(valueAsString, "Keypad8")) {
		value = VuoKey_Keypad8;
	} else if (! strcmp(valueAsString, "Keypad9")) {
		value = VuoKey_Keypad9;
	} else if (! strcmp(valueAsString, "Return")) {
		value = VuoKey_Return;
	} else if (! strcmp(valueAsString, "Tab")) {
		value = VuoKey_Tab;
	} else if (! strcmp(valueAsString, "Space")) {
		value = VuoKey_Space;
	} else if (! strcmp(valueAsString, "Delete")) {
		value = VuoKey_Delete;
	} else if (! strcmp(valueAsString, "Escape")) {
		value = VuoKey_Escape;
	} else if (! strcmp(valueAsString, "Command")) {
		value = VuoKey_Command;
	} else if (! strcmp(valueAsString, "Shift")) {
		value = VuoKey_Shift;
	} else if (! strcmp(valueAsString, "CapsLock")) {
		value = VuoKey_CapsLock;
	} else if (! strcmp(valueAsString, "Option")) {
		value = VuoKey_Option;
	} else if (! strcmp(valueAsString, "Control")) {
		value = VuoKey_Control;
	} else if (! strcmp(valueAsString, "RightShift")) {
		value = VuoKey_RightShift;
	} else if (! strcmp(valueAsString, "RightOption")) {
		value = VuoKey_RightOption;
	} else if (! strcmp(valueAsString, "RightControl")) {
		value = VuoKey_RightControl;
	} else if (! strcmp(valueAsString, "Function")) {
		value = VuoKey_Function;
	} else if (! strcmp(valueAsString, "F17")) {
		value = VuoKey_F17;
	} else if (! strcmp(valueAsString, "VolumeUp")) {
		value = VuoKey_VolumeUp;
	} else if (! strcmp(valueAsString, "VolumeDown")) {
		value = VuoKey_VolumeDown;
	} else if (! strcmp(valueAsString, "Mute")) {
		value = VuoKey_Mute;
	} else if (! strcmp(valueAsString, "F18")) {
		value = VuoKey_F18;
	} else if (! strcmp(valueAsString, "F19")) {
		value = VuoKey_F19;
	} else if (! strcmp(valueAsString, "F20")) {
		value = VuoKey_F20;
	} else if (! strcmp(valueAsString, "F5")) {
		value = VuoKey_F5;
	} else if (! strcmp(valueAsString, "F6")) {
		value = VuoKey_F6;
	} else if (! strcmp(valueAsString, "F7")) {
		value = VuoKey_F7;
	} else if (! strcmp(valueAsString, "F3")) {
		value = VuoKey_F3;
	} else if (! strcmp(valueAsString, "F8")) {
		value = VuoKey_F8;
	} else if (! strcmp(valueAsString, "F9")) {
		value = VuoKey_F9;
	} else if (! strcmp(valueAsString, "F11")) {
		value = VuoKey_F11;
	} else if (! strcmp(valueAsString, "F13")) {
		value = VuoKey_F13;
	} else if (! strcmp(valueAsString, "F16")) {
		value = VuoKey_F16;
	} else if (! strcmp(valueAsString, "F14")) {
		value = VuoKey_F14;
	} else if (! strcmp(valueAsString, "F10")) {
		value = VuoKey_F10;
	} else if (! strcmp(valueAsString, "F12")) {
		value = VuoKey_F12;
	} else if (! strcmp(valueAsString, "F15")) {
		value = VuoKey_F15;
	} else if (! strcmp(valueAsString, "Help")) {
		value = VuoKey_Help;
	} else if (! strcmp(valueAsString, "Home")) {
		value = VuoKey_Home;
	} else if (! strcmp(valueAsString, "PageUp")) {
		value = VuoKey_PageUp;
	} else if (! strcmp(valueAsString, "ForwardDelete")) {
		value = VuoKey_ForwardDelete;
	} else if (! strcmp(valueAsString, "F4")) {
		value = VuoKey_F4;
	} else if (! strcmp(valueAsString, "End")) {
		value = VuoKey_End;
	} else if (! strcmp(valueAsString, "F2")) {
		value = VuoKey_F2;
	} else if (! strcmp(valueAsString, "PageDown")) {
		value = VuoKey_PageDown;
	} else if (! strcmp(valueAsString, "F1")) {
		value = VuoKey_F1;
	} else if (! strcmp(valueAsString, "LeftArrow")) {
		value = VuoKey_LeftArrow;
	} else if (! strcmp(valueAsString, "RightArrow")) {
		value = VuoKey_RightArrow;
	} else if (! strcmp(valueAsString, "DownArrow")) {
		value = VuoKey_DownArrow;
	} else if (! strcmp(valueAsString, "UpArrow")) {
		value = VuoKey_UpArrow;
	} else if (! strcmp(valueAsString, "Yen")) {
		value = VuoKey_Yen;
	} else if (! strcmp(valueAsString, "Underscore")) {
		value = VuoKey_Underscore;
	} else if (! strcmp(valueAsString, "KeypadComma")) {
		value = VuoKey_KeypadComma;
	} else if (! strcmp(valueAsString, "Eisu")) {
		value = VuoKey_Eisu;
	} else if (! strcmp(valueAsString, "Kana")) {
		value = VuoKey_Kana;
	}

	return value;
}

/**
 * @ingroup VuoKey
 * Encodes @c value as a JSON object.
 */
json_object * VuoKey_getJson(const VuoKey value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoKey_Any:
			valueAsString = "any";
			break;
		case VuoKey_A:
			valueAsString = "A";
			break;
		case VuoKey_S:
			valueAsString = "S";
			break;
		case VuoKey_D:
			valueAsString = "D";
			break;
		case VuoKey_F:
			valueAsString = "F";
			break;
		case VuoKey_H:
			valueAsString = "H";
			break;
		case VuoKey_G:
			valueAsString = "G";
			break;
		case VuoKey_Z:
			valueAsString = "Z";
			break;
		case VuoKey_X:
			valueAsString = "X";
			break;
		case VuoKey_C:
			valueAsString = "C";
			break;
		case VuoKey_V:
			valueAsString = "V";
			break;
		case VuoKey_B:
			valueAsString = "B";
			break;
		case VuoKey_Q:
			valueAsString = "Q";
			break;
		case VuoKey_W:
			valueAsString = "W";
			break;
		case VuoKey_E:
			valueAsString = "E";
			break;
		case VuoKey_R:
			valueAsString = "R";
			break;
		case VuoKey_Y:
			valueAsString = "Y";
			break;
		case VuoKey_T:
			valueAsString = "T";
			break;
		case VuoKey_1:
			valueAsString = "1";
			break;
		case VuoKey_2:
			valueAsString = "2";
			break;
		case VuoKey_3:
			valueAsString = "3";
			break;
		case VuoKey_4:
			valueAsString = "4";
			break;
		case VuoKey_6:
			valueAsString = "6";
			break;
		case VuoKey_5:
			valueAsString = "5";
			break;
		case VuoKey_Equal:
			valueAsString = "Equal";
			break;
		case VuoKey_9:
			valueAsString = "9";
			break;
		case VuoKey_7:
			valueAsString = "7";
			break;
		case VuoKey_Minus:
			valueAsString = "Minus";
			break;
		case VuoKey_8:
			valueAsString = "8";
			break;
		case VuoKey_0:
			valueAsString = "0";
			break;
		case VuoKey_RightBracket:
			valueAsString = "RightBracket";
			break;
		case VuoKey_O:
			valueAsString = "O";
			break;
		case VuoKey_U:
			valueAsString = "U";
			break;
		case VuoKey_LeftBracket:
			valueAsString = "LeftBracket";
			break;
		case VuoKey_I:
			valueAsString = "I";
			break;
		case VuoKey_P:
			valueAsString = "P";
			break;
		case VuoKey_L:
			valueAsString = "L";
			break;
		case VuoKey_J:
			valueAsString = "J";
			break;
		case VuoKey_Quote:
			valueAsString = "Quote";
			break;
		case VuoKey_K:
			valueAsString = "K";
			break;
		case VuoKey_Semicolon:
			valueAsString = "Semicolon";
			break;
		case VuoKey_Backslash:
			valueAsString = "Backslash";
			break;
		case VuoKey_Comma:
			valueAsString = "Comma";
			break;
		case VuoKey_Slash:
			valueAsString = "Slash";
			break;
		case VuoKey_N:
			valueAsString = "N";
			break;
		case VuoKey_M:
			valueAsString = "M";
			break;
		case VuoKey_Period:
			valueAsString = "Period";
			break;
		case VuoKey_Grave:
			valueAsString = "Grave";
			break;
		case VuoKey_KeypadDecimal:
			valueAsString = "KeypadDecimal";
			break;
		case VuoKey_KeypadMultiply:
			valueAsString = "KeypadMultiply";
			break;
		case VuoKey_KeypadPlus:
			valueAsString = "KeypadPlus";
			break;
		case VuoKey_KeypadClear:
			valueAsString = "KeypadClear";
			break;
		case VuoKey_KeypadDivide:
			valueAsString = "KeypadDivide";
			break;
		case VuoKey_KeypadEnter:
			valueAsString = "KeypadEnter";
			break;
		case VuoKey_KeypadMinus:
			valueAsString = "KeypadMinus";
			break;
		case VuoKey_KeypadEquals:
			valueAsString = "KeypadEquals";
			break;
		case VuoKey_Keypad0:
			valueAsString = "Keypad0";
			break;
		case VuoKey_Keypad1:
			valueAsString = "Keypad1";
			break;
		case VuoKey_Keypad2:
			valueAsString = "Keypad2";
			break;
		case VuoKey_Keypad3:
			valueAsString = "Keypad3";
			break;
		case VuoKey_Keypad4:
			valueAsString = "Keypad4";
			break;
		case VuoKey_Keypad5:
			valueAsString = "Keypad5";
			break;
		case VuoKey_Keypad6:
			valueAsString = "Keypad6";
			break;
		case VuoKey_Keypad7:
			valueAsString = "Keypad7";
			break;
		case VuoKey_Keypad8:
			valueAsString = "Keypad8";
			break;
		case VuoKey_Keypad9:
			valueAsString = "Keypad9";
			break;
		case VuoKey_Return:
			valueAsString = "Return";
			break;
		case VuoKey_Tab:
			valueAsString = "Tab";
			break;
		case VuoKey_Space:
			valueAsString = "Space";
			break;
		case VuoKey_Delete:
			valueAsString = "Delete";
			break;
		case VuoKey_Escape:
			valueAsString = "Escape";
			break;
		case VuoKey_Command:
			valueAsString = "Command";
			break;
		case VuoKey_Shift:
			valueAsString = "Shift";
			break;
		case VuoKey_CapsLock:
			valueAsString = "CapsLock";
			break;
		case VuoKey_Option:
			valueAsString = "Option";
			break;
		case VuoKey_Control:
			valueAsString = "Control";
			break;
		case VuoKey_RightShift:
			valueAsString = "RightShift";
			break;
		case VuoKey_RightOption:
			valueAsString = "RightOption";
			break;
		case VuoKey_RightControl:
			valueAsString = "RightControl";
			break;
		case VuoKey_Function:
			valueAsString = "Function";
			break;
		case VuoKey_F17:
			valueAsString = "F17";
			break;
		case VuoKey_VolumeUp:
			valueAsString = "VolumeUp";
			break;
		case VuoKey_VolumeDown:
			valueAsString = "VolumeDown";
			break;
		case VuoKey_Mute:
			valueAsString = "Mute";
			break;
		case VuoKey_F18:
			valueAsString = "F18";
			break;
		case VuoKey_F19:
			valueAsString = "F19";
			break;
		case VuoKey_F20:
			valueAsString = "F20";
			break;
		case VuoKey_F5:
			valueAsString = "F5";
			break;
		case VuoKey_F6:
			valueAsString = "F6";
			break;
		case VuoKey_F7:
			valueAsString = "F7";
			break;
		case VuoKey_F3:
			valueAsString = "F3";
			break;
		case VuoKey_F8:
			valueAsString = "F8";
			break;
		case VuoKey_F9:
			valueAsString = "F9";
			break;
		case VuoKey_F11:
			valueAsString = "F11";
			break;
		case VuoKey_F13:
			valueAsString = "F13";
			break;
		case VuoKey_F16:
			valueAsString = "F16";
			break;
		case VuoKey_F14:
			valueAsString = "F14";
			break;
		case VuoKey_F10:
			valueAsString = "F10";
			break;
		case VuoKey_F12:
			valueAsString = "F12";
			break;
		case VuoKey_F15:
			valueAsString = "F15";
			break;
		case VuoKey_Help:
			valueAsString = "Help";
			break;
		case VuoKey_Home:
			valueAsString = "Home";
			break;
		case VuoKey_PageUp:
			valueAsString = "PageUp";
			break;
		case VuoKey_ForwardDelete:
			valueAsString = "ForwardDelete";
			break;
		case VuoKey_F4:
			valueAsString = "F4";
			break;
		case VuoKey_End:
			valueAsString = "End";
			break;
		case VuoKey_F2:
			valueAsString = "F2";
			break;
		case VuoKey_PageDown:
			valueAsString = "PageDown";
			break;
		case VuoKey_F1:
			valueAsString = "F1";
			break;
		case VuoKey_LeftArrow:
			valueAsString = "LeftArrow";
			break;
		case VuoKey_RightArrow:
			valueAsString = "RightArrow";
			break;
		case VuoKey_DownArrow:
			valueAsString = "DownArrow";
			break;
		case VuoKey_UpArrow:
			valueAsString = "UpArrow";
			break;
		case VuoKey_Yen:
			valueAsString = "Yen";
			break;
		case VuoKey_Underscore:
			valueAsString = "Underscore";
			break;
		case VuoKey_KeypadComma:
			valueAsString = "KeypadComma";
			break;
		case VuoKey_Eisu:
			valueAsString = "Eisu";
			break;
		case VuoKey_Kana:
			valueAsString = "Kana";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoKey VuoKey_getAllowedValues(void)
{
	VuoList_VuoKey l = VuoListCreate_VuoKey();
	for (VuoKey key = VuoKey_Any; key <= VuoKey_Kana; ++key)
		VuoListAppendValue_VuoKey(l, key);
	return l;
}

/**
 * @ingroup VuoKey
 * Returns a string representation of the key as it would be interpreted in the user's keyboard layout.
 */
char * VuoKey_getSummary(const VuoKey value)
{
	int keyCode;
	bool isVisibleChar = true;

	switch (value) {
		case VuoKey_A:
			keyCode = kVK_ANSI_A;
			break;
		case VuoKey_S:
			keyCode = kVK_ANSI_S;
			break;
		case VuoKey_D:
			keyCode = kVK_ANSI_D;
			break;
		case VuoKey_F:
			keyCode = kVK_ANSI_F;
			break;
		case VuoKey_H:
			keyCode = kVK_ANSI_H;
			break;
		case VuoKey_G:
			keyCode = kVK_ANSI_G;
			break;
		case VuoKey_Z:
			keyCode = kVK_ANSI_Z;
			break;
		case VuoKey_X:
			keyCode = kVK_ANSI_X;
			break;
		case VuoKey_C:
			keyCode = kVK_ANSI_C;
			break;
		case VuoKey_V:
			keyCode = kVK_ANSI_V;
			break;
		case VuoKey_B:
			keyCode = kVK_ANSI_B;
			break;
		case VuoKey_Q:
			keyCode = kVK_ANSI_Q;
			break;
		case VuoKey_W:
			keyCode = kVK_ANSI_W;
			break;
		case VuoKey_E:
			keyCode = kVK_ANSI_E;
			break;
		case VuoKey_R:
			keyCode = kVK_ANSI_R;
			break;
		case VuoKey_Y:
			keyCode = kVK_ANSI_Y;
			break;
		case VuoKey_T:
			keyCode = kVK_ANSI_T;
			break;
		case VuoKey_1:
			keyCode = kVK_ANSI_1;
			break;
		case VuoKey_2:
			keyCode = kVK_ANSI_2;
			break;
		case VuoKey_3:
			keyCode = kVK_ANSI_3;
			break;
		case VuoKey_4:
			keyCode = kVK_ANSI_4;
			break;
		case VuoKey_6:
			keyCode = kVK_ANSI_6;
			break;
		case VuoKey_5:
			keyCode = kVK_ANSI_5;
			break;
		case VuoKey_Equal:
			keyCode = kVK_ANSI_Equal;
			break;
		case VuoKey_9:
			keyCode = kVK_ANSI_9;
			break;
		case VuoKey_7:
			keyCode = kVK_ANSI_7;
			break;
		case VuoKey_Minus:
			keyCode = kVK_ANSI_Minus;
			break;
		case VuoKey_8:
			keyCode = kVK_ANSI_8;
			break;
		case VuoKey_0:
			keyCode = kVK_ANSI_0;
			break;
		case VuoKey_RightBracket:
			keyCode = kVK_ANSI_RightBracket;
			break;
		case VuoKey_O:
			keyCode = kVK_ANSI_O;
			break;
		case VuoKey_U:
			keyCode = kVK_ANSI_U;
			break;
		case VuoKey_LeftBracket:
			keyCode = kVK_ANSI_LeftBracket;
			break;
		case VuoKey_I:
			keyCode = kVK_ANSI_I;
			break;
		case VuoKey_P:
			keyCode = kVK_ANSI_P;
			break;
		case VuoKey_L:
			keyCode = kVK_ANSI_L;
			break;
		case VuoKey_J:
			keyCode = kVK_ANSI_J;
			break;
		case VuoKey_Quote:
			keyCode = kVK_ANSI_Quote;
			break;
		case VuoKey_K:
			keyCode = kVK_ANSI_K;
			break;
		case VuoKey_Semicolon:
			keyCode = kVK_ANSI_Semicolon;
			break;
		case VuoKey_Backslash:
			keyCode = kVK_ANSI_Backslash;
			break;
		case VuoKey_Comma:
			keyCode = kVK_ANSI_Comma;
			break;
		case VuoKey_Slash:
			keyCode = kVK_ANSI_Slash;
			break;
		case VuoKey_N:
			keyCode = kVK_ANSI_N;
			break;
		case VuoKey_M:
			keyCode = kVK_ANSI_M;
			break;
		case VuoKey_Period:
			keyCode = kVK_ANSI_Period;
			break;
		case VuoKey_Grave:
			keyCode = kVK_ANSI_Grave;
			break;
		case VuoKey_KeypadDecimal:
			keyCode = kVK_ANSI_KeypadDecimal;
			break;
		case VuoKey_KeypadMultiply:
			keyCode = kVK_ANSI_KeypadMultiply;
			break;
		case VuoKey_KeypadPlus:
			keyCode = kVK_ANSI_KeypadPlus;
			break;
		case VuoKey_KeypadClear:
			keyCode = kVK_ANSI_KeypadClear;
			break;
		case VuoKey_KeypadDivide:
			keyCode = kVK_ANSI_KeypadDivide;
			break;
		case VuoKey_KeypadEnter:
			keyCode = kVK_ANSI_KeypadEnter;
			break;
		case VuoKey_KeypadMinus:
			keyCode = kVK_ANSI_KeypadMinus;
			break;
		case VuoKey_KeypadEquals:
			keyCode = kVK_ANSI_KeypadEquals;
			break;
		case VuoKey_Keypad0:
			keyCode = kVK_ANSI_Keypad0;
			break;
		case VuoKey_Keypad1:
			keyCode = kVK_ANSI_Keypad1;
			break;
		case VuoKey_Keypad2:
			keyCode = kVK_ANSI_Keypad2;
			break;
		case VuoKey_Keypad3:
			keyCode = kVK_ANSI_Keypad3;
			break;
		case VuoKey_Keypad4:
			keyCode = kVK_ANSI_Keypad4;
			break;
		case VuoKey_Keypad5:
			keyCode = kVK_ANSI_Keypad5;
			break;
		case VuoKey_Keypad6:
			keyCode = kVK_ANSI_Keypad6;
			break;
		case VuoKey_Keypad7:
			keyCode = kVK_ANSI_Keypad7;
			break;
		case VuoKey_Keypad8:
			keyCode = kVK_ANSI_Keypad8;
			break;
		case VuoKey_Keypad9:
			keyCode = kVK_ANSI_Keypad9;
			break;
		case VuoKey_Yen:
			keyCode = kVK_JIS_Yen;
			break;
		case VuoKey_Underscore:
			keyCode = kVK_JIS_Underscore;
			break;
		case VuoKey_KeypadComma:
			keyCode = kVK_JIS_KeypadComma;
			break;
		case VuoKey_Eisu:
			keyCode = kVK_JIS_Eisu;
			break;
		case VuoKey_Kana:
			keyCode = kVK_JIS_Kana;
			break;
		default:
			// VuoKey_Any or corresponds to a position-independent virtual key code that doesn't type a visible character
			isVisibleChar = false;
	}

	char *summary = NULL;

	if (isVisibleChar)
	{
		UInt32 deadKeyState = 0;
		summary = VuoKey_getCharactersForMacVirtualKeyCode(keyCode, 0, &deadKeyState);

		if (!summary)
			isVisibleChar = false;
		else if (strlen(summary) == 0 || (strlen(summary) == 1 && summary[0] <= ' '))
		{
			// corresponds to a virtual key code that doesn't type a visible character on the user's keyboard
			isVisibleChar = false;
			free(summary);
		}
	}

	if (! isVisibleChar)
	{
		if (value == VuoKey_KeypadClear)
			summary = strdup("Keypad Clear");
		else if (value == VuoKey_KeypadEnter)
			summary = strdup("Keypad Enter");
		else if (value == VuoKey_KeypadComma)
			summary = strdup("Keypad ,");
		else
		{
			json_object *keyAsJson = VuoKey_getJson(value);
			const char *keyAsString = json_object_get_string(keyAsJson);
			summary = strdup(keyAsString);
			json_object_put(keyAsJson);
		}
	}

	if (isVisibleChar && ((VuoKey_KeypadDecimal <= value && value <= VuoKey_Keypad9) || value == VuoKey_KeypadComma))
	{
		const char *prefix = "Keypad ";
		char *tmp = malloc(strlen(summary) + strlen(prefix) + 1);
		strcpy(tmp, prefix);
		strcat(tmp, summary);
		free(summary);
		summary = tmp;
	}

	return summary;
}

/**
 * Returns the Unicode characters that would be typed by a key press.
 *
 * @param keyCode The virtual key code.
 * @param flags The event flags (of type @c CGEventFlags) for the key press.
 * @param deadKeyState To capture key combinations (e.g. Option-E-E for "é"), pass a reference to the same integer variable
 *		on consecutive calls to this function. Before the first call, you should initialize the variable to 0.
 * @return One or more UTF8-encoded characters.
 */
char * VuoKey_getCharactersForMacVirtualKeyCode(unsigned short keyCode, unsigned long flags, unsigned int *deadKeyState)
{
	// http://stackoverflow.com/questions/22566665/how-to-capture-unicode-from-key-events-without-an-nstextview
	// http://stackoverflow.com/questions/12547007/convert-key-code-into-key-equivalent-string
	// http://stackoverflow.com/questions/8263618/convert-virtual-key-code-to-unicode-string

	TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
	CFDataRef layoutData = TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
	if (!layoutData)
		return NULL;
	const UCKeyboardLayout *keyboardLayout = (const UCKeyboardLayout *)CFDataGetBytePtr(layoutData);

	UInt32 modifierKeyState = (flags >> 16) & 0xFF;

	const UniCharCount unicodeStringLength = 4;
	UniChar unicodeString[unicodeStringLength];
	UniCharCount realLength;

	UCKeyTranslate(keyboardLayout,
				   keyCode,
				   kUCKeyActionDown,
				   modifierKeyState,
				   LMGetKbdType(),
				   0,
				   deadKeyState,
				   unicodeStringLength,
				   &realLength,
				   unicodeString);
	CFRelease(currentKeyboard);
	CFStringRef cfString = CFStringCreateWithCharacters(kCFAllocatorDefault, unicodeString, realLength);

	VuoText t = VuoText_makeFromCFString(cfString);
	VuoRetain(t);
	char *characters = strdup(t);
	VuoRelease(t);
	CFRelease(cfString);
	return characters;
}

/**
 * Returns the VuoKey that corresponds to the given virtual key code.
 */
VuoKey VuoKey_makeFromMacVirtualKeyCode(unsigned short keyCode)
{
	switch (keyCode) {
		case kVK_ANSI_A:
			return VuoKey_A;
		case kVK_ANSI_S:
			return VuoKey_S;
		case kVK_ANSI_D:
			return VuoKey_D;
		case kVK_ANSI_F:
			return VuoKey_F;
		case kVK_ANSI_H:
			return VuoKey_H;
		case kVK_ANSI_G:
			return VuoKey_G;
		case kVK_ANSI_Z:
			return VuoKey_Z;
		case kVK_ANSI_X:
			return VuoKey_X;
		case kVK_ANSI_C:
			return VuoKey_C;
		case kVK_ANSI_V:
			return VuoKey_V;
		case kVK_ANSI_B:
			return VuoKey_B;
		case kVK_ANSI_Q:
			return VuoKey_Q;
		case kVK_ANSI_W:
			return VuoKey_W;
		case kVK_ANSI_E:
			return VuoKey_E;
		case kVK_ANSI_R:
			return VuoKey_R;
		case kVK_ANSI_Y:
			return VuoKey_Y;
		case kVK_ANSI_T:
			return VuoKey_T;
		case kVK_ANSI_1:
			return VuoKey_1;
		case kVK_ANSI_2:
			return VuoKey_2;
		case kVK_ANSI_3:
			return VuoKey_3;
		case kVK_ANSI_4:
			return VuoKey_4;
		case kVK_ANSI_6:
			return VuoKey_6;
		case kVK_ANSI_5:
			return VuoKey_5;
		case kVK_ANSI_Equal:
			return VuoKey_Equal;
		case kVK_ANSI_9:
			return VuoKey_9;
		case kVK_ANSI_7:
			return VuoKey_7;
		case kVK_ANSI_Minus:
			return VuoKey_Minus;
		case kVK_ANSI_8:
			return VuoKey_8;
		case kVK_ANSI_0:
			return VuoKey_0;
		case kVK_ANSI_RightBracket:
			return VuoKey_RightBracket;
		case kVK_ANSI_O:
			return VuoKey_O;
		case kVK_ANSI_U:
			return VuoKey_U;
		case kVK_ANSI_LeftBracket:
			return VuoKey_LeftBracket;
		case kVK_ANSI_I:
			return VuoKey_I;
		case kVK_ANSI_P:
			return VuoKey_P;
		case kVK_ANSI_L:
			return VuoKey_L;
		case kVK_ANSI_J:
			return VuoKey_J;
		case kVK_ANSI_Quote:
			return VuoKey_Quote;
		case kVK_ANSI_K:
			return VuoKey_K;
		case kVK_ANSI_Semicolon:
			return VuoKey_Semicolon;
		case kVK_ANSI_Backslash:
			return VuoKey_Backslash;
		case kVK_ANSI_Comma:
			return VuoKey_Comma;
		case kVK_ANSI_Slash:
			return VuoKey_Slash;
		case kVK_ANSI_N:
			return VuoKey_N;
		case kVK_ANSI_M:
			return VuoKey_M;
		case kVK_ANSI_Period:
			return VuoKey_Period;
		case kVK_ANSI_Grave:
			return VuoKey_Grave;
		case kVK_ANSI_KeypadDecimal:
			return VuoKey_KeypadDecimal;
		case kVK_ANSI_KeypadMultiply:
			return VuoKey_KeypadMultiply;
		case kVK_ANSI_KeypadPlus:
			return VuoKey_KeypadPlus;
		case kVK_ANSI_KeypadClear:
			return VuoKey_KeypadClear;
		case kVK_ANSI_KeypadDivide:
			return VuoKey_KeypadDivide;
		case kVK_ANSI_KeypadEnter:
			return VuoKey_KeypadEnter;
		case kVK_ANSI_KeypadMinus:
			return VuoKey_KeypadMinus;
		case kVK_ANSI_KeypadEquals:
			return VuoKey_KeypadEquals;
		case kVK_ANSI_Keypad0:
			return VuoKey_Keypad0;
		case kVK_ANSI_Keypad1:
			return VuoKey_Keypad1;
		case kVK_ANSI_Keypad2:
			return VuoKey_Keypad2;
		case kVK_ANSI_Keypad3:
			return VuoKey_Keypad3;
		case kVK_ANSI_Keypad4:
			return VuoKey_Keypad4;
		case kVK_ANSI_Keypad5:
			return VuoKey_Keypad5;
		case kVK_ANSI_Keypad6:
			return VuoKey_Keypad6;
		case kVK_ANSI_Keypad7:
			return VuoKey_Keypad7;
		case kVK_ANSI_Keypad8:
			return VuoKey_Keypad8;
		case kVK_ANSI_Keypad9:
			return VuoKey_Keypad9;
		case kVK_Return:
			return VuoKey_Return;
		case kVK_Tab:
			return VuoKey_Tab;
		case kVK_Space:
			return VuoKey_Space;
		case kVK_Delete:
			return VuoKey_Delete;
		case kVK_Escape:
			return VuoKey_Escape;
		case kVK_Command:
			return VuoKey_Command;
		case kVK_Shift:
			return VuoKey_Shift;
		case kVK_CapsLock:
			return VuoKey_CapsLock;
		case kVK_Option:
			return VuoKey_Option;
		case kVK_Control:
			return VuoKey_Control;
		case kVK_RightShift:
			return VuoKey_RightShift;
		case kVK_RightOption:
			return VuoKey_RightOption;
		case kVK_RightControl:
			return VuoKey_RightControl;
		case kVK_Function:
			return VuoKey_Function;
		case kVK_F17:
			return VuoKey_F17;
		case kVK_VolumeUp:
			return VuoKey_VolumeUp;
		case kVK_VolumeDown:
			return VuoKey_VolumeDown;
		case kVK_Mute:
			return VuoKey_Mute;
		case kVK_F18:
			return VuoKey_F18;
		case kVK_F19:
			return VuoKey_F19;
		case kVK_F20:
			return VuoKey_F20;
		case kVK_F5:
			return VuoKey_F5;
		case kVK_F6:
			return VuoKey_F6;
		case kVK_F7:
			return VuoKey_F7;
		case kVK_F3:
			return VuoKey_F3;
		case kVK_F8:
			return VuoKey_F8;
		case kVK_F9:
			return VuoKey_F9;
		case kVK_F11:
			return VuoKey_F11;
		case kVK_F13:
			return VuoKey_F13;
		case kVK_F16:
			return VuoKey_F16;
		case kVK_F14:
			return VuoKey_F14;
		case kVK_F10:
			return VuoKey_F10;
		case kVK_F12:
			return VuoKey_F12;
		case kVK_F15:
			return VuoKey_F15;
		case kVK_Help:
			return VuoKey_Help;
		case kVK_Home:
			return VuoKey_Home;
		case kVK_PageUp:
			return VuoKey_PageUp;
		case kVK_ForwardDelete:
			return VuoKey_ForwardDelete;
		case kVK_F4:
			return VuoKey_F4;
		case kVK_End:
			return VuoKey_End;
		case kVK_F2:
			return VuoKey_F2;
		case kVK_PageDown:
			return VuoKey_PageDown;
		case kVK_F1:
			return VuoKey_F1;
		case kVK_LeftArrow:
			return VuoKey_LeftArrow;
		case kVK_RightArrow:
			return VuoKey_RightArrow;
		case kVK_DownArrow:
			return VuoKey_DownArrow;
		case kVK_UpArrow:
			return VuoKey_UpArrow;
		case kVK_JIS_Yen:
			return VuoKey_Yen;
		case kVK_JIS_Underscore:
			return VuoKey_Underscore;
		case kVK_JIS_KeypadComma:
			return VuoKey_KeypadComma;
		case kVK_JIS_Eisu:
			return VuoKey_Eisu;
		case kVK_JIS_Kana:
			return VuoKey_Kana;
	}

	return VuoKey_Any;
}

/**
 * Returns true if the virtual key code matches the given key.
 */
bool VuoKey_doesMacVirtualKeyCodeMatch(unsigned short keyCode, VuoKey key)
{
	if (key == VuoKey_Any)
		return true;

	return (key == VuoKey_makeFromMacVirtualKeyCode(keyCode));
}
