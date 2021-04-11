/**
 * @file
 * Implementation of stb_textedit functions.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoText.h"

/**
 * The type of a single character.
 */
#ifndef STB_TEXTEDIT_CHARTYPE
#define STB_TEXTEDIT_CHARTYPE uint64_t
#endif

#include <stdlib.h>
#include "VuoHeap.h"
#include "VuoImageText.h"
#include "stb_textedit.h"
#include "VuoClipboard.h"
#include "VuoModifierKey.h"

#define COMMAND_HI ((((uint64_t)VuoModifierKey_Command) << 32))					///< Convert VuoModifierKey_Command to high 32 bits of 64 unsigned int
#define CONTROL_HI ((((uint64_t)VuoModifierKey_Control) << 32))					///< Convert Control to high 32 bits of 64 unsigned int
#define OPTION_HI ((((uint64_t)VuoModifierKey_Option) << 32))					///< Convert VuoModifierKey_Option to high 32 bits of 64 unsigned int
#define SHIFT_HI ((((uint64_t)VuoModifierKey_Shift) << 32))						///< Convert VuoModifierKey_Shift to high 32 bits of 64 unsigned int

#define KEYCODE_COPY 0x3 	///< The keycode to recognize as a "Copy" event.
#define KEYCODE_PASTE 0x16 	///< The keycode to recognize as a "Paste" event.
#define KEYCODE_CUT 0x18 	///< The keycode to recognize as a "Cut" event.
#define KEYCODE_ESC 0x1b 	///< The keycode to recognize as a "Escape" event.
#define KEYCODE_TAB 0x9		///< The keycode to recognize as `Tab`
#define KEYCODE_RETURN 0xD  ///< The keycode to recognize as `Return` (above right shift key)
#define KEYCODE_ENTER 0x3   ///< The keycode to recognize as `Enter` (fn-return, or button on the bottom-right of an extended keyboard)

/// The character representing a newline. See also VuoTextEdit_isNewLine.
static char STB_TEXTEDIT_NEWLINE = '\n';

#define STB_TEXTEDIT_K_LEFT            0x0001C                                     ///< Keyboard input to move cursor left
#define STB_TEXTEDIT_K_RIGHT           0x0001D                                     ///< Keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP              0x0001E                                     ///< Keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN            0x0001F                                     ///< Keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART       (COMMAND_HI | STB_TEXTEDIT_K_LEFT)          ///< Keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINESTART2      0x1                                         ///< Keyboard input to move cursor to start of line (home key)
#define STB_TEXTEDIT_K_LINEEND         (COMMAND_HI | STB_TEXTEDIT_K_RIGHT)         ///< Keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_LINEEND2        0x4                                         ///< Keyboard input to move cursor to end of line (end key)
#define STB_TEXTEDIT_K_TEXTSTART       (COMMAND_HI | STB_TEXTEDIT_K_UP)            ///< Keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTSTART2      (CONTROL_HI | 0x41)                         ///< Keyboard input to move cursor to start of text (Control+A)
#define STB_TEXTEDIT_K_TEXTEND         (COMMAND_HI | STB_TEXTEDIT_K_DOWN)          ///< Keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_TEXTEND2        (CONTROL_HI | 0x45)                         ///< Keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE          0x7f                                        ///< Keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE       0x00008                                     ///< Keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO            (COMMAND_HI | 0x1A)                         ///< Keyboard input to perform undo
#define STB_TEXTEDIT_K_REDO            (COMMAND_HI | 0x19)                         ///< Keyboard input to perform redo
#define STB_TEXTEDIT_K_REDO2           ((COMMAND_HI | SHIFT_HI) | 0x1A)            ///< Keyboard input to perform redo
#define STB_TEXTEDIT_K_WORDLEFT        (OPTION_HI | STB_TEXTEDIT_K_LEFT)           ///< Keyboard input to move cursor left one word
#define STB_TEXTEDIT_K_WORDRIGHT       (OPTION_HI | STB_TEXTEDIT_K_RIGHT)          ///< Keyboard input to move cursor right one word
#define STB_TEXTEDIT_K_DELETEWORD      (OPTION_HI | STB_TEXTEDIT_K_BACKSPACE)      ///< Keyboard input to delete the characters to the left of the cursor up to the first whitespace
#define STB_TEXTEDIT_K_DELETEWORDRIGHT (OPTION_HI | STB_TEXTEDIT_K_DELETE)         ///< Keyboard input to delete the characters to the right of the cursor up to the first whitespace
#define STB_TEXTEDIT_K_DELETELINE      (COMMAND_HI | STB_TEXTEDIT_K_BACKSPACE)     ///< Keyboard input to delete the characters to the left of the cursor up to the first line break.
#define STB_TEXTEDIT_K_DELETELINERIGHT (COMMAND_HI | STB_TEXTEDIT_K_DELETE)        ///< Keyboard input to delete the characters to the right of the cursor up to the first line break.
#define STB_TEXTEDIT_K_SHIFT           SHIFT_HI                                    ///< Keyboard input to signify shift modifier.
#define STB_TEXTEDIT_K_SELECTALL       (COMMAND_HI | 0x1)                          ///< Keyboard input to select all text.

