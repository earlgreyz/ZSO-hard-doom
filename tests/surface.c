#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "../include/doomdev.h"

int main(int argc, char *argv[]) {
  int doom_fd = open("/dev/doom0", O_RDWR);
  if (doom_fd < 0) {
    perror("Couldn't open /dev/doom0");
    return 1;
  }

  struct doomdev_ioctl_create_surface surface_params = {
    .width = 1024,
    .height = 1024,
  };

  int surface_fd = ioctl(doom_fd, DOOMDEV_IOCTL_CREATE_SURFACE, (unsigned long) &surface_params);
  if (surface_fd < 0) {
    perror("Couldn't create surface");
    return 1;
  }

  fprintf(stderr, "Surface successfully created\n");
  return 0;
}
