//this file implement only Wait funct stub.

unsigned long get_tick_mS(void);

void Wait(unsigned long time_mS)
{
	unsigned long start_time=get_tick_mS();
	while( (get_tick_mS()-start_time)<time_mS ){ asm("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;"); }
}
