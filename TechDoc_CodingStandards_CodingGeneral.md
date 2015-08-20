## Class Rules ##
  * Handle the _Trilogy of Evil_ correctly.  Define the Assignment Operator and Copy Constructor when necessary, especially when creating a singleton. The operations do not need to be fully implemented, but if the module is holding on to dynamic memory or sensitive data which could break when hard copied, these must be declared privately to prevent accessing them.
  * Destructor must be declared virtual if the class/structure is inherited or takes advantage of inheritance.
  * All modules must have an Initialize() and Shutdown() routine defined unless otherwise noted.

## Heap Access Rules ##
  * All allocated memory must be freed when it is no longer needed.
  * Pointers that held on to virtual memory which has been freed should be reassigned to NULL.

## Other Rules ##
  * When casting variables, use the C standard cast in place of the C++ static\_cast declaration.
  * No warnings allowed!
  * Initialize all variables either on their declaration line or in the class' constructor.
  * Initializing a pointer means setting it equal to NULL/0.
  * Opening and closing braces must be placed on their own line below the condition statement and code block, respectively.

[Back](TechDoc_CodingStandards.md)