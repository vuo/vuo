Creates an image of audio amplitudes.

Each horizontal line in the image corresponds to a channel of audio samples.

The height of the image in pixels equals the number of channels.  The width of the image in pixels equals the number of samples per sample buffer.

Each pixel in the line represents the amplitude of a single sample.  The `Range` port specifies how audio amplitudes are mapped to grayscale levels:

   - `Unipolar`
      - Zero amplitude is medium gray.
      - Positive amplitudes are light gray.
      - Negative amplitudes are dark gray.
   - `Bipolar`
      - Zero amplitude is black.
      - Positive amplitudes are gray.
      - Negative amplitudes are negative color intensities (typically rendered as black, but react differently with nodes such as [Blend Images](vuo-node://vuo.image.blend)).
   - `Absolute`
      - Zero amplitude is black.
      - Positive and negative amplitudes are gray.

![](channels-image.png)
