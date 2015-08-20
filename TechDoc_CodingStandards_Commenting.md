All files must have a header comment at the top. All functions must have a function header above their declarations. All class member variables must have a comment either above or beside them that inform the reader of its purpose. Commenting inside the function bodies (to illustrate or describe function flow) are not required but are highly encouraged. Commenting noise should be kept at a minimum; when a comment is required, keep it as brief as possible to get the point across. The code should be clean enough to speak for itself in most cases.

## C++ File Headers ##
Must be present at the top of every source/header file.

```
	//////////////////////////////////////////////////////
	// C&C: The Dead 6 - Core File
	// Copyright (C), RenEvo Software & Designs, _DATE_
	//
	// FileName.h or FileName.cpp
	//
	// Purpose: Why do I exist?
	//
	// History:
	//	- 1/01/07 : File created - YOUR_INITIALS
	//	- 1/02/07 : <REASON> - YOUR_INITIALS
	//////////////////////////////////////////////////////
```

## Lua File Headers ##
Must be present at the top of every script file.

```
	------------------------------------------------
	-- C&C: The Dead 6 - Core File
	-- Copyright (C), RenEvo Software & Designs, _DATE_
	--
	-- FileName.lua
	--
	-- Purpose: Why do I exist?
	--
	-- History:
	--	- 1/01/07 : File created - YOUR_INITIALS
	--	- 1/02/07 : <REASON> - YOUR_INITIALS
	-----------------------------------------------
```

## C++ Member Function Headers ##
The following must be present before the member function declaration in the class and interface header files.  These do not need to be copied over to the source file. The In, Out, Return and Notes only need to be declared if they are valid for the member function.

```
	//////////////////////////////////////////////////////
	// Function_Name
	//
	// Purpose: What do I do?
	//
	// In:	arg_name - What is it?
	//	<Other entries follow>
	//
	// Out:	arg_name - What comes out?
	//	<Other entries follow>
	//
	// Returns <what do you return>
	// (Returns TRUE on success, false otherwise.)
	// (Returns the object ID.)
	//
	// Notes: <Any notes follow here>
	//////////////////////////////////////////////////////
```

## Lua Function Headers ##
The following must be present before the function body in the script file. The In, Return and Notes only need to be declared if they are valid for the function.

```
	------------------------------------------------
	-- Function_Name
	--
	-- Purpose: What do I do?
	--
	-- In:	arg_name - (Type expected) What is it?
	--	<Other entries follow>
	--
	-- Return: (Type expected) What is it?
	--	   <Other entries follow>
	--
	-- Notes: <Any notes follow here>
	------------------------------------------------
```

[Back](TechDoc_CodingStandards.md)