/// The width of a newline character.
#define STB_TEXTEDIT_GETWIDTH_NEWLINE 0

/**
 * The type of object representing a string being edited, typically this is a wrapper object with other data you need
 */
typedef VuoText STB_TEXTEDIT_STRING;

/**
 * Pack a UTF32 char and VuoModifierKey into a Uint64.
 */
static STB_TEXTEDIT_CHARTYPE VuoTextEdit_combineKeyAndModifier(uint32_t utf32, VuoModifierKey modifiers)
{
	return ((uint64_t)utf32) | (((uint64_t)modifiers) << 32);
}

/**
 * Convert a VuoText string to an array of STB_TEXTEDIT_CHARTYPE.  The length of the resulting
 * array is stored in length.  No modifier keys are encoded.
 */
static STB_TEXTEDIT_CHARTYPE* VuoTextEdit_makeCharArrayWithVuoText(VuoText text, size_t* length)
{
	uint32_t* utf32 = VuoText_getUtf32Values(text, length);
	STB_TEXTEDIT_CHARTYPE* stbchars = (STB_TEXTEDIT_CHARTYPE*) malloc(sizeof(STB_TEXTEDIT_CHARTYPE) * (*length));
	for(int i = 0; i < (*length); i++)
		stbchars[i] = utf32[i];
	free(utf32);
	return stbchars;
}

/**
 * The length of the string.
 */
static int STB_TEXTEDIT_STRINGLEN(const STB_TEXTEDIT_STRING* obj)
{
	return VuoText_length(*obj);
}

/**
 * Tests if a UTF32 value is considered a new line character.
 */
static bool VuoTextEdit_isNewLine(uint32_t utf32_char)
{
	return  utf32_char >= 0xA && utf32_char <= 0xD;
}

/**
 * Tests if a UTF32 value is considered white space.
 */
static bool VuoTextEdit_isWhiteSpace(uint32_t utf32_char)
{
	// control code or one of those weird white space chars between 0x2000-A
	if( utf32_char < 33 || (utf32_char >= 0x2000 && utf32_char <= 0x200A) )
		return true;

	return false;
}

/**
 * 	Tests if a UTF32 value is considered a word separator.  Used to calculate movement stops (ex: alt+left).
 *	Separators: /\()"'-:,.;<>~!@#$%^&*|+=[]{}`~?
 */
static bool VuoTextEdit_isSeparator(uint32_t utf32_char)
{
	// control code or one of those weird white space chars between 0x2000-A
	if( VuoTextEdit_isWhiteSpace(utf32_char) )
		return true;

	size_t length = 0;

	uint32_t* separators = VuoText_getUtf32Values("./\\()\"'-:,;<>~!@#$%%^&*|+=[]{}`~?", &length);

	for(size_t i = 0; i < length; i++)
	{
		if(utf32_char == separators[i])
		{
			free(separators);
			return true;
		}
	}

	free(separators);

	return false;
}

/**
 *	Forward declaration stb_textedit_prep_selection_at_cursor.
 */
static void stb_textedit_prep_selection_at_cursor(STB_TexteditState *state);

/**
 *	Selects the all text to the left and right of cursor up to any character
 *	matched by separatorFunc.
 */
static void VuoTextEdit_selectFromCursorToSeparator(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, bool (*separatorFunc)(uint32_t))
{
	if(VuoText_isEmpty(*str))
		return;

	stb_textedit_prep_selection_at_cursor(state);

	size_t len = 0;
	uint32_t* chars = VuoText_getUtf32Values(*str, &len);
	// select_start can be greater if placeholder text is present
	int start = MIN(len, state->select_start);
	state->select_start = 0;

	for(int i = start; i > 0; i--)
	{
		if( (*separatorFunc)(chars[i]) )
		{
			state->select_start = i + 1;
			break;
		}
	}

	int prev_end = state->select_end;
	state->select_end = len;
	state->cursor = len;

	for(int i = prev_end; i < len; i++)
	{
		if( (*separatorFunc)(chars[i]) )
		{
			state->select_end = i;
			state->cursor = i;
			break;
		}
	}
}

