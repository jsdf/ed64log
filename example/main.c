
#include <nusys.h>
#include "stage00.h"

#include "ed64io_everdrive.h"

#include "ed64io_fault.h"
#include "ed64io_flush.h"
#include "ed64io_usb.h"

NUContData contdata[1];  // storage for controller 1 inputs

void mainproc(void) {
  // start everdrive communication
  evd_init();

  // start thread which will catch and log errors
  ed64StartFaultHandlerThread(NU_MAIN_THREAD_PRI + 1);
  ed64PrintfSync("=> after ed64StartFaultHandlerThread...\n");
  // ed64StartFlushThread(NU_MAIN_THREAD_PRI, 10);
  // ed64PrintfSync("=> after ed64StartFlushThread...\n");

  // initialize the graphics system
  nuGfxInit();

  // initialize the controller manager
  nuContInit();

  // initialize the level
  initStage00();

  // set the update+draw callback to be called every frame
  nuGfxFuncSet((NUGfxFunc)stage00);

  // enable video output
  nuGfxDisplayOn();

  // send this thread into an infinite loop
  while (1)
    ;
}
