#ifndef MCS51_MEMORY_H
#define MCS51_MEMORY_H

#if defined(__SDCC)
#define MCS51_XDATA __xdata
#define MCS51_CODE __code
#else
#define MCS51_XDATA
#define MCS51_CODE const
#endif

#endif