/**
 * Copy the current text selection to the clipboard.
 */
static void VuoTextEdit_copy(STB_TEXTEDIT_STRING *str, STB_TexteditState *state)
{
	int start = MIN(state->select_start, state->select_end);
	int end = MAX(state->select_start, state->select_end);
	VuoText selection = VuoText_substring(*str, start + 1, end - start);
	VuoLocal(selection);
	VuoClipboard_setText(selection);
}

/**
 * Forward declaration of cut function.
 */
static int stb_textedit_cut(STB_TEXTEDIT_STRING *str, STB_TexteditState *state);

/**
 * Perform a text cut operation (copy selected characters to clipboard and delete).
 */
static int VuoTextEdit_cut(STB_TEXTEDIT_STRING *str, STB_TexteditState *state)
{
	if( state->select_start != state->select_end )
	{
		VuoTextEdit_copy(str, state);
		stb_textedit_cut(str, state);
		return 1;
	}
	return 0;
}

/**
 *	Forward declaration of stb_textedit_paste.
 */
static int stb_textedit_paste(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, STB_TEXTEDIT_CHARTYPE const *ctext, int len);

/**
 * returns the i'th character of obj, 0-based
 */
static STB_TEXTEDIT_CHARTYPE STB_TEXTEDIT_GETCHAR(const STB_TEXTEDIT_STRING* obj, int idx)
{
	if(idx < 0)
		return -1;

	// @todo It's inefficient to keep allocating a full utf32 array for a single char
	size_t length;

	uint32_t* utf32 = VuoText_getUtf32Values(*obj, &length);

	if(idx >= length)
		return -1;

	return (STB_TEXTEDIT_CHARTYPE) utf32[idx];
}

/**
 * Return the number of new line characters in a string. Note that this may not match textData->lineCount because textData
 * only counts lines that have a non-whitespace character present (or is followed by another line).
 */
static int stb_textedit_linecount(STB_TEXTEDIT_STRING* str)
{
	int count = 1;

	for(int ii = 0; ii < STB_TEXTEDIT_STRINGLEN(str); ii++)
		if( STB_TEXTEDIT_GETCHAR(str, ii) == STB_TEXTEDIT_NEWLINE)
			count++;

	return count;
}

/**
 * Remove any text past maxLines.
 */
static VuoText VuoTextEdit_truncateLines(VuoText text, VuoInteger maxLines)
{
	VuoList_VuoInteger newLines = VuoText_findOccurrences(text, "\n");
	VuoLocal(newLines);

	VuoText res;

	if(VuoListGetCount_VuoInteger(newLines) >= maxLines)
	{
		VuoInteger trunc = VuoListGetValue_VuoInteger(newLines, maxLines);
		// Subtract 1 because we don't want the \n
		res = VuoText_substring(text, 1, trunc - 1);
	}
	else
	{
		res = VuoText_make(text);
	}

	return res;
}

/**
 * Perform a text paste operation (copy text from clipboard to current cursor position).
 */
static void VuoTextEdit_paste(STB_TEXTEDIT_STRING *str, STB_TexteditState *state)
{
	VuoText clipboard = VuoClipboard_getContents();
	VuoLocal(clipboard);

	int lineCount = stb_textedit_linecount(str);
	// +1 because line_count is a count of \n, not lines.
 	VuoText res = VuoTextEdit_truncateLines(clipboard, (state->line_count - lineCount) + 1);
 	VuoLocal(res);

	size_t len = 0;
	uint64_t* clip64 = VuoTextEdit_makeCharArrayWithVuoText(res, &len);
	stb_textedit_paste(str, state, clip64, len);
	free(clip64);
}

/**
 * Overrides stb_textedit implementation
 */
static int stb_text_locate_coord(STB_TexteditState *state, float x, float y)
{
	if (!state->textImageData)
		return 0;

	return VuoImageTextData_getNearestCharToPoint((VuoImageTextData) state->textImageData, VuoPoint2d_make(x, y));
}

/**
 * Find the x/y location of a character, and remember info about the previous row in
 * case we get a move-up event (for page up, we'll have to rescan).
 */
