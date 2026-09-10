#include "systemcalls.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    return (system(cmd) == -1) ? false : true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    pid_t p = fork();
    if (p < 0) {
        printf("Error in fork().");
        return false;
    }

    if (p == 0) {
        /* Child process */

        execv(command[0], command); 

        _exit(1);
    } else {
        int child_ret = 0;
        waitpid(p, &child_ret, 0);

        // Treat anything other than child returning true as false
        if (WIFEXITED(child_ret)) {
            return WEXITSTATUS(child_ret) == 0;
        }

        return false;
    }

    va_end(args);
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);

    pid_t p = fork();
    if (p < 0) {
        printf("Error in fork().");
        return false;
    }

    if (p == 0) {
        /* Child process */
        if (dup2(fd, 1) < 0) {
            perror("dup2");
            abort();
        }
        execv(command[0], command); 

        _exit(1);
    } else {
        int child_ret;
        waitpid(0, &child_ret, 0);

        // Treat anything other than child returning true as false
        if (WIFEXITED(child_ret)) {
            return WEXITSTATUS(child_ret) == 0;
        }

        return false;
    }

    va_end(args);
}
