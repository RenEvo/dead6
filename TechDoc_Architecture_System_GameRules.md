[Back](TechDoc_Architecture.md)

# Overview #

The Dead6 GameRules is derived from the Crysis' GameRules (CGameRules) and handles the specific rules of the Dead 6 Core. Mainly, it is used to overwrite the team management code present in Crysis, and instead delegate the workload to the Team Manager.

# [Functionality](TechDoc_Architecture_System_GameRules_Functionality.md) #

# Damage Reports #

The Dead6 GameRules overloads the damage-control functions for Hit and Explosion types on both the Client and Server. It first delegates the workload to the base GameRules module to ensure compatibility with normal damage. It then delegates the same info to the Base Manager to handle interface lookups on the Building Controllers. This info is then passed along to the corresponding controller if it uses the hit entity as an interface.

[Back](TechDoc_Architecture.md)