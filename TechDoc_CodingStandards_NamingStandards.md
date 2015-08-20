Naming standards are used throughout all source and script files created by RenEvo Software & Designs. These standards ensure a common form of communication is kept with all members of the development team. This allows all immediate team members to easily read the code written for the current project, and it allows for all future teams and team members to easily read the code for inclusion in future projects. All aspects of the code from variable prefixing and naming to classes and structures must follow the naming convention.

To review the variable prefix conventions, please visit [here](TechDoc_CodingStandards_PrefixConvention.md)

To review the structure and class conventions, please visit [here](TechDoc_CodingStandards_StructClass.md)

## File Names ##

Source files created for including with the Dead 6 Core must be named according to their contents:
  * If the source file contains an interface declaration, it must begin with an 'I'. For example: "ID6TeamManager."
  * If the source file contains a module's declaration or definition, it must begin with a 'C'. For example: "CD6TeamManager.h" and "CD6TeamManager.cpp"
  * In any other case, only the file name is necessary. If the first letter of the file name is either an 'I' or a 'C', the second letter must not be capitalized. For example: "SomeCodeNotAModule.cpp"
It is important to note that each letter that begins a new word in the filename must be capitalized. Underscores must be used in place of spaces.

Lua script files have no file naming convention. However, the names for the scripts must adhere to the general rules laid out by the Engine and must hint at what is defined inside. For example, if a Lua script defines the GDI Barracks, it should be named something similar to "GDIBarracks.lua" and placed next to other scripts defining other buildings.

## File Locations ##

When organizing source files into the project's structure tree, the physical location on the harddrive must match according to its contents:
  * If the source file contains an interface declaration, it must be placed within the "Dead6/Interfaces" folder.
  * All other source files must be placed within the "Dead6" folder.
The Dead6 folder is located within the "GameDLL" folder, where the Visual Studio project file is located. All CE2 source files must remain within the "GameDLL" folder.

The Script folder hierarchy should be obeyed when placing Lua script files. See the CE2 Engine documentation for details on this.

[Back](TechDoc_CodingStandards.md)