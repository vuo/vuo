These nodes are for playing movies (videos). 

A movie file contains a sequence of **frames**. Each frame has an image and a time when it's supposed to be displayed. You can play frames in sequence using the `Play Movie` node. 

These nodes use [FFmpeg](http://www.ffmpeg.org/) and support any of its [supported file formats](http://www.ffmpeg.org/general.html#File-Formats). 

These nodes support both [inter-frame](http://en.wikipedia.org/wiki/Inter_frame) and [intra-frame](http://en.wikipedia.org/wiki/Intra-frame) codecs. When retrieving a frame from an inter-frame movie, these nodes output the frame as it would normally be shown (interpolated from keyframes, not as raw inter-frame data). Since this requires extra processing, you'll often get better performance with an intra-frame codec (such as MJPEG, ProRes 422/4444, or DV). 

Currently, these nodes don't play sound. 

To download a movie from the internet, copy the movie's URL from your browser. Example: 

   - `http://example.com/video.mp4`

Currently, URLs that begin with `https` are not supported. 

To load a movie from a file on the computer running the composition, use the file's location on the computer. Examples: 

   - `file:///System/Library/Compositions/Fish.mov`
   - `/System/Library/Compositions/Fish.mov`
   - `Fish.mov` (for a file called "Fish.mov" in the same folder as the composition)
   - `movies/Fish.mov` (for a file called "Fish.mov" in a folder called "movies" in the same folder as the composition)

If the file's location doesn't start with `file://` or `/`, then it's treated as a relative path. This is the option you'd typically want when sharing the composition with others. If running the composition on a different computer than it was created on, the file needs to exist in the same location relative to the composition (`.vuo`) file. If running an application exported from the composition, the file needs to exist in the `Contents/Resources` folder inside the application package; see the [Vuo Manual](http://vuo.org/manual.pdf) for details. 

If the file's location starts with `file://` or `/`, then it's treated as an absolute path. If running the composition on a different computer than it was created on, the file needs to exist in the same location on that computer. To get the file's path in the correct format, open the TextEdit application, go to Format > Make Plain Text, drag the file onto the TextEdit window, and copy the path that appears in the TextEdit window. 
