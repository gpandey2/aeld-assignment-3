#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/ip.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MY_SOCK_PATH "/tmp/aesdsocketfile"
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define LISTEN_BACKLOG 50
#define MALLOC_SIZE (1 << 15)

int server_fd, file_fd;
char *msg = NULL;

void cleanup_resources (int sig)
{
  syslog(LOG_ERR, "Caught signal, exiting\n");

  if (msg) {
    free(msg);
    msg = NULL;
  }

  if (close(server_fd) == -1) {
    syslog(LOG_ERR, "close() returned errno : %d %s\n", errno, strerror(errno));
  }

  if (close(file_fd) == -1) {
    syslog(LOG_ERR, "close() returned errno : %d %s\n", errno, strerror(errno));
  }

  if (unlink(MY_SOCK_PATH) == -1) {
    syslog(LOG_ERR, "unlink() returned errno : %d %s\n", errno, strerror(errno));
  }

  remove(FILE_PATH);
  closelog();
}

int main(int argc, char *argv[])
{
  if (argc > 2) {
    syslog(LOG_ERR, "More than 2 args not supported.\n");
  }

  signal(SIGINT, cleanup_resources);
  signal(SIGTERM, cleanup_resources);
  openlog(NULL, 0, LOG_USER); 

  int client_fd;
  int rc = 0;
  socklen_t client_addr_size;
  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
      syslog(LOG_ERR, "socket() returned errno : %d %s\n", errno, strerror(errno));
      return -1; 
  }

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(9000);
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  rc = bind(server_fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
  if (rc == -1) {
      syslog(LOG_ERR, "bind() returned errno : %d %s\n", errno, strerror(errno));
      cleanup_resources(server_fd);
      return -1; 
  }

  // Create the daemon when proper args are passed
  if (argc > 1) {
    if (!strncmp(argv[1], "-d", 2)) {
      pid_t pid = fork();
      if (pid < 0) {
        syslog(LOG_ERR, "fork() returned errno : %d %s\n", errno, strerror(errno));
        cleanup_resources(server_fd);
        return -1; 
      } else if (pid > 0) {
        // Terminate the parent
        cleanup_resources(server_fd);
        exit(0);
      } else {
        // Child process becomes the daemon
        if (setsid() < 0) exit(EXIT_FAILURE);
        pid = fork();

        if (pid < 0) {
          cleanup_resources(server_fd);
          exit(EXIT_FAILURE);
        }

        if (pid > 0) {
          // Exit from parent
          exit(EXIT_SUCCESS);
        }

        /* Set new file permissions */
        umask(0);

        /* Change the working directory to the root directory */
        chdir("/");

      }
    } else {
      syslog(LOG_ERR, "Invalid argument.\n");
    }
  }

  rc = listen(server_fd, LISTEN_BACKLOG);
  if (rc == -1) {
      syslog(LOG_ERR, "listen() returned errno : %d %s\n", errno, strerror(errno));
      cleanup_resources(server_fd);
      return -1; 
  }

  //sending and receiving
  msg = (char *)malloc(MALLOC_SIZE);
  if (msg == NULL) {
      syslog(LOG_ERR, "malloc() returned errno : %d %s\n", errno, strerror(errno));
      cleanup_resources(server_fd);
      return -1; 
  }

  ssize_t ret;
  file_fd = open(FILE_PATH, O_CREAT|O_RDWR);
  if (file_fd < 0) {
    cleanup_resources(server_fd);
    return -1;
  }

  while (1) {
    client_addr_size = sizeof(clientAddr);
    client_fd = accept(server_fd, (struct sockaddr *)&clientAddr, &client_addr_size);
    if (rc == -1) {
      syslog(LOG_ERR, "accept() returned errno : %d %s\n", errno, strerror(errno));
      cleanup_resources(server_fd);
      return -1; 
    }

    syslog(LOG_INFO, "Accepted connection from : %s\n", inet_ntoa(clientAddr.sin_addr));

    bzero((void *)msg, MALLOC_SIZE);
    rc = recv(client_fd, (void *)msg, MALLOC_SIZE, 0);
    if (rc == -1) {
      cleanup_resources(server_fd);
      return -1; 
    }

    // Break the loop once empty packet detected
    if (rc == 0) {
      break;
    }

    // Print to file
    ret = write(file_fd, msg, strlen(msg));

    bzero((void *)msg, sizeof(msg));
    rc = pread(file_fd, msg, MALLOC_SIZE, 0);

    rc = send(client_fd, msg, strlen(msg), 0);

    if (close(client_fd) == -1) {
      syslog(LOG_ERR, "close() returned errno : %d %s\n", errno, strerror(errno));
    }
  }

  return 0;
}
