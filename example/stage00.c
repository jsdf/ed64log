
#include <assert.h>
#include <nusys.h>

#include <PR/os_internal.h>

#include "graphic.h"
#include "main.h"
#include "n64logo.h"
#include "stage00.h"

#include "ed64io_usb.h"

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

void initStage00() {
  squaresRotationDirection = FALSE;
  showN64Logo = FALSE;

  {
    int i;
    for (i = 0; i < NUM_SQUARES; ++i) {
      squaresRotations[i] = initSquaresRotations[i];
    }
  }
}

void updateGame00() {
  nuContDataGetEx(contdata, 0);
  if (contdata[0].trigger & A_BUTTON) {
    squaresRotationDirection = !squaresRotationDirection;
    // Blocking (sync) log call. This will freeze the game for several
    // milliseconds (depending on I/O time), so this mainly makes sense for
    // debug logging where you just really want to get a message through to the
    // logger.
    ed64PrintfSync("reversing squaresRotationDirection\n");
  }

  if (contdata[0].button & B_BUTTON) {
    if (!showN64Logo) {
      ed64PrintfSync("showN64Logo = TRUE\n");
    }
    showN64Logo = TRUE;
  } else {
    showN64Logo = FALSE;
  }

  if (contdata[0].trigger & START_BUTTON) {
    u32 fpstat;

    /*
     * Fetch the current floating-point control/status register
     * Enable divide-by-zero exception for floating point
     */
    fpstat = __osGetFpcCsr();
    __osSetFpcCsr(fpstat | FPCSR_EZ);
    ed64PrintfSync("intentionally causing a fault\n");
    {
      long e1;
      e1 = *(long*)1;  // TLB exception on load/instruction fetch
    }
    // {
    //   float zero = 0;
    //   ed64PrintfSync("d=%d", 1 / zero);
    // }
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
  // ed64AsyncLoggerFlush();
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
