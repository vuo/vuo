Nodes for making numbers change over time, useful for animations.

Several of these nodes work with easing curves. An **easing curve** (easing function) is a curve that describes an object's speed over a period of time — for example, whether it starts moving slowly and picks up speed or it starts moving quickly and slows down. 

When you select an easing curve, you pick from two sets of options that control the shape of the curve: 

   - Type of curve
      - <img src="curveLinear.png" height=16> **Linear** — A straight line. As time changes, the speed stays the same (and the position changes in direct proportion to time). 
      - <img src="curveQuadratic.png" height=16> **Quadratic** — A gradual curve. As time changes, the speed changes in proportion to time (and the position changes in proportion to time squared). 
      - <img src="curveCubic.png" height=16> **Cubic** — A slightly steeper curve. As time changes, the speed changes in proportion to time squared (and the position changes in proportion to time cubed). 
      - <img src="curveCircular.png" height=16> **Circular** — An even steeper curve. As time changes, the position changes in proportion to a quarter circle. 
      - <img src="curveExponential.png" height=16> **Exponential** — A very steep curve. As time changes, the position changes in proportion to a function with time in the exponent. 
   - Type of easing
      - <img src="easingIn.png" height=16> **In** — The curve is shallowest near the start and steepest near the end.
      - <img src="easingOut.png" height=16> **Out** — The curve is shallowest near the end and steepest near the start.
      - <img src="easingIn+Out.png" height=16> **In + Out** — The curve is shallowest near the start and end, and steepest near the middle.
      - <img src="easingMiddle.png" height=16> **Middle** — The curve is shallowest in the middle and steepest near the start and end.

See [this page](https://web.archive.org/web/20190409200047/http://robertpenner.com/easing/) for more information on easing curves.
