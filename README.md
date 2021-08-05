# Incompressible Fluid Simulation

## About

This Project was made in C++ through Visual Studios with the help of OpenGL and GLFW. The goal of the project is to implement an incompressible fluid simulator and put a unique spin on it. 

### Resources

* https://www.youtube.com/watch?v=alhpH6ECFvQ&ab_channel=TheCodingTrain
* https://www.karlsims.com/fluid-flow.html
* https://www.khanacademy.org/math/multivariable-calculus

This is a learning project and thus heavily used other resources as references for the hard math and algorithms involved in Fluid Simulations.

## Controls

# Sim Window
Move your mouse around the screen while left clicking to add fluid to the simulation. Press Space to freeze the simulation. You may use your mouse to add particles to the sim during this freeze time. The window is also resizable. Press C to clear the simulation.

# Console
Here is a complete command list:
* "help" - Displays a list of all the commands.
* "clear" - Clears the sim of all fluid density particles and or tracers.
* "get fps" - Outputs the sim's current fps to the console.
* "set tracers enabled" - Enables the addition of tracers to the sim.
* "set tracers disabled" - Disables the addition of tracers to the sim and removes all existing tracers.
* "set colors enabled" - Enables traditional RGB channels in the fluid sim.
* "set colors disabled" - Turns the simulation to a single color channel which is between white and black.