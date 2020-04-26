#ifndef _ED64IO_FLUSH_H
#define _ED64IO_FLUSH_H

#define ED64IO_FLUSH_STACKSIZE 0x20000

void ed64StartFlushThread(int mainThreadPri, int flushIntervalMS);

#endif /* _ED64IO_FLUSH_H */