static void stb_textedit_find_charpos(StbFindState* find, STB_TEXTEDIT_STRING *str, int n, STB_TexteditState* state)
{
	VuoImageTextData data = (VuoImageTextData)state->textImageData;

	unsigned int lineIndex = 0;
	VuoPoint2d position = VuoImageTextData_getPositionForCharIndex(data, n, &lineIndex);

	find->x = position.x;
	find->y = position.y;
	find->height = data->lineHeight;
	find->first_char = VuoImageTextData_getCharIndexForLine(data, lineIndex);
	find->length = data->lineCounts[lineIndex];
	find->prev_first = VuoImageTextData_getCharIndexForLine(data, MAX(0, ((int)lineIndex) - 1));
}

/**
 * Given a character index, find the line it resides in.
 */
static int VuoTextEdit_findLineIndex(int charIndex, STB_TexteditState* state)
{
	VuoImageTextData data = (VuoImageTextData) state->textImageData;
	unsigned int lineIndex = 0;
	VuoImageTextData_getPositionForCharIndex(data, charIndex, &lineIndex);
	return VuoImageTextData_getCharIndexForLine(data, lineIndex);
}

/**
 * maps a keyboard input to an insertable character (return type is int, 0 means not valid to insert)
 */
static uint32_t STB_TEXTEDIT_KEYTOTEXT(STB_TEXTEDIT_CHARTYPE key)
{
	// fprintf(stderr, "raw: %llu (%llu) hi: %llu  lo: %u\n", key, (OPTION_HI | 0xd), key & VuoModifierKey_Any, (uint32_t) ((key & ~VuoModifierKey_Any) & 0xFFFF));

	uint32_t lo = (uint32_t) (key & 0xFFFF);

	if (lo == KEYCODE_RETURN)
		return STB_TEXTEDIT_NEWLINE;
	else if (lo == KEYCODE_TAB)
		return '\t';

	return lo > 31 && lo < UINT32_MAX ? lo : 0;
}

/// Tests if a UTF32 value is considered whitespace.
#define STB_TEXTEDIT_IS_SPACE(x) VuoTextEdit_isSeparator(((uint32_t)x))

/// Tests if a UTF32 value is considered a word separator.
#define STB_TEXTEDIT_IS_SEPARATOR(x) VuoTextEdit_isSeparator(((uint32_t)x))

/**
 * Returns the pixel delta from the xpos of the i'th character
 * to the xpos of the i+1'th char for a line of characters
 * starting at character `n` (i.e. accounts for kerning
 * with previous char)

 */
static float STB_TEXTEDIT_GETWIDTH(STB_TEXTEDIT_STRING* obj, int n, int i, STB_TexteditState* state)
{
	VuoImageTextData data = (VuoImageTextData) state->textImageData;
	return data->charAdvance[n + i];
}

/**
 * Lay out the text in rows.
 */
static void STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, STB_TEXTEDIT_STRING* obj, int line_start_idx, STB_TexteditState* state)
{
	unsigned int len = 1;
	VuoRectangle rect = VuoImageTextData_layoutRowAtIndex((VuoImageTextData) state->textImageData, line_start_idx, &len);
	r->x0 = rect.center.x - (rect.size.x * .5);
	r->x1 = rect.size.x;
	r->baseline_y_delta = rect.size.y;
	r->ymin = rect.center.y - (rect.size.y * .5f);
	r->ymax = rect.size.y;
	r->num_chars = len;
}

/**
 *  Delete n characters starting at i
 */
static void STB_TEXTEDIT_DELETECHARS(STB_TEXTEDIT_STRING* obj, int pos, int n)
{
	VuoText old = *obj;
	*obj = VuoText_removeAt(old, pos + 1, n);
	VuoRetain(*obj);
	VuoRelease(old);
}

/**
 * insert n characters at i (pointed to by STB_TEXTEDIT_CHARTYPE*)
 */
static bool STB_TEXTEDIT_INSERTCHARS(STB_TEXTEDIT_STRING* obj, int pos, const STB_TEXTEDIT_CHARTYPE* new_text, int new_text_len)
{
	uint32_t new_text_32[new_text_len];

	for(int i = 0; i < new_text_len; i++)
		new_text_32[i] = (uint32_t) (new_text[i] & 0xFFFF);

	VuoText old = *obj;
	VuoText ascii = VuoText_makeFromUtf32(new_text_32, new_text_len);
	VuoLocal(ascii);
	*obj = VuoText_insert(old, pos + 1, ascii);
	VuoRelease(old);
	VuoRetain(*obj);
	return true;
}
