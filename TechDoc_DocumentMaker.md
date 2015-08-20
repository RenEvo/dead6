# Introduction #

To ease the documentation process of our modules during the development stages, a tool DocumentMaker has been developed to automatically parse our header files to generate Wiki-friendly pages.


# How To Use #

DocumentMaker.exe is ran through the command-line. The syntax is as followed:
```
Syntax: DocumentMaker.exe <filein> [fileout]
Arguments:
        filein: Header file to document.
        fileout: Optional output filename.
```

An example run would be:
```
DocumentMaker.exe "Dead6\ITeamManager.h"
```

If no output file is specified, a text file is created in the same directory as the input file with "_Output.txt" appended to the end._


# Sample #

This is a sample page outputted by the DocumentMaker tool. The following header file was parsed:

```
///////////////////////////////////////////////////
// Test.h
///////////////////////////////////////////////////

#ifndef _TEST_H_
#define _TEST_H_

class CTest
{
	///////////////////////////////////////////////////
	// Func1
	//
	// Purpose: This is a void function
	///////////////////////////////////////////////////
	void Func1(void);

	///////////////////////////////////////////////////
	// 
	// Func2
	//
	// Purpose: This just has arguments
	//
	// In:  nArg1 - First argument
	//			separated by a line
	//		pArg2 - Second argument
	///////////////////////////////////////////////////
	void Func2(int nArg1, char *pArg2);

	///////////////////////////////////////////////////
	//
	// Func3
	//
	// Purpose: This just returns a value
	//
	//  Out: nRet - Out
	//value
	//
	//	 Returns: 5+5
	///////////////////////////////////////////////////
	int Func3(int &nRet);

	///////////////////////////////////////////////////
	// Func4
	//
	// Purpose: Everything included!
	//
	// In:	pArgLongName1 - First argument
	//
	// Out:	pRet2 - Out argument
	//						on another line
	//
	// Returns TRUE
	//
	// Note: Testing a note.
	///////////////////////////////////////////////////
	bool Func4(void const* pArgLongName1, void *pRet2);

	///////////////////////////////////////////////////
	// FuncThatIsBroke
	//
	// All broke
	///////////////////////////////////////////////////
	void Func5(void);
};

struct ITestStruct {
	///////////////////////////////////////////////////
	// Func4
	//
	// Purpose: Everything included!
	//
	// In:	pArg1 - First argument
	//
	// Returns TRUE
	//
	// Note: Testing a note.
	///////////////////////////////////////////////////
	bool Func4(void *pArg1);
};

#endif //_TEST_H_
```

# Functionality #

## Test ##
Below are the methods defined in the Test module.

### **_Func1_** ###
**Purpose**:
This is a void function

**Arguments**:
void

**Returns**:
void


### **_Func2_** ###
**Purpose**:
This just has arguments

**Arguments**:
  * _nArg1_ - `[In]` First argument separated by a line
  * _pArg2_ - `[In]` Second argument

**Returns**:
void


### **_Func3_** ###
**Purpose**:
This just returns a value

**Arguments**:
  * _nRet_ - `[Out]` Out value

**Returns**:
5+5


### **_Func4_** ###
**Purpose**:
Everything included!

**Arguments**:
  * _pArgLongName1_ - `[In]` First argument
  * _pRet2_ - `[Out]` Out argument on another line

**Returns**:
TRUE

**Note**:
Testing a note.

### **_FuncThatIsBroke_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


## TestStruct ##
Below are the methods defined in the TestStruct interface.

### **_Func4_** ###
**Purpose**:
Everything included!

**Arguments**:
  * _pArg1_ - `[In]` First argument

**Returns**:
TRUE

**Note**:
Testing a note.


