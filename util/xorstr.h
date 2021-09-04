#pragma once

// warning C4172: returning address of local variable or temporary
// potential pointer dangling? seems fine for now.
#pragma warning( disable : 4172 )  

// generate 'pseudo-random' xor key based on file, date and line.
#define GET_XOR_KEYUI8  ( ( CONST_HASH( __FILE__ __TIMESTAMP__ ) + __LINE__ ) % UINT8_MAX )
#define GET_XOR_KEYUI16 ( ( CONST_HASH( __FILE__ __TIMESTAMP__ ) + __LINE__ ) % UINT16_MAX )
#define GET_XOR_KEYUI32 ( ( CONST_HASH( __FILE__ __TIMESTAMP__ ) + __LINE__ ) % UINT32_MAX )


#define XOR( s ) ( s )
