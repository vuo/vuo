# Vuo Example Compositions


This document will provide some explanation for Vuo’s sample compositions. By opening and running these compositions, you can better see how events trigger node actions, data gets transmitted to downstream nodes, and how certain nodes function. 

As you run these compositions please remember Vuo provides three ways to understand what is going on in your composition. First, node tooltips. If you hover over a node in the node Library or hover over the node title on the canvas you can see more detail on how a specific node functions. If you hover over a port, you will display a port popover, which displays information about the node as it executes. These can be dragged away from the node to display information continuously as the composition is running. The last is Vuo’s "Show Events" mode under the Run menu, which shows events passing though your composition. Please see the User Manual for more details about how to use these aids. 


## BlendImages.vuo

This composition demonstrates blending two images into a single image, which is displayed in a window. The composition retrieves two images. These images are then blended using the Blend Images node. You can edit the Blend Image node's "blendMode" and "foregroundOpacity" values. More information about blend modes is available in the node tooltip documentation. The Fire Periodically node and the foreground Opacity port let you manipulate the image while the composition is running.


## CalculateTip.vuo

This composition calculates the amount to tip at a restaurant. It shows how to enter text into a composition, convert it to a number, do calculations on that number and output the result as a piece of text. 


## CheckSMSLength.vuo

This composition waits for the user to type a line and hit "Enter." Then it reports whether the text entered is more or less than 160 characters. Note that the typed Line port is a trigger port, generating an event every time a line is typed on the console window.


## Count.vuo

This composition outputs a count every second. 


## CountAndHold.vuo

This composition outputs the current count every 5 seconds by using two Fire Periodically nodes and a Hold Value node. The count gets incremented every second, but the value is held in the Hold Value node until the second Fire Periodically node causes the value to be written to the console window. This composition displays how the Hold Value node functions, and that the data-and-event that arrives at the newValue port causes the port to execute, but the event wall does not allow that event to travel downstream. For more about event walls,, please see the User Manual. 


## CountAndOffset.vuo

This composition shows that Vuo allows multiple output window. In this case the composition outputs a count to one window and an offset count to separate window every second.


## CountDown.vuo 

This composition outputs a countdown from T minus 5 seconds to liftoff. It uses two Is Less Than nodes and two Select Input nodes to control what is sent to the console window and then, what the console window prints out.


## CountLeapObjects.vuo

This composition outputs every second how many hands and pointables (fingers and tools) are detected by a Leap Motion device. 

This composition requires a Leap Motion device. 


## CountSometimes.vuo

This composition outputs a count every second. but the count is only incremented every five seconds. This composition demonstrates the use of the refresh port and the increment input port for the Count node. 


## CountWithFeedback.vuo

This composition outputs a count every second, showing a simple feedback loop implemented using the Hold Value node 


## CountWithPublishedPorts.vuo

This composition displays the use of published ports, and outputs a count every second. For more on published ports, please see the Vuo Manual. 


## DisplayImage.vuo

This composition gets an image and displays in in a window. 


## DisplayLeapHand

This composition takes a frame of data from Leap Motion, finds the right-most hand, then draws a simple 3d represenation on screen.

This composition requires a Leap Motion device. 


## DisplayScene.vuo

This composition demonstrates how to load a 3D scene from a mesh file, how to position it on the canvas, rotate it, and render it in a window.


## DisplaySquare.vuo

This composition renders a square using the Make Square Vertices node and the Make 3D Object node. 


## ExploreColorSchemes.vuo

This composition shows circles of different color schemes against white and black backgrounds. Scroll to change the central color. Click to change the color scheme. This composition also demonstrates making multiple objects and rendering them to a single window. 
 
The color schemes are based on points spaced at certain distances around the red-green-blue color wheel. They correspond to the triad, split-complementary, and analogous color schemes that are usually associated with a red-yellow-blue color wheel (http://www.tigercolor.com/color-lab/color-theory/color-harmonies.htm). 


## FlipCoin.vuo

This composition lets you flip a coin by scrolling the mouse or using a laptop touch pad. It demonstrates loading two images and manipulating them as a single entity based on mouse action. 

This composition requires an internet connection to download the coin image. 


## HelloWorld.vuo

This composition prints "Hello world!" on the console window.


## LoadImagesAsynchronously.vuo

This composition displays one image while downloading another image, then displays the second image. This composition works using the Spin Off Event node. 


## PlayFingerPuppetsWithLeap.vuo

This composition uses the location of a finger, detected by a Leap Motion device, and animates a finger puppet to move from foreground to background, and left to right. 

This composition requires a Leap Motion device. 


## PlayTennis.vuo

This composition is a game of tennis, similar to a certain well-known game from the 1970s. It pits a human player against a computer player. The human controls a tennis "racquet" by moving the mouse.


## PlayTennisWithLeap.vuo

This version of the composition allows the user to use two fingers to control the paddles.

This composition requires a Leap Motion device. 


## RevealWord.vuo

This composition reveals a word one letter at a time. Hidden letters are replaced with periods. It stops outputting once all letters have been revealed. This composition highlights use of the Cut node. 


## RippleImageOfSphere.vuo

This composition renders a rotating sphere to an image then uses a Ripple Image node to modify it. This composition uses a Make 3D object node, a Render Scene to Image and Make 3D Object From Image to create the final composition. 


## SendMIDINotes.vuo

This composition sends MIDI notes with pitches derived from gradient noise. 

This composition doesn't play sound on its own. You need to connect a MIDI device that can receive and play the MIDI notes. 


## ShowMouseClicks.vuo

This composition demonstrates how to differentiate between mouse button actions, displaying mouse clicks and releases, including double clicks. 


## SpinSphere.vuo

This composition demonstrates how to create a 3D object and animate it in Vuo. Specifically, it renders a sphere using OpenGL.


## TwirlImageWithLeap.vuo

This composition uses the palm of a hand, detected by a Leap Motion Device, to distort an image, using the Twirl Image node. 

This composition requires a Leap Motion device. 


## WalkCaterpillar.vuo

This composition makes a caterpillar walk toward the point where you most recently pressed the left mouse button. This composition displays how to combine objects and animate them as a group, based on user input. 


## WanderImage.vuo

This composition animates an image moving along an organic, wandering path using the Make Gradient Noise nodes.

This composition requires an internet connection to download the image. 


## WaveSphere.vuo

This composition is another example of creating a 3D object and animating it. In this case it is a sphere with a wave motion.
