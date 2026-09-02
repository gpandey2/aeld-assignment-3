#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
  
  openlog(NULL, 0, LOG_USER); 

  if (argc < 3) {
    syslog(LOG_ERR, "Not enough arguments.");
    return 1;
  }

  int fd = open(argv[1], O_CREAT|O_WRONLY); 
  if (fd < 0) {
    syslog(LOG_ERR, "open() returned rc : %d %s", errno, strerror(errno));
    return -1;
  }

  syslog(LOG_ERR, "Writing %s to %s", argv[2], argv[1]);
  
  ssize_t ret = write(fd, (void *)argv[2], strlen(argv[2]));
  if (ret < strlen(argv[2])) {
    syslog(LOG_ERR, "write returned rc : %d %s", errno, strerror(errno));
  }

  close(fd);
  return 0;
}
