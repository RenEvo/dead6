#ifndef __ValidNumber_h__
#define __ValidNumber_h__

//--------------------------------------------------------------------------------

// http://www.psc.edu/general/software/packages/ieee/ieee.html

/*
	The IEEE standard for floating point arithmetic
	""""""""""""""""""""""""""""""""""""""""""""""""

	Single Precision
	"""""""""""""""""

	The IEEE single precision floating point standard representation requires a 32 bit word, 
	which may be represented as numbered from 0 to 31, left to right. 
	The first bit is the sign bit, S, 
	the next eight bits are the exponent bits, 'E', 
	and the final 23 bits are the fraction 'F':

	S EEEEEEEE FFFFFFFFFFFFFFFFFFFFFFF
	0 1      8 9                    31

	The value V represented by the word may be determined as follows:

	* If E=255 and F is nonzero, then V=NaN ("Not a number")
	* If E=255 and F is zero and S is 1, then V=-Infinity
	* If E=255 and F is zero and S is 0, then V=Infinity
	* If 0<E<255 then V=(-1)**S * 2 ** (E-127) * (1.F) where "1.F" is intended to represent the binary number created by prefixing F with an implicit leading 1 and a binary point.
	* If E=0 and F is nonzero, then V=(-1)**S * 2 ** (-126) * (0.F) These are "unnormalized" values.
	* If E=0 and F is zero and S is 1, then V=-0
	* If E=0 and F is zero and S is 0, then V=0 



	Double Precision
	"""""""""""""""""

	The IEEE double precision floating point standard representation requires a 64 bit word, 
	which may be represented as numbered from 0 to 63, left to right. 
	The first bit is the sign bit, S, 
	the next eleven bits are the exponent bits, 'E', 
	and the final 52 bits are the fraction 'F':

	S EEEEEEEEEEE FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	0 1        11 12                                                63

	The value V represented by the word may be determined as follows:

	* If E=2047 and F is nonzero, then V=NaN ("Not a number")
	* If E=2047 and F is zero and S is 1, then V=-Infinity
	* If E=2047 and F is zero and S is 0, then V=Infinity
	* If 0<E<2047 then V=(-1)**S * 2 ** (E-1023) * (1.F) where "1.F" is intended to represent the binary number created by prefixing F with an implicit leading 1 and a binary point.
	* If E=0 and F is nonzero, then V=(-1)**S * 2 ** (-1022) * (0.F) These are "unnormalized" values.
	* If E=0 and F is zero and S is 1, then V=-0
	* If E=0 and F is zero and S is 0, then V=0 

*/

//--------------------------------------------------------------------------------

#define FloatU32(x)						(*( (uint32*) &(x) ))
#define FloatU32ExpMask				(0xFF << 23)
#define FloatU32FracMask			((1 << 23) - 1)
#define FloatU32SignMask			(1 << 31)
#define FloatNAN							(FloatU32ExpMask | FloatU32FracMask)

#define DoubleU64(x)					(*( (uint64*) &(x) ))
#define DoubleU64ExpMask			((uint64)255 << 55)
#define DoubleU64FracMask			(((uint64)1 << 55) - (uint64)1)
#define DoubleU64SignMask			((uint64)1 << 63)
#define DoubleNAN							(DoubleU64ExpMask | DoubleU64FracMask)

//--------------------------------------------------------------------------------

ILINE bool NumberValid(float x)
{
	return ((FloatU32(x) & FloatU32ExpMask) != FloatU32ExpMask);
}

ILINE bool NumberNAN(float x)
{
	return (((FloatU32(x) & FloatU32ExpMask) == FloatU32ExpMask) && 
					((FloatU32(x) & FloatU32FracMask) != 0));
}

ILINE bool NumberINF(float x)
{
	return (((FloatU32(x) & FloatU32ExpMask) == FloatU32ExpMask) && 
					((FloatU32(x) & FloatU32FracMask) == 0));
}

ILINE bool NumberDEN(float x)
{
	return (((FloatU32(x) & FloatU32ExpMask) == 0) && 
					((FloatU32(x) & FloatU32FracMask) != 0));
}

//--------------------------------------------------------------------------------

ILINE bool NumberValid(double x)
{
	return ((DoubleU64(x) & DoubleU64ExpMask) != DoubleU64ExpMask);
}

ILINE bool NumberNAN(double x)
{
	return (((DoubleU64(x) & DoubleU64ExpMask) == DoubleU64ExpMask) && 
					((DoubleU64(x) & DoubleU64FracMask) != 0));
}

ILINE bool NumberINF(double x)
{
	return (((DoubleU64(x) & DoubleU64ExpMask) == DoubleU64ExpMask) && 
					((DoubleU64(x) & DoubleU64FracMask) == 0));
}

ILINE bool NumberDEN(double x)
{
	return (((DoubleU64(x) & DoubleU64ExpMask) == 0) && 
					((DoubleU64(x) & DoubleU64FracMask) != 0));
}

//--------------------------------------------------------------------------------


ILINE bool NumberValid(int8 x)
{	
	return 1; //integers are always valid
}
ILINE bool NumberValid(uint8 x)
{	
	return 1; //integers are always valid
}
ILINE bool NumberValid(int16 x)
{	
	return 1; //integers are always valid
}
ILINE bool NumberValid(uint16 x)
{	
	return 1; //integers are always valid
}
ILINE bool NumberValid(int32 x)
{	
	return 1; //integers are always valid
}
ILINE bool NumberValid(uint32 x)
{	
	return 1; //integers are always valid
}
ILINE bool NumberValid(int64 x)
{	
	return 1; //integers are always valid
}
ILINE bool NumberValid(uint64 x)
{	
	return 1; //integers are always valid
}


#endif // __ValidNumber_h__
