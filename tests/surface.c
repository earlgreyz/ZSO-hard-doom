#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "../include/doomdev.h"

static int errors = 0;

int main(int argc, char *argv[]) {
  int doom_fd = open("/dev/doom0", O_RDWR);
  if (doom_fd < 0) {
    fprintf(stderr, "Couldn't open /dev/doom0\n");
    return 1;
  }

  struct doomdev_ioctl_create_surface surface_params = {
    .width = 1024,
    .height = 1024,
  };

  int surface_fd = ioctl(doom_fd, DOOMDEV_IOCTL_CREATE_SURFACE, (unsigned long) &surface_params);
  if (surface_fd < 0) {
    perror("Couldn't create surface");
    errors++;
  } else {
    fprintf(stderr, "Surface successfully created\n");
    close(surface_fd);
  }

  surface_params = (struct doomdev_ioctl_create_surface){
    .width = 2048 + 64,
    .height = 1024,
  };

  surface_fd = ioctl(doom_fd, DOOMDEV_IOCTL_CREATE_SURFACE, (unsigned long) &surface_params);
  if (surface_fd > 0) {
    fprintf(stderr, "Surface width overflow not detected\n");
    errors++;
    close(surface_fd);
  } else if (errno != EOVERFLOW) {
    fprintf(stderr, "Surface width overflow detected, but returned invalid error code\n");
    errors++;
  } else {
    fprintf(stderr, "Surface width overflow successfully detected\n");
  }

  surface_params = (struct doomdev_ioctl_create_surface){
    .width = 1024,
    .height = 2048 + 64,
  };

  surface_fd = ioctl(doom_fd, DOOMDEV_IOCTL_CREATE_SURFACE, (unsigned long) &surface_params);
  if (surface_fd > 0) {
    fprintf(stderr, "Surface height overflow not detected\n");
    errors++;
    close(surface_fd);
  } else if (errno != EOVERFLOW) {
    fprintf(stderr, "Surface height overflow detected, but returned invalid error code\n");
    errors++;
  } else {
    fprintf(stderr, "Surface height overflow successfully detected\n");
  }

  surface_params = (struct doomdev_ioctl_create_surface){
    .width = 1020,
    .height = 1024,
  };

  surface_fd = ioctl(doom_fd, DOOMDEV_IOCTL_CREATE_SURFACE, (unsigned long) &surface_params);
  if (surface_fd > 0) {
    fprintf(stderr, "Surface width alignment error not detected\n");
    errors++;
    close(surface_fd);
  } else if (errno != EINVAL) {
    fprintf(stderr, "Surface width alignment error detected, but returned invalid error code\n");
    errors++;
  } else {
    fprintf(stderr, "Surface width alignment error successfully detected\n");
  }

  surface_params = (struct doomdev_ioctl_create_surface){
    .width = 60,
    .height = 1024,
  };

  surface_fd = ioctl(doom_fd, DOOMDEV_IOCTL_CREATE_SURFACE, (unsigned long) &surface_params);
  if (surface_fd > 0) {
    fprintf(stderr, "Surface width underflow error not detected\n");
    errors++;
    close(surface_fd);
  } else if (errno != EINVAL) {
    fprintf(stderr, "Surface width underflow error detected, but returned invalid error code\n");
    errors++;
  } else {
    fprintf(stderr, "Surface width underflow error successfully detected\n");
  }

  surface_params = (struct doomdev_ioctl_create_surface){
    .width = 1020,
    .height = 0,
  };

  surface_fd = ioctl(doom_fd, DOOMDEV_IOCTL_CREATE_SURFACE, (unsigned long) &surface_params);
  if (surface_fd > 0) {
    fprintf(stderr, "Surface height underflow error not detected\n");
    errors++;
    close(surface_fd);
  } else if (errno != EINVAL) {
    fprintf(stderr, "Surface height underflow error detected, but returned invalid error code\n");
    errors++;
  } else {
    fprintf(stderr, "Surface height underflow error detected\n");
  }

  close(doom_fd);
  if (errors > 0) {
    fprintf(stderr, "%d tests failed\n", errors);
    return 1;
  } else {
    fprintf(stderr, "All tests were successful\n");
    return 0;
  }
}
