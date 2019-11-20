#pragma once

namespace os {

unsigned long get_tick_mS(void);
void wait(unsigned long time_mS);

void set_err(const char *perrtxt); //24.10.2019
void clear_err();

}


/*#ifndef OS_H
#define OS_H

#endif // OS_H*/
