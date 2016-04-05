Outputs a list of amplitudes, with each representing a range of frequencies in the audio.

This node is useful for finding the loudness of different ranges of pitches. The output can be used to create an audio spectrum visualizer.

This node performs a calculation called a [Fourier transform](http://nautil.us/blog/the-math-trick-behind-mp3s-jpegs-and-homer-simpsons-face). It divides up the full range of frequencies from the audio into subranges called *bins*, calculates the amplitude for each bin, and optionally aggregates the list of amplitudes to more closely match how humans perceive audio.

   - `Samples` — The audio samples to analyze.
   - `Frequency Bins` — The number of subranges that the full range of frequencies is divided into. With more bins, the audio is divided into smaller subranges of frequencies for a higher-resolution result.
   - `Frequency Bin Averaging` — How the amplitudes per bin are aggregated into the output list.
      - `None` does not aggregate. The output contains an item for each bin.
      - `Quadratic` and `Logarithmic` combine the results for bins, with more bins being combined at higher frequencies. The output more closely matches how humans perceive audio.

If you want a specific number of outputs, use the table below to pick the right `Frequency Bins` and `Frequency Bin Averaging`.<br>

Frequency Bins | Quadratic<br>Outputs | Logarithmic<br>Outputs
-------------: | -------------------: | ---------------------:
             3 |                    3 |                      3
             7 |                    5 |                      4
            15 |                    6 |                      5
            31 |                    9 |                      6
            63 |                   12 |                      7
           127 |                   17 |                      8
           255 |                   24 |                      9
           511 |                   33 |                     10
          1023 |                   46 |                     11
          2047 |                   65 |                     12

Technical details about the calculation: It's a fast Fourier transform (FFT). The total number of bins used in the FFT is `2 * (Frequency Bins + 1)` (a power of 2). `Frequency Bins` is the number of real-valued (frequency domain) coefficients minus the DC coefficient. The windowing function is Blackman-Harris.
