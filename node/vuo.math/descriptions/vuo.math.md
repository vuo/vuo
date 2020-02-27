Nodes for doing mathematical calculations with numbers.

Many of these nodes can work with various types of numbers — Integers, Reals, 2D Points, 3D Points, 4D Points — because their ports have changeable data types. To change the type of number in the calculation, change the port's data type. (For example, right-click on the port and select from the Set Data Type submenu.)

In calculations, a difference between Integers and Reals is that the results for Integers are exact, while the results for Reals (numbers with decimal points) involve small rounding errors. For example, the Integer version of  [Divide](vuo-node://vuo.math.divide.VuoInteger) says that 4 divided by 3 equals 1 remainder 1, while the Real version of [Divide](vuo-node://vuo.math.divide.VuoReal) says that 4 divided by 3 equals 1.3333333333 (slightly less than 1 1/3).
