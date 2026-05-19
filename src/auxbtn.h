#ifndef AUXBTN_H
#define AUXBTN_H

#include "button.h"

void auxbtn_init(void);
void auxbtn_init_task(void);
void auxbtn_onOnePress(int key, void (*handler)(void));

#endif 