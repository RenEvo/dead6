[Back](TechDoc_Architecture.md)

# Overview #

These nodes extend the Dead6 Player Class and allow the designer to manipulate a Dead6 player in the game.

# [Functionality](TechDoc_Architecture_Game_Node_D6Player_Functionality.md) #

# Flow Nodes #

## D6Player:CreditBank ##
Purpose: This node can be used to access or manipulate the amount of credits a Dead6 Player owns.

Input Ports:
  * GetCredits - Call to just get the amount of credits owned by the player
  * SetCredits - Call to set the credits to the amount specified
  * GiveCredits - Call to give (+) amount of credits specified
  * TakeCredits - Call to take (-) amount of credits specified
  * Amount - (Float) Amount of credits to set/give/take

Output Ports:
  * Credits - Resulting amount of credits based on action

## D6Player:CreditRange ##
Purpose: This node can be used to check if a Dead6 Player owns a certain amount of credits by comparing them to an inclusive range.

Input Ports:
  * Trigger - Call to run the test
  * MinAmount - (Int) Minimum amount to check for
  * MaxAmount - (Int) Maximum amount to check for

Output Ports:
  * Passed - Triggered if test passes (in range)
  * Failed - Triggered if test fails (not in range)

[Back](TechDoc_Architecture.md)