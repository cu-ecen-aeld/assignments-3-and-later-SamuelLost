#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

    /*
    * TODO  add your code here
    *  Call the system() function with the command set in the cmd
    *   and return a boolean true if the system() call completed with success
    *   or false() if it returned a failure
    */
    if(cmd == NULL) {
        return false;
    }
    
    return (system(cmd) == 0) ? true : false;
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
    char * command[count + 1];
    int i;
    for(i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    /*
    * TODO:
    *   Execute a system command by calling fork, execv(),
    *   and wait instead of system (see LSP page 161).
    *   Use the command[0] as the full path to the command to execute
    *   (first argument to execv), and use the remaining arguments
    *   as second argument to the execv() command.
    *
    */


    fflush(stdout);
    printf("Forking\n");

    int status, exec_ret;
    pid_t child_pid;
    
    if ((child_pid = fork()) == -1) {
        printf("Fork failed\n");
        return false;
    } else if(child_pid == 0) {
        printf("Child process\n");
        if((exec_ret = execv(command[0], command)) == -1) {
            printf("Exec failed\n");
            _exit(1);
            // return false;
        }
    } else {
        // Parent process
        printf("Parent process waiting\n");
        if(waitpid(child_pid, &status, 0) == -1) {
            printf("Wait failed\n");
            return false;
        } else {
            if(WIFEXITED(status) != 0) {
                // Child process exited normally
                // Check the status to see if the child process ran
                printf("Child process exited normally\n");
                if(WEXITSTATUS(status) != 0) {
                    printf("Child process existed but did not run\n");
                    return false;
                }
            } else {
                printf("Child did not exit normally\n");
                return false;
            }
        }
        
    }

    va_end(args);
    return true;
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
    char *command[count + 1];
    int i;
    for(i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];
    va_end(args);

    /*
    * TODO
    *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
    *   redirect standard out to a file specified by outputfile.
    *   The rest of the behaviour is same as do_exec()
    *
    */
    int status, exec_ret;
    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd == -1) {
        printf("Open failed\n");
        return false;
    }

    fflush(stdout);
    printf("Forking\n");
    
    pid_t child_pid = fork();
    if(child_pid == -1) {
        printf("Fork failed\n");
        return false;
    } else if(child_pid == 0) {
        printf("Child process\n");
        if(dup2(fd, 1) < 0) {
            printf("Failed to duplicate file descriptor\n");
            _exit(1);
        }
        close(fd);
        if((exec_ret = execv(command[0], command)) == -1) {
            printf("Exec failed\n");
            _exit(1);
        }
    } else {
        // Parent process
        printf("Parent process waiting\n");
        if(waitpid(child_pid, &status, 0) == -1) {
            printf("Wait failed\n");
            return false;
        } else {
            if(WIFEXITED(status) != 0) {
                if(WEXITSTATUS(status) != 0) {
                    printf("Child process existed but did not run\n");
                    return false;
                }
            } else {
                printf("Child did not exit normally\n");
                return false;
            }
        }
        
    }

    return true;
}
