[Back](TechDoc_Architecture.md)

# Overview #

These General Flow Nodes fit in no special category. They exist to add or aid functionality present in the Flow Graphs.

# [Functionality](TechDoc_Architecture_Game_Node_General_Functionality.md) #

# Flow Nodes #

## Time:Stopwatch ##
Purpose: This Node acts as a stopwatch and can be used to calculate the amount of time it takes to complete two events.

Input Ports:
  * Start - Call to start the stopwatch.
  * Stop - Call to stop the stopwatch.
  * Timeout - (Float) Amount of time that can pass before stopwatch automatically stops.

Output Ports:
  * Done - Triggered when stopwatch is stopped. Returns amount of time that past since it was started.

## Game:EditorGameStart ##
Purpose: This Node can be used to start a chain of execution when the game in the Editor starts. It serves no purpose when ran through the game outside the Editor.

Input Ports:
N/A

Output Ports:
  * Start - Triggered when the game in the Editor starts.

[Back](TechDoc_Architecture.md)