/**
 * @file
 * VuoPluginApp interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 * 
 * Example application demonstrating the use of Vuo compositions as plugins.
 * This application expects the composition plugin to have the following published ports:
 * - Inputs:
 *   - 'image' (Data type: Image)
 *   - 'time'  (Data type: Real)
 * - Outputs:
 *   - 'outputImage' (Data type: Image)
 */

#import <Cocoa/Cocoa.h>
#include <Vuo/Vuo.h>

@interface Document : NSDocument
{
    std::string compositionAsString; // The Vuo composition, represented as a .vuo source string.
    VuoRunner *runner; // The runner used to start, stop, control, and query the composition.
    NSImage *outputImage; // The output image to be retrieved from the running composition.
}

// UI elements
@property (unsafe_unretained) IBOutlet NSButton *runButton;
@property (unsafe_unretained) IBOutlet NSButton *stopButton;
@property (unsafe_unretained) IBOutlet NSButton *setImageButton;
@property (unsafe_unretained) IBOutlet NSSlider *timeSlider;
@property (unsafe_unretained) IBOutlet NSImageView *outputImageView;
@property (unsafe_unretained) IBOutlet NSTextField *imageTextField;
@property (unsafe_unretained) IBOutlet NSTextField *imageLabel;
@property (unsafe_unretained) IBOutlet NSTextField *timeLabel;
@property (unsafe_unretained) IBOutlet NSTextField *outputImageLabel;

@end
