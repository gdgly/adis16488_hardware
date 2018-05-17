#include "delay.h"
#include "stm32f1xx_hal.h" 

#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos ??	  
#endif

static uint32_t fac_us=0;							//us?????

#if SYSTEM_SUPPORT_OS		
    static uint16_t fac_ms=0;				        //ms?????,?os?,???????ms?
#endif

#if SYSTEM_SUPPORT_OS							//??SYSTEM_SUPPORT_OS???,?????OS?(???UCOS).

#ifdef 	OS_CRITICAL_METHOD						//OS_CRITICAL_METHOD???,?????UCOSII				
#define delay_osrunning		OSRunning			//OS??????,0,???;1,???
#define delay_ostickspersec	OS_TICKS_PER_SEC	//OS????,???????
#define delay_osintnesting 	OSIntNesting		//??????,???????
#endif

//??UCOSIII
#ifdef 	CPU_CFG_CRITICAL_METHOD					//CPU_CFG_CRITICAL_METHOD???,?????UCOSIII	
#define delay_osrunning		OSRunning			//OS??????,0,???;1,???
#define delay_ostickspersec	OSCfg_TickRate_Hz	//OS????,???????
#define delay_osintnesting 	OSIntNestingCtr		//??????,???????
#endif


void delay_osschedlock(void)
{
#ifdef CPU_CFG_CRITICAL_METHOD   			//??UCOSIII
	OS_ERR err; 
	OSSchedLock(&err);						//UCOSIII???,????,????us??
#else										//??UCOSII
	OSSchedLock();							//UCOSII???,????,????us??
#endif
}

//us????,??????
void delay_osschedunlock(void)
{	
#ifdef CPU_CFG_CRITICAL_METHOD   			//??UCOSIII
	OS_ERR err; 
	OSSchedUnlock(&err);					//UCOSIII???,????
#else										//??UCOSII
	OSSchedUnlock();						//UCOSII???,????
#endif
}

void delay_ostimedly(uint32_t ticks)
{
#ifdef CPU_CFG_CRITICAL_METHOD
	OS_ERR err; 
	OSTimeDly(ticks,OS_OPT_TIME_PERIODIC,&err); //UCOSIII????????
#else
	OSTimeDly(ticks);						    //UCOSII??
#endif 
}
 
void SysTick_Handler(void)
{	
    HAL_IncTick();
	if(delay_osrunning==1)					//OS????,??????????
	{
		OSIntEnter();						//????
		OSTimeTick();       				//??ucos???????               
		OSIntExit();       	 				//?????????
	}
}
#endif
			   
void delay_init(uint8_t SYSCLK)
{
#if SYSTEM_SUPPORT_OS 						//??????OS.
	uint32-t reload;
#endif
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);//SysTick???HCLK
	fac_us=SYSCLK;						//??????OS,fac_us?????
#if SYSTEM_SUPPORT_OS 						//??????OS.
	reload = SYSCLK;					    //???????? ???K	   
	reload *= 1000000/delay_ostickspersec;	//
											//reload?24????,???:16777216,?180M?,??0.745s??	
	fac_ms = 1000/delay_ostickspersec;		//??OS?????????	   
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;//??SYSTICK??
	SysTick->LOAD = reload; 					//?1/OS_TICKS_PER_SEC?????	
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; //??SYSTICK
#else
#endif
}								    

#if SYSTEM_SUPPORT_OS 						//??????OS.
							   
void delay_us(uint32_t nus)
{		
	uint32_t ticks;
	uint32_t told,tnow,tcnt = 0;
	uint32_t reload = SysTick->LOAD;				//LOAD??	    	 
	ticks = nus*fac_us; 						//?????? 
	delay_osschedlock();					//??OS??,????us??
	told = SysTick->VAL;        				//?????????
	while(1)
	{
		tnow = SysTick->VAL;	
		if(tnow != told)
		{	    
			if(tnow<told) 
				tcnt+=told-tnow;	//??????SYSTICK?????????????.
			else 
				tcnt+=reload-tnow+told;	    
			
			told = tnow;
			if(tcnt>=ticks)
				break;			//????/????????,???.
		}  
	};
	delay_osschedunlock();					//??OS??											    
}  

void delay_ms(uint16_t nms)
{	
	if(delay_osrunning&&delay_osintnesting==0)//??OS?????,?????????(??????????)	    
	{		 
		if(nms>=fac_ms)						//???????OS??????? 
		{ 
   			delay_ostimedly(nms/fac_ms);	//OS??
		}
		nms%=fac_ms;						//OS?????????????,????????    
	}
	delay_us((uint32_t)(nms*1000));				//??????
}
#else  //??ucos?
 
void delay_us(uint32_t nus)
{		
	uint32_t ticks;
	uint32_t told,tnow,tcnt = 0;
	uint32_t reload = SysTick->LOAD;				//LOAD??	    	 
	ticks = nus*fac_us; 						//?????? 
	told = SysTick->VAL;        				//?????????
	while(1)
	{
		tnow = SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)
				tcnt += told-tnow;	//??????SYSTICK?????????????.
			else 
				tcnt += reload-tnow+told;	  
			
			told = tnow;
			if(tcnt>=ticks)break;			//????/????????,???.
		}  
	};
}

void delay_ms(uint16_t nms)
{
	uint32_t i;
	
	for(i=0;i<nms;i++) 
		delay_us(1000);
}
#endif