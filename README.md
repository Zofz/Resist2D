This simple program reads a bitmap (e.g. png) representing the PCB copper shape (white), test electrodes (red and blue) and performs 2D resistance calculations.
The result is a current density map and a voltage drop map (animation). 
Moving mouse pointer over the image gives the numerical output in the actual point.
The program also shows the total resistance value, between the 2 terminals. To keep it simple for me, all values are to be scaled to the particular physical constraints.
I assume the resistance of the 2D square, perfectly connected at the opposite edges, to be 1. I also assume that the voltage across the test points is also 1.
The software is to be download in two versions. 32-bit and 64-bit. The reason for this is that the equation solving part, for now, is a straightforward Gaussian elimination, suitable only for relative small amount of variables, limited by n² storage requirement.
To make it a bit better, 64-bit version can access much bigger memory, available now on the standard PC.
The program starts with preloaded demo.

Compile on Visual Studio C++ - configuration included
