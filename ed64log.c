/* ed64log.c
   libftdi Everdrive64 USB logging tool
*/

#include <stdio.h>
#ifdef __APPLE__
#include <libftdi1/ftdi.h>
#else
#include <ftdi.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ed64log.h"
#include "gopt.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define __WINDOWS__
#endif

#ifndef __WINDOWS__
volatile sig_atomic_t stop;
#else
int stop = 0;
#endif

void inthand(int signum) {
  stop = 1;
}

int main(int argc, const char** argv) {
#ifndef __WINDOWS__
  signal(SIGINT, inthand);
#endif

  // usb buffer
  char recv_buff[512];

  int arg_fail = 0;

  // rom filename
  const char* filename;

  // verbosity level 0-2
  int verbosity;
  int i, print_help;

  void* options = gopt_sort(
      &argc, argv,
      gopt_start(
          gopt_option('h', 0, gopt_shorts('h'), gopt_longs("help")),
          gopt_option('z', 0, gopt_shorts('z'), gopt_longs("version")),
          gopt_option('v', GOPT_REPEAT, gopt_shorts('v'),
                      gopt_longs("verbose"))
          ));

  if (gopt(options, 'h')) {
    fprintf(stdout, "Syntax: sudo ./ed64log [options] ...\n\n");
    fprintf(stdout, "ed64log - Everdrive64 USB logging tool\n");
    fprintf(stdout, " -h, --help\t\tdisplay this help and exit\n");
    fprintf(stdout, " -v, --verbose\t\tverbose\n");
    fprintf(stdout, " -z, --version\t\tversion\n");

    exit(EXIT_SUCCESS);
  }

  // show info without options
  print_help = 0;

  if (gopt(options, 'h')) {
    print_help = 1;
  }

  if (gopt(options, 'z')) {
    fprintf(stdout, "loader64 version v%d.%d\n", MAJOR_VERSION, MINOR_VERSION);
    exit(EXIT_SUCCESS);
  }

  verbosity = gopt(options, 'v');

  if (verbosity > 1)
    fprintf(stderr, "being really verbose\n");

  else if (verbosity)
    fprintf(stderr, "being verbose\n");

  // options are ok - init ftdi
  if (!arg_fail) {
    int ret;
    int ret_s;
    int ret_r;
    struct ftdi_context* ftdi;
    if ((ftdi = ftdi_new()) == 0) {
      fprintf(stderr, "ftdi_new failed\n");
      return EXIT_FAILURE;
    }

    if ((ret = ftdi_usb_open(ftdi, USB_VENDOR, USB_DEVICE)) < 0) {
      fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret,
              ftdi_get_error_string(ftdi));
      ftdi_free(ftdi);
      return EXIT_FAILURE;
    } else {
      // read/write timeout e.g. 500ms
      ftdi->usb_read_timeout = USB_READ_TIMEOUT;
      ftdi->usb_write_timeout = USB_WRITE_TIMEOUT;
    }

    if (ftdi->type == TYPE_R) {
      unsigned int chipid;
      if (verbosity >= 1) {
        printf("ftdi_read_chipid: %d\n", ftdi_read_chipid(ftdi, &chipid));
        printf("FTDI chipid: %X\n", chipid);
      }
    }

    // init usb transfer buffer
    memset(recv_buff, 0, 512);
 

    if (verbosity >= 1)
      printf("start logging\n");

    while (!stop) {
      memset(recv_buff, 0, 512);
      ret_s = 0;
      ret_r = 0;
 
      ret_r = ftdi_read_data(ftdi, recv_buff, 512);

      if (verbosity >= 1)
        printf("recv: %i bytes\n", ret_r);

      if (ret_r > 0) {
        int chIdx;
        for (chIdx = 0; chIdx < 512; ++chIdx) {
          if (recv_buff[chIdx] == '\0')
            break;
          printf("%c", recv_buff[chIdx]);
        } 

        fflush(stdout); // Will now print everything in the stdout buffer
      }


      usleep(500);
    }

    memset(recv_buff, 0, 512);
    ret_s = 0;
    ret_r = 0;

    if ((ret = ftdi_usb_close(ftdi)) < 0) {
      fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret,
              ftdi_get_error_string(ftdi));
      ftdi_free(ftdi);
      return EXIT_FAILURE;
    }

    ftdi_free(ftdi);
  }

  if (print_help) {
    printf("%s: missing operand\n", argv[0]);
    printf("Try '%s --help' for more information.\n", argv[0]);
  }

  gopt_free(options);
  return EXIT_SUCCESS;
}
