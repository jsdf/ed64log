
#include <nusys.h>
#include "stage00.h"

#include "ed64io_everdrive.h"

#include "ed64io_fault.h"
#include "ed64io_os_error.h"
#include "ed64io_usb.h"

NUContData contdata[1];  // storage for controller 1 inputs

void mainproc(void) {
  // start everdrive communication
  evd_init();

  // register libultra error handler
  ed64RegisterOSErrorHandler();

  ed64ReplaceOSSyncPrintf();

  // start thread which will catch and log errors
  ed64StartFaultHandlerThread(NU_GFX_TASKMGR_THREAD_PRI);

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
