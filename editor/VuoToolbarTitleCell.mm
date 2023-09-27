/**
 * @file
 * VuoToolbarTitleCell implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoToolbarTitleCell.hh"

@implementation VuoToolbarTitleCell

- (NSRect)titleRectForBounds:(NSRect)frame
{
	CGFloat stringHeight = self.attributedStringValue.size.height;
	NSRect titleRect = [super titleRectForBounds:frame];
	CGFloat oldOriginY = frame.origin.y;
	titleRect.origin.y = frame.origin.y + (frame.size.height - stringHeight) / 2.0;
	titleRect.size.height = titleRect.size.height - (titleRect.origin.y - oldOriginY);
	return titleRect;
}

- (void)drawInteriorWithFrame:(NSRect)cFrame inView:(NSView*)cView
{
	[super drawInteriorWithFrame:[self titleRectForBounds:cFrame] inView:cView];
}

@end
