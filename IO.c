/**********************************************************************/
//驱动
#include "LPC23xx.h"
#include "type.h"
#include "irq.h"
#include "target.h"
#include "absacc.h"
#include "timer.h"
//#include "ex_int.h"
#include "rtc.h"
#include "can.h"
//#include "uart.h"
//电梯
#define  IO_Ex
#include "IO.h"
//#include "Call.h"
#include "Lift.h"
#include "Flash.h"
#include "Menu.h"
#include "LiftCan.h"
/**********************************************************************/
void GPIOInit(void)
{
	SCS |= 0x00000001; //增强
	FIO1DIR  |= ((DWORD)1<<15)|((DWORD)1<<14)|((DWORD)1<<10)|((DWORD)1<<9)|((DWORD)1<<8)|((DWORD)1<<4)|((DWORD)1<<1)|((DWORD)1<<0);
}
/**********************************************************************/
void ScanOutput(void)
{
	if(FlashTimer<10)
		FlashTimer++;
	else
		FlashTimer = 0;
	if((LiftOnline[0])&&(FlashTimer<5))
		WriteLed1(0);
	else
		WriteLed1(1);
	if((LiftOnline[1])&&(FlashTimer<5))
		WriteLed2(0);
	else
		WriteLed2(1);
	if((LiftOnline[2])&&(FlashTimer<5))
		WriteLed3(0);
	else
		WriteLed3(1);
	if((LiftOnline[3])&&(FlashTimer<5))
		WriteLed4(0);
	else
		WriteLed4(1);
	if((LiftOnline[4])&&(FlashTimer<5))
		WriteLed5(0);
	else
		WriteLed5(1);
	if((LiftOnline[5])&&(FlashTimer<5))
		WriteLed6(0);
	else
		WriteLed6(1);
	if((LiftOnline[6])&&(FlashTimer<5))
		WriteLed7(0);
	else
		WriteLed7(1);
	if((LiftOnline[7])&&(FlashTimer<5))
		WriteLed8(0);
	else
		WriteLed8(1);
}
/**********************************************************************/
