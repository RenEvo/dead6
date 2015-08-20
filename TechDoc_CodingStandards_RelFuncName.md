Functions should be named logically in such a way that anybody working with and/or reading the code can easily distinguish the function's purpose completely by the name alone. There is no limit to the length of a function's name; however, lengthy names are frowned upon. Keep it concise and simple. Separations of words in the name are noted with capital letters i.e. the function named _"This is a function"_ is read as _"ThisIsAFunction."_

No prefixes are applied to function names. The only acception to this rule is in regards to private member functions defined in classes. If a member function is declared with private permission and can only be called from within the class' namespace, the function's name should be prefixed with an underscore. For example:

```
...

private:
	//////////////////////////////////////////////////////
	// _FillArray
	//
	// Purpose: Fill the private array with 0's.
	//////////////////////////////////////////////////////
	void _FillArray(void);
```

[Back](TechDoc_CodingStandards.md)