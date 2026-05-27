/**********************************************************************/
#ifndef __IO_H
#define __IO_H

#ifdef  IO_Ex
   #define IO_Extern         //定义变量
#else
   #define IO_Extern extern  //声明变量
#endif
/**********************************************************************/
#define  WriteLed1(x)  ( (x==0) ? (FIO1CLR=((DWORD)1<<15)) : (FIO1SET=((DWORD)1<<15)) ) 
#define  WriteLed2(x)  ( (x==0) ? (FIO1CLR=((DWORD)1<<14)) : (FIO1SET=((DWORD)1<<14)) ) 
#define  WriteLed3(x)  ( (x==0) ? (FIO1CLR=((DWORD)1<<10)) : (FIO1SET=((DWORD)1<<10)) ) 
#define  WriteLed4(x)  ( (x==0) ? (FIO1CLR=((DWORD)1<<9))  : (FIO1SET=((DWORD)1<<9)) ) 
#define  WriteLed5(x)  ( (x==0) ? (FIO1CLR=((DWORD)1<<8))  : (FIO1SET=((DWORD)1<<8)) ) 
#define  WriteLed6(x)  ( (x==0) ? (FIO1CLR=((DWORD)1<<4))  : (FIO1SET=((DWORD)1<<4)) ) 
#define  WriteLed7(x)  ( (x==0) ? (FIO1CLR=((DWORD)1<<1))  : (FIO1SET=((DWORD)1<<1)) ) 
#define  WriteLed8(x)  ( (x==0) ? (FIO1CLR=((DWORD)1<<0))  : (FIO1SET=((DWORD)1<<0)) ) 
/**********************************************************************/
IO_Extern DWORD FlashTimer;
void GPIOInit( void );
void ScanOutput(void);
/**********************************************************************/
#endif














