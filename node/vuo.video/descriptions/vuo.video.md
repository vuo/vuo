Nodes for working with movie files, with live video from cameras, and streaming video.

### Video basics

The visual part of a video is made up of a sequence of **frames**. Each frame contains an image. Each frame also contains a time indicating when the frame was recorded or when it's intended to be played back, relative to the start of the video.

The audio part of a video is made up of a sequence of **samples** on one or more **channels**. (For more information about audio, see the [vuo.audio](vuo-nodeset://vuo.audio) node set documentation.) You can play the audio from a movie, or create a movie with audio, using a combination of nodes from this node set and the [vuo.audio](vuo-nodeset://vuo.audio) node set.

### Cameras

The [Receive Live Video](vuo-node://vuo.video.receive2) node, and related nodes for working with cameras, support any AV Foundation device. This includes the built-in iSight/FaceTime cameras in MacBooks and iMacs, iOS devices when connected via a Lightning cable, and many other USB and FireWire cameras.

### Movie URLs

Nodes that open or save to movie files have a URL input port. You can drag a movie file from Finder onto the input port's constant value to fill in the URL. Or drag the movie file onto the composition canvas to create a [Play Movie](vuo-node://vuo.video.play) node. These nodes only support file URLs, not HTTP.

### Movie file formats

A movie file has several characteristics that determine which applications (including Vuo) can play it and how efficiently it can be played.

The file extension (such as .mov or .mp4) tells you which **container** the movie uses. Different applications support different containers. Vuo's [Play Movie](vuo-node://vuo.video.play) and [Decode Movie Image](vuo-node://vuo.video.decodeImage) nodes can open any of the [file formats supported by FFmpeg](https://www.ffmpeg.org/general.html#File-Formats), including .mov, .avi, .dv, .mpeg, .mpg, .mp2, .m4v, .mp4, .webm, and .ogv.

Within the container, the way that the video and audio are stored in the file is determined by the **encoding**. As with containers, different applications support different encodings. Vuo's [Play Movie](vuo-node://vuo.video.play) and [Decode Movie Image](vuo-node://vuo.video.decodeImage) nodes can read any of the [video encodings supported by FFmpeg](https://www.ffmpeg.org/general.html#Video-Codecs).

Some video encodings (**delivery codecs**) are designed for playing back and sharing movie files, while others (**intermediate codecs**) are designed for video editing. Delivery codecs tend to produce videos with smaller file sizes and be more efficient when playing video frames in order (for example, with the [Play Movie](vuo-node://vuo.video.play) node at its default playback rate). Intermediate codecs tend to be more efficient when playing frames in reverse or otherwise out of order (for example, with the [Decode Movie Image](vuo-node://vuo.video.decodeImage) node).

The table below summarizes some of the video codecs supported by this node set.

Encoding    | File size | Non-forward playback | Supports transparency? | Supported by [Save Images](vuo-node://vuo.video.save)/[Frames to Movie](vuo-node://vuo.video.save2)? | Notes
----------- | --------- | -------------------- | ---------------------- | ------------------------------------------- | -----
H.264       | Smaller   | Less efficient       | No                     | Yes                                         |
H.265 (HEVC)| Smaller   | Less efficient       | No                     | Yes                                         |
H.265 (HEVC with Alpha)| Smaller | Less efficient | Yes                 | Yes                                         | macOS 10.15 and up
JPEG        | Larger    | More efficient       | No                     | Yes                                         |
ProRes 422  | Larger    | More efficient       | No                     | Yes                                         | See [Apple's documentation](https://support.apple.com/en-us/HT202410) for choosing between ProRes formats
ProRes 422 HQ | Larger    | More efficient     | No                     | Yes                                         | macOS 10.15 and up
ProRes 422 LT | Larger    | More efficient     | No                     | Yes                                         | macOS 10.15 and up
ProRes 422 Proxy | Larger | More efficient     | No                     | Yes                                         | macOS 10.15 and up
ProRes 4444 | Larger    | More efficient       | Yes                    | Yes                                         |
Hap         | Larger    | Less efficient       | No                     | No                                          | Hap Q has the same characteristics.
Hap Alpha   | Larger    | Less efficient       | Yes                    | No                                          | Hap Q Alpha has the same characteristics.

Audio is encoded separately from video. Similarly to video encodings, audio encodings have a tradeoff between quality and file size. **Lossless** encodings preserve the original audio at the expense of larger file sizes. **Lossy** encodings tend to produce smaller file sizes but may reduce the audio quality.

The [Play Movie](vuo-node://vuo.video.play) node can read any of the [audio encodings supported by FFmpeg](https://www.ffmpeg.org/general.html#Audio-Codecs). The table below summarizes some of the audio encodings supported by this node set.

Encoding       | Compression             | Supported by [Play Movie](vuo-node://vuo.video.play)? | Supported by [Save Images](vuo-node://vuo.video.save)/[Frames to Movie](vuo-node://vuo.video.save2)?
---------------|-------------------------|----------------------------|-----------------------------
Linear PCM     | Lossless (uncompressed) | Yes                        | Yes
Apple Lossless | Lossless                | Yes                        | No
AAC            | Lossy                   | Yes                        | Yes
MP3            | Lossy                   | Yes                        | No

To find out which video and audio encodings a movie file uses, you can open it in QuickTime Player and go to Window > Show Movie Inspector.

### Saving movies

Vuo provides 2 nodes for saving images and audio to movie files:

   - [Save Images to Movie](vuo-node://vuo.video.save) — Useful for realtime recording.  Simply feed images (and optionally audio) to this node, and it records them in a movie file in realtime.  Each frame's timestamp is derived from the time at which the image or audio event arrives at this node.
   - [Save Frames to Movie](vuo-node://vuo.video.save2) — Useful for non-realtime exporting and rotoscoping.  This node lets you explicitly specify the timestamps for images and audio sample buffers.

(See [Exporting a Movie](https://doc.vuo.org/latest/manual/exporting-compositions.xhtml#exporting-a-movie) for information on other ways to create movie files with Vuo.)
