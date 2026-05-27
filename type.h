/*****************************************************************************
 *   type.h:  Type definition Header file for NXP LPC230x Family 
 *   Microprocessors
 *
 *   Copyright(C) 2006, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2006.09.01  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#ifndef __TYPE_H__
#define __TYPE_H__

#define NULL    ((void *)0)
#define FALSE   0
#define TRUE    1
#define _NO_    0
#define _NC_    1
#define _NL_    2
#define _OFF_   0
#define _ON_    1

//#define  MovBit(x)        ((DWORD)1<<(x)) 
//#define  ReadBit(x,y)     ((x)&(y)) 
//#define  WriteBit(x,y,z)  ( (z==0) ? ((x)&=(~(y))) : ((x)|=(y)) )
//#define  Bit(x,y,z)       ( (z>1)  ? ReadBit(x,MovBit(y)) : WriteBit(x,MovBit(y),z) )
//#define  Bit64(x,y,z)     ( (z>1)  ? ReadBit(x[(y>>5)&0x01],MovBit(y&0x1f)) : WriteBit(x[(y>>5)&0x01],MovBit(y&0x1f),z) ) 

typedef unsigned char  BYTE;				  
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   BOOL;
typedef unsigned long long  LWORD;

typedef struct
{
	DWORD Frame; 	// Bits 16..19: DLC - Data Length Counter
					// Bit 30: Set if this is a RTR message
					// Bit 31: Set if this is a 29-bit ID message
	DWORD MsgID;	// CAN Message ID (11-bit or 29-bit)
	DWORD DataA;	// CAN Message Data Bytes 0-7
	DWORD DataB;	// CAN Message Data Bytes 0-7
} CAN_MSG;

typedef struct
{
  int (*CurrentOperate)();
} FunTabStruct;

DWORD Bit64(DWORD *p, DWORD i, DWORD Value);

#endif  /* __TYPE_H__ */
