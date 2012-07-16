#ifndef MAKO_VM_H
#define MAKO_VM_H

#include <stdint.h>

void init_vm(int32_t *mem);
void run_vm();

extern uint32_t framebuf[320][240];

#endif
