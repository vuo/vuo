These nodes are for receiving, analyzing, and outputting audio signals.

An audio **input device** (such as the MacBook Pro's built-in microphone, or an external USB interface) can send audio to the `Receive Live Audio` node, allowing the audio to control the composition. 

An audio **output device** (such as the MacBook Pro's built-in speakers, or an external USB interface) can receive audio from the `Send Live Audio` node, allowing you to hear audio that was read from a movie or sound file, or synthesized within Vuo. 

Each audio device has a unique **ID**, which is assigned by the operating system. If all devices were plugged in after the computer started up, then ID 0 is usually the first device plugged in, ID 1 is usually the second device plugged in, etc. 

Each audio device has a **name**, which is not necessarily unique. On Mac OS X, you can look up a device's name in the Audio MIDI Setup application. 

Each audio device has one or more **channels**.  A channel is an independent audio signal — for example, in stereo audio, there are two channels: left and right. In stereo, typically, left is channel 1 and right is channel 2. You can look up a device's channel numbers in the Audio MIDI Setup application. 

A continuous stream of audio is broken up into slices called **samples**.  Each sample is a measurement of the **amplitude** of the audio signal, which represents how much the physical airwaves are compressed or expanded at that instant in time.  In Vuo, each sample is typically a number between -1 and 1.  On Mac OS X, you can configure the sample rate in the Audio MIDI Setup application.  Higher sample rates result in a more accurate digital reproduction of the audio signal, but also require more computational power.

The `Receive Live Audio` node fires an event each time a new **sample buffer** is received.  A sample buffer is a collection of 512 audio samples.  Samples are grouped into buffers in order to improve efficiency.

Each event from the `Receive Live Audio` node provides a sample buffer for each channel.  You can split up the channels and analyze them individually using the `Analyze Audio Loudness` node, or you can combine them first using the `Mix Audio Channels` node.
