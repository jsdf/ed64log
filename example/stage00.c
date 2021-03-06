
#include <assert.h>
#include <nusys.h>

#include "graphic.h"
#include "main.h"
#include "n64logo.h"
#include "stage00.h"

#include "ed64io.h"

#include <PR/os_internal.h>

Vec3d cameraPos = {-200.0f, -200.0f, -200.0f};
Vec3d cameraTarget = {0.0f, 0.0f, 0.0f};
Vec3d cameraUp = {0.0f, 1.0f, 0.0f};

#define NUM_SQUARES 5
struct Vec3d squares[NUM_SQUARES] = {
    {0.0f, 0.0f, 0.0f},   {0.0f, 0.0f, 0.0f},    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 200.0f}, {0.0f, 0.0f, -100.0f},
};

int squaresRotations[NUM_SQUARES];
int initSquaresRotations[NUM_SQUARES] = {
    0, 20, 40, 40, 40,
};

int squaresRotationDirection;

int showN64Logo;

static int frameCounter = 0;

void initStage00() {
  squaresRotationDirection = FALSE;
  showN64Logo = FALSE;

  osSyncPrintf("osSyncPrintf %s\n", "works");

  ed64StartWatchdogThread(&frameCounter, 500);

  {
    int i;
    for (i = 0; i < NUM_SQUARES; ++i) {
      squaresRotations[i] = initSquaresRotations[i];
    }
  }
}

void toggleRotationDirection() {
  squaresRotationDirection = !squaresRotationDirection;
  // Blocking (sync) log call. This will freeze the game for several
  // milliseconds (depending on I/O time), so this mainly makes sense for
  // debug logging where you just really want to get a message through to the
  // logger.
  ed64PrintfSync("reversing squaresRotationDirection\n");
}

void updateGame00() {
  frameCounter++;

  nuContDataGetEx(contdata, 0);

  if (contdata[0].trigger & A_BUTTON) {
    toggleRotationDirection();
  }

  if (contdata[0].button & B_BUTTON) {
    if (!showN64Logo) {
      ed64PrintfSync("showN64Logo = TRUE\n");
    }
    showN64Logo = TRUE;
  } else {
    showN64Logo = FALSE;
  }

  // each C button demonstrates an error handling feature
  if (contdata[0].trigger & L_CBUTTONS) {
    causeTLBFault();
  }
  if (contdata[0].trigger & R_CBUTTONS) {
    causeDivideByZeroException();
  }
  if (contdata[0].trigger & U_CBUTTONS) {
    causeOSError();
  }
  if (contdata[0].trigger & D_CBUTTONS) {
    hangThread();
  }

  if (contdata[0].trigger & D_JPAD) {
    // debugger example: sets a breakpoint in a function, then calls the
    // function, which should hit breakpoint and enter debugger
    ed64SetBreakpoint((u32*)&breakInThisFunction);
    breakInThisFunction();
  }

  {
    int i;
    for (i = 0; i < NUM_SQUARES; ++i) {
      squaresRotations[i] =
          (squaresRotations[i] + (squaresRotationDirection ? 1 : -1)) % 360;
    }
  }

  if (nuScRetraceCounter % 60 == 0) {
    // Non-blocking (async) log call, suitable for frequent logging
    ed64Printf("retrace=%d\n", (int)nuScRetraceCounter);
  }

  // ed64AsyncLoggerFlush() needs to be called on a regular interval to try to
  // send any pending async (ed64Printf()) logs to the host via USB (if it's not
  // busy). If you call this every frame you  can log about 128k of logging text
  // every few frames. You can call it more frequently than once per frame, but
  // subsequent calls need to be spaced out across the frame time, otherwise
  // they won't do anything, because the RAM-to-Everdrive and
  // Everdrive-to-USB-Host I/O takes time. If you only use synchronous logging
  // (ed64PrintfSync()), you don't need to call this function.
  ed64AsyncLoggerFlush();
}

