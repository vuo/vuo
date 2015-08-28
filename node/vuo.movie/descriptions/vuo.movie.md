These nodes are for playing movies (videos). 

A movie file contains a sequence of **frames**. Each frame has an image and a time when it's supposed to be displayed. You can play frames in sequence using the `Play Movie` node. You can grab individual frames using the `Decode Movie Image` node. 

These nodes use [FFmpeg](http://www.ffmpeg.org/) and support any of its [supported file formats](http://www.ffmpeg.org/general.html#File-Formats). They don't support the proprietary AAC audio format. 

These nodes support both [inter-frame](http://en.wikipedia.org/wiki/Inter_frame) and [intra-frame](http://en.wikipedia.org/wiki/Intra-frame) codecs. When retrieving a frame from an inter-frame movie, these nodes output the frame as it would normally be shown (interpolated from keyframes, not as raw inter-frame data). Since this requires extra processing — especially when using `Decode Movie Image`, or `Play Movie` with mirrored looping or a negative `playbackRate` — you'll often get better performance with an intra-frame codec (such as MJPEG, ProRes 422/4444, or DV). 

These nodes support decoding the movie's sound, too.  Connect the `decodedAudio` port to a `Send Live Audio` node's `sendChannels` port to hear the audio.  (The `decodedAudio` port only fires events when the movie is playing forward at normal speed. When the `playbackRate` is not 1, or when the `loop` is "Mirror" and the movie is playing in reverse, the `decodedAudio` port does not fire events.)

Currently these nodes only support `movieURL`s which are absolute filesystem paths. (In a future release, you'll be able to use relative paths and HTTP URLs.)

To load a movie from a file on the computer running the composition, use the file's location on the computer. Example: 

   - `/System/Library/Compositions/Fish.mov`
