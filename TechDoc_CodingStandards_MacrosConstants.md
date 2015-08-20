Pre-processor defined macros are permissible. Macros must have a purposeful reason to be used.

Valid reasons for using a macro:
  * Shorten commonly-typed code for a static result, such as error checking or calling a function with a particular set of arguments.
  * Simplify accessing/modifying variables or constants of similar names.

Invalid reasons for using a macro:
  * Shortcutting code that is only used in one or two places.
  * Code that takes up more than half a page if normally typed out.

Pre-processor defined constants may be used to store constant data that is referenced in several locations and may need to be changed later on during the development cycle. Examples of these include text files read in/out or character buffer sizes. A constant should be declared in the source file it is referenced in if the same data is not needed outside the module or in other source files. If the later is the case, it should be placed in the header file of the module that most frequently references it. Naming of constants should hint at what module uses it and what it is used for. Avoid common names to keep global pollution to a minimum.

Enumerations should be used for constant values that are more frequent in numbers, such as input actions/buttons and GUIDs. The same rules for constants apply to enumerations.

[Back](TechDoc_CodingStandards.md)