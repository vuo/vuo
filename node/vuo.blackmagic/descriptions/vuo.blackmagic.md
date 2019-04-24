These nodes are for working with the video capture and playback devices manufactured by Blackmagic Design.

They provide the ability to list devices, receive video frames from a capture device, send video frames to a playback device, switch input connections, and switch video modes.  (Other features, such as VTR controls, are not yet supported.)  These nodes do not directly support sending and receiving audio, but since Blackmagic Design provides standard macOS audio drivers, you can use nodes in the [Audio node set](vuo-nodeset://vuo.audio) to work with Blackmagic audio.

Before using a Blackmagic device with Vuo, you need to download and install the Desktop Video software from the [Blackmagic Design Support Center](https://www.blackmagicdesign.com/support/).

## Supported devices

The following Blackmagic Design product lines are supported:

   - DeckLink
   - Intensity
   - UltraStudio
   - Teranex
   - Cinema Camera
   - HyperDeck Studio

Only some Blackmagic Design products can simultaneously capture and play back video ("full duplex").  For example, [DeckLink and UltraStudio products with "4K" in the name](https://forum.blackmagicdesign.com/viewtopic.php?p=205847#p205847) do support simultaneous capture and playback, whereas the Intensity Shuttle Thunderbolt does not.

## Device configuration

Some Blackmagic devices support autodetection of the device's connection and video mode, described below. Others do not, and you have to choose the correct settings in Vuo in order to receive or send video.

### Connection

Several nodes have a `Connection` port, which indicates the physical input source or output destination on the Blackmagic device. For example, if you're capturing video from a camera plugged into the Blackmagic device's HDMI IN port, the connection type in Vuo would be HDMI.

### Video Mode

Another common port is `Video Mode`, which indicates the resolution and framerate being used by the Blackmagic device. For example, if you're capturing video from a camera that's transmitting HD 1080i at 59.94 fps, that would be the video mode in Vuo. To find out the video modes compatible with your hardware, consult its documentation.

### Sub-device

Some Blackmagic devices have multiple inputs or outputs of the same connection type. Nodes such as `Make Blackmagic Input` and `Get Blackmagic Output Values` have a `Sub-device` port that distinguishes between the same-typed connections on the device. Sub-devices are numbered consecutively starting at 1. Device input editors and the `List Blackmagic Devices` node list each sub-device as a separate item.

## Deinterlacing

If your input source uses an interlaced video mode (NTSC, PAL, or HD 1080i), you may wish to remove the interlacing before applying spatial filters to it, or before displaying it on a progressive video output device such as a computer LCD display, projector, or an HD 1080p display.  These nodes offer a few deinterlacing options:

   - `None` — Outputs the image as-is, with each pair of fields combined into a single image, scanlines alternating.  This is appropriate for all progressive video, and for interlaced video if you either want to directly output to an interlaced video output device, or perform your own deinterlacing.  For NTSC video, frames are fired at 29.97 FPS, and for PAL, frames are fired at 25 FPS.
   - `Alternate fields` — Outputs each field as a separate image.  The scanlines in each field are doubled and smoothed.  For NTSC video, frames are fired at 59.94 FPS, and for PAL, frames are fired at 50 FPS.  This produces smooth motion and lower latency, but lower vertical resolution.
   - `Blend fields` — Outputs each pair of fields as a single image.  Each pair of fields is blended together and smoothed.  For NTSC video, frames are fired at 29.97 FPS, and for PAL, frames are fired at 25 FPS.  This produces better vertical resolution at the expense of temporal resolution.

## Troubleshooting

   - In System Preferences, open the Blackmagic Desktop Video preference pane. You should see your device listed there.
   - If you're trying to capture video from a Blackmagic device: Open the Blackmagic Media Express app (installed along with the Blackmagic Design Desktop Video package). You should see your video in the Log and Capture tab.
   - Check for messages in the Console app.
