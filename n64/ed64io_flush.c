
#include "ed64io_everdrive.h"
#include "ed64io_sys.h"
#include "ed64io_types.h"
#include "ed64io_usb.h"

#include "ed64io_flush.h"

static OSMesgQueue flushMsgQ;
static OSMesg flushMsgBuf;
static OSThread flushThread;
static char flushThreadStack[ED64IO_FLUSH_STACKSIZE];

OSTimer flushTimer;
int flushingLock = 0;

static void flushproc(char* argv) {
  while (1) {
    (void)osRecvMesg(&flushMsgQ, NULL, OS_MESG_BLOCK);
    // prevent it from interrupting itself
    if (!flushingLock) {
      flushingLock = 1;
      ed64AsyncLoggerFlush();
      flushingLock = 0;
    }
  }
}

// mainThreadPri: the priority of your main thread. if it's higher the
// game will hang. if it's lower, no logs will be flushed. for NuSystem games
// you can use NU_MAIN_THREAD_PRI

// flushIntervalMS: the interval (in
// milliseconds) on which to call ed64AsyncLoggerFlush. should be >= 10
// otherwise your game will hang
void ed64StartFlushThread(int mainThreadPri, int flushIntervalMS) {
  osCreateMesgQueue(&flushMsgQ, &flushMsgBuf, 1);
  osCreateThread(&flushThread, /*id*/ 24, (void (*)(void*))flushproc,
                 /*argv*/ NULL, flushThreadStack + ED64IO_FLUSH_STACKSIZE / 8,
                 /*priority*/ (OSPri)mainThreadPri);

  osStartThread(&flushThread);
  osSetTimer(&flushTimer, OS_CYCLES_TO_USEC(flushIntervalMS * 1000),
             OS_CYCLES_TO_USEC(flushIntervalMS * 1000), &flushMsgQ, NULL);
}
