These nodes are for receiving live video from cameras, and for playing movies. 

### Receiving live video

The `Receive Live Video` and `List Video Devices` nodes use QuickTime, and therefore should work with any device that works with the modern QuickTime interface — like the built-in iSight/FaceTime cameras in MacBooks and iMacs, and many other USB and FireWire cameras.

### Playing movies

A movie file contains a sequence of **frames**. Each frame has an image and a time when it's supposed to be displayed. You can play frames in sequence using the `Play Movie` node. You can grab individual frames using the `Decode Movie Image` node. 

A movie file may also contain audio, which can be played using the `Play Movie` node. See that node's description for more information.

These nodes use [FFmpeg](http://www.ffmpeg.org/) and support any of its [supported file formats](http://www.ffmpeg.org/general.html#File-Formats). This includes files with extension .mov, .avi, .dv, .mpeg, .mpg, .mp2, .m4v, .mp4, and .ogv. They don't support the proprietary AAC or MP3 audio formats. 

These nodes support both [inter-frame](http://en.wikipedia.org/wiki/Inter_frame) and [intra-frame](http://en.wikipedia.org/wiki/Intra-frame) codecs. When retrieving a frame from an inter-frame movie, these nodes output the frame as it would normally be shown (interpolated from keyframes, not as raw inter-frame data). Since this requires extra processing — especially when using `Decode Movie Image`, or `Play Movie` with mirrored looping or a negative `Playback Rate` — you'll often get better performance with an intra-frame codec (such as MJPEG, ProRes 422/4444, or DV). 

Currently these nodes only support URLs which are filesystem paths. (In a future release, you'll be able to use HTTP URLs.)

To load a movie from a file on the computer running the composition, use the file's location on the computer. Example: 

   - `/System/Library/Compositions/Fish.mov`