void makeDL00() {
  unsigned short perspNorm;
  GraphicsTask* gfxTask;

  gfxTask = gfxSwitchTask();

  gfxRCPInit();

  gfxClearCfb();

  guPerspective(&gfxTask->projection, &perspNorm, FOVY, ASPECT, NEAR_PLANE,
                FAR_PLANE, 1.0);

  gSPPerspNormalize(displayListPtr++, perspNorm);

  guLookAt(&gfxTask->modelview, cameraPos.x, cameraPos.y, cameraPos.z,
           cameraTarget.x, cameraTarget.y, cameraTarget.z, cameraUp.x,
           cameraUp.y, cameraUp.z);

  gSPMatrix(displayListPtr++, OS_K0_TO_PHYSICAL(&(gfxTask->projection)),
            G_MTX_PROJECTION | G_MTX_LOAD |

                G_MTX_NOPUSH

  );

  gSPMatrix(displayListPtr++, OS_K0_TO_PHYSICAL(&(gfxTask->modelview)),
            G_MTX_MODELVIEW | G_MTX_NOPUSH | G_MTX_LOAD);

  {
    int i;
    Vec3d* square;
    for (i = 0; i < NUM_SQUARES; ++i) {
      square = &squares[i];
      guPosition(&gfxTask->objectTransforms[i],

                 squaresRotations[i], 0.0f, 0.0f, 1.0f,

                 square->x, square->y, square->z);

      gSPMatrix(displayListPtr++,
                OS_K0_TO_PHYSICAL(&(gfxTask->objectTransforms[i])),
                G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL

      );

      if (showN64Logo) {
        drawN64Logo();
      } else {
        drawSquare();
      }

      gSPPopMatrix(displayListPtr++, G_MTX_MODELVIEW);
    }
  }

  gDPFullSync(displayListPtr++);
  gSPEndDisplayList(displayListPtr++);

  assert(displayListPtr - gfxTask->displayList < MAX_DISPLAY_LIST_COMMANDS);

  nuGfxTaskStart(gfxTask->displayList,
                 (int)(displayListPtr - gfxTask->displayList) * sizeof(Gfx),
                 NU_GFX_UCODE_F3DEX, NU_SC_SWAPBUFFER);
}

Vtx squareVerts[] __attribute__((aligned(16))) = {

    {-64, 64, -5, 0, 0, 0, 0x00, 0xff, 0x00, 0xff},
    {64, 64, -5, 0, 0, 0, 0x00, 0x00, 0x00, 0xff},
    {64, -64, -5, 0, 0, 0, 0x00, 0x00, 0xff, 0xff},
    {-64, -64, -5, 0, 0, 0, 0xff, 0x00, 0x00, 0xff},
};

void drawSquare() {
  gSPVertex(displayListPtr++, &(squareVerts[0]), 4, 0);

  gDPSetCycleType(displayListPtr++, G_CYC_1CYCLE);

  gDPSetRenderMode(displayListPtr++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);

  gSPClearGeometryMode(displayListPtr++, 0xFFFFFFFF);

  gSPSetGeometryMode(displayListPtr++, G_SHADE | G_SHADING_SMOOTH | G_ZBUFFER);

  gSP2Triangles(displayListPtr++, 0, 1, 2, 0, 0, 2, 3, 0);

  gDPPipeSync(displayListPtr++);
}

void drawN64Logo() {
  gDPSetCycleType(displayListPtr++, G_CYC_1CYCLE);
  gDPSetRenderMode(displayListPtr++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
  gSPClearGeometryMode(displayListPtr++, 0xFFFFFFFF);
  gSPSetGeometryMode(displayListPtr++, G_SHADE | G_SHADING_SMOOTH | G_ZBUFFER);

  gSPDisplayList(displayListPtr++, N64Yellow_PolyList);
  gSPDisplayList(displayListPtr++, N64Red_PolyList);
  gSPDisplayList(displayListPtr++, N64Blue_PolyList);
  gSPDisplayList(displayListPtr++, N64Green_PolyList);

  gDPPipeSync(displayListPtr++);
}

void stage00(int pendingGfx) {
  if (pendingGfx < 1)
    makeDL00();

  updateGame00();
}

// functions which demonstrate various error handling capabilities

void breakInThisFunction() {
  // asm("break  0");
  int i;
  int data[10];
  data[0] = 0;
  data[1] = 1;
  for (i = 2; i < 10; ++i) {
    data[i] = data[i - 2] + data[i - 1];
  }
}

void causeTLBFault() {
  ed64PrintfSync(
      "intentionally causing TLB exception on load/instruction fetch "
      "fault\n");
  {
    long e1;
    e1 = *(long*)1;  // TLB exception on load/instruction fetch
  }
}

void causeDivideByZeroException() {
  ed64PrintfSync("intentionally causing float divide-by-zero fault\n");

  {
    // Fetch the current floating-point control/status register
    u32 fpstat = __osGetFpcCsr();
    // Enable divide-by-zero exception for floating point, so the fault
    // handler thread can log float divide-by-zero errors
    __osSetFpcCsr(fpstat | FPCSR_EZ);
  }

  {
    float zero = 0;
    ed64PrintfSync("result=%f\n", 1 / zero);
  }
}

void hangThread() {
  while (1) {
  }
}

void causeOSError() {
  // cause an os error
  osSetThreadPri(&nuGfxThread, /*invalid thread priority*/ -1);
}
