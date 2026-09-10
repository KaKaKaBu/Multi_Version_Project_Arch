#ifndef IR_REMOTE_H
#define IR_REMOTE_H

#include "control.h"

void IrRemote_Init(void);
void IrRemote_Poll(void);
MotionCmd_t IrRemote_GetMotionCommand(void);
void IrRemote_OnEdge(void);

uint8_t IrRemote_GetLastCode(void);
uint8_t IrRemote_GetPinLevel(void);
uint32_t IrRemote_GetEdgeCount(void);
uint16_t IrRemote_GetLeadLowUs(void);
uint16_t IrRemote_GetLeadHighUs(void);
uint8_t IrRemote_GetFailStage(void);

#endif
