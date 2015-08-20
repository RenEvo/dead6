[Back](TechDoc_Architecture.md)

# Overview #

This Vehicle Movement class is utilized _Harvester_-type vehicles. It derives from the **StdWheeled** movement class and uses the same format for its movement properties. Essentially, all this class does is clone the standard wheel movements and removes the logic that requires a driver entity be present for the vehicle's engine to turn on and process AI movement requests.

# [Functionality](TechDoc_Architecture_System_VMHarvester_Functionality.md) #

# Notes #
When defining the vehicle entity to be used for the Harvester, you should not define any seats. If you do and a player jumps in to the "driver" seat of the Harvester, it may interfere with the flow logic assigned to the Harvester, causing unforeseen errors and a disruption to the Harvester flow.

[Back](TechDoc_Architecture.md)