/* ***** BEGIN LICENSE BLOCK *****
 *
 * The MIT License
 *
 * Copyright (c) 2012 BBC Research
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ***** END LICENSE BLOCK ***** */

#include <string.h>
#include "utf8.h"

void
init_utf8tounicode(utf8conv_state_t* state, const char *str)
{
	state->idx = 0;
	state->str = str;
	state->str_len = strlen(str);
}

unsigned utf8conv_getnext(utf8conv_state_t* state)
{
	int trailing_bytes = 0;
	int unicode_val = 0;
	int i = state->idx;

	if (i == state->str_len) {
		/* already at end */
		return 0;
	}

	for (; i < state->str_len; i++) {
		unsigned char c = state->str[i];
		/* convert utf-8 to unicode value */
		if (c < 0x80) {
			/* fast path for ascii */
			unicode_val = c;
			break;
		}
		if (c < 0xc0) {
			/* lost sync: discard */
			continue;
		}

		/* first byte: count leading ones */
		while ((c <<= 1) & 0x80)
			trailing_bytes++;
		unicode_val = c >> (1 + trailing_bytes);
		break;
	}

	int end = 1 + i + trailing_bytes;
	if (end > state->str_len) {
		/* invalid */
		unicode_val = 0xfffd;
	}
	else {
		for (i++; i < end; i++) {
			unsigned char c = state->str[i];
			if ((c & 0xc0) != 0x80) {
				/* missing byte somewhere, this is first byte of next word */
				i--;
				unicode_val = 0xfffd;
				break;
			}
			unicode_val = (unicode_val << 6) | (c & ~0xc0);
		}
	}

	state->idx = i;
	return unicode_val;
}
