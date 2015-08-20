## Naming Convention ##

All variable names will abide to the Hungarian notation standards. The following formula should be applied when naming all classes/structures/variables:

Variable name = a\_bC where
  * a = Scope.  If local, not needed.  Otherwise use 'm' for member variables and 'g' for global variables.
  * b = Type.  See chart below.
  * C = Name of the variable.  This should be specific enough to know what the variable does or is used for.  Capital letters are used to break up words i.e. _ThisIsMyVariable_.

## Prefix Convention ##

Variables should be named by the type of data they store and not their location. For example:
```
int *m_pVar = &nVal; // A member variable that is a pointer to a local integer.
int *pVar = &m_nVal; // A local variable that is a pointer to a member integer.
int nVar = 5;        // A local integer variable.
```

The table below describes the prefix characters used for the given data types/scopes:

**Variable Types**
> | char | c |
|:-----|:--|
> | unsigned char | uc |
> | short | s |
> | unsigned short | us |
> | int  | n |
> | unsigned int | ui |
> | float | f |
> | double | d |
> | pointer | p... |
> | long | l... |

**Object Types**
> | Class Name | C... |
|:-----------|:-----|
> | Interface Name | I... |
> | Struct Name | t... |
> | Member Variable | m_..._|
> | Global Variable | g_..._|



[Back](TechDoc_CodingStandards.md)




