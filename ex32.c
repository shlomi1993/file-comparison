// Shlomi Ben-Shushan

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

// Defines status codes.
#define SUCCESS     0
#define IDENTICAL   1
#define SIMILAR     3
#define DIFFERENT   2
#define FAILURE     4   // A failure is a possible result, and does not require to exit the program.
#define NOT_FOUND   5
#define ERROR       -1
#define TIMED_OUT   124 // Timeout return 124 if program timed-out.

// Defines maximum sizes for conf.txt file reading buffer and system path size.
#define PATH_MAX    4096
#define CONF_MAX    600  // 150 chars * 3 lines + a little bit extra.

// Defines relative file locations.
#define BINARY  "./b.out"
#define OUTPUT  "./output.txt"
#define RESULTS "./results.csv"
#define ERRORS  "./errors.txt"
#define COMP    "./comp.out"

/**********************************************************************************
* Function:     print
* Input:        a string (const char *).
* Output:       0 for success, -1 for error.
* Operation:    Prints msg to the teminal using stdout file descriptor (1).
***********************************************************************************/
int print(const char *msg) {
    if (write(1, msg, strlen(msg)) == ERROR) {
        return ERROR;
    }
    return SUCCESS;
}

/**********************************************************************************
* Function:     writeToCSV
* Input:        Folder's (student's) name, stringed grade, a reason for the grade.
* Output:       0 for success, -1 for error.
* Operation:    This function creates a line of comma-seperated details and append
*               it to result.csv file (creates file if not exists).
***********************************************************************************/
int writeToCSV(const char *name, const char *grade, const char *reason) {

    // Open the file in write-only-append mode -- create if not exist.
    int csv = open(RESULTS, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (csv == ERROR) {
        print("Error in: open\n");
        return ERROR;
    }

    // Create a comma-seperated line ends with line-break.
    char string[strlen(name) + strlen(grade) + strlen(reason) + 4];
    strcpy(string, name);
    strcat(string, ",");
    strcat(string, grade);
    strcat(string, ",");
    strcat(string, reason);
    strcat(string, "\n");

    // Write to results.txt file, handle error if occured.
    if (write(csv, string, strlen(string)) == ERROR) {
        return ERROR;
    }
    
    // Release FD.
    if (close(csv) != SUCCESS) {
        print("Error in: close\n");
        return ERROR;
    }

    // If all good, return SUCCESS.
    return SUCCESS;
    
}

/**********************************************************************************
* Function:     ioRedirection
* Input:        File path (with name) and an int which represents the desired FD.
* Output:       0 for success, -1 for error.
* Operation:    If newFD is 1 or 2, the function redirect output/errors to the
*               given file. If newFD is 0, the it redirect the input from the given
*               file. If something went wrong or newFD is not 0, 1 nor 2, the
*               the function returns -1.
***********************************************************************************/
int ioRedirection(const char *file, int newFD) {

    // Output and errors redirection.
    if (newFD == 1 || newFD == 2) {
        int dstFD = open(file, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (dstFD == ERROR) {
            print("Error in: open\n");
            return ERROR;
        }
        if (dup2(dstFD, newFD) == ERROR) {
            close(dstFD);
            print("Error in: dup2\n");
            return ERROR;
        }
        if (close(dstFD) == ERROR) {
            print("Error in: close\n");
            return ERROR;
        }
        return SUCCESS;
    }

    // Input redirection.
    if (newFD == 0) {
        int inFD = open(file, O_RDONLY);
        if (inFD == ERROR) {
            print("Error in: open\n");
            return ERROR;
        }
        if (dup2(inFD, newFD) == ERROR) {
            print("Error in: dup2\n");
            close(inFD);
            return ERROR;
        }
        if (close(inFD) == ERROR) {
            print("Error in: close\n");
            return ERROR;
        }
        return SUCCESS;
    }

    // Illigal newFD.
    return ERROR;
    
}

/**********************************************************************************
* Function:     safeRemove
* Input:        String -- a path to a file (include its name).
* Output:       0 if the file is removed or not exists, -1 if an error occured.
* Operation:    Safely remove a file and notify if something went wrong.
***********************************************************************************/
int safeRemove(const char *fileName) {

    // Check if the file exists.
    if (access(fileName, F_OK) == SUCCESS) {

        // If so remove it (return -1 if something wrong happened).
        if (remove(fileName) != SUCCESS) {
            print("Error in: remove\n");
            return ERROR;
        }
    }

    // Return 0 for success.
    return SUCCESS;

}

/**********************************************************************************
* Function:     execute
* Input:        Arguments for execvp() and a path for an input file (maybe NULL).
* Output:       0 for success, -1 for error, ex31.c return value, or 124 for time-out.
* Operation:    Uses fork() and execvp() to run the given command. Note that the 
*               child is responsible for IO redirection and the parent is
*               responsible for getting the output/timeout of the execution.
***********************************************************************************/
int execute(char **command, const char *inputFile) {

    // Fork in order to call a bash command without loosing the memory allocated for this program.
    pid_t pid = fork();

    // Child executing process.
    if (pid == 0) {

        // Redirect output to output.txt (temp) file and errors to errors.txt file.
        if (ioRedirection(ERRORS, 2) == ERROR || ioRedirection(OUTPUT, 1) == ERROR) {
            return ERROR;
        }

        // Redirect input from the given inputFile (if given).
        if (inputFile != NULL && ioRedirection(inputFile, 0)) {
            return ERROR;
        }   

        // User execvp() to execute command.
        if (execvp(command[0], command) != SUCCESS) {
            print("Error in: execvp\n");
            return ERROR;
        }      
    }

    // Parent waiting process.
    else if (pid > 0) {

        // Wait for the child to finish.
        int status;
        if (waitpid(pid, &status, WUNTRACED) == ERROR) {
            print("Error in: waitpid\n");
            return ERROR;
        }

        // If child is timed-out, return 4 for failure.
        if (inputFile != NULL && WIFEXITED(status) && WEXITSTATUS(status) == TIMED_OUT) {
            return TIMED_OUT;
        }
        
        // If chiled has finished on time, check exit-status and return it.
        if (!strcmp(command[0], COMP) && WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }

    }

    // Case pid < 0 means fork() failed so return -1 for error.
    else if (pid < 0) {
        print("Error in: fork\n");
        return ERROR;
    }

    // Returns 0 for success.
    return SUCCESS;

}

/**********************************************************************************
* Function:     checkExtension
* Input:        fileName - a string which represent a file name.
* Output:       0 for success, 4 for failure
* Operation:    Look for a sequence of chars: ".c\0" or: ".C\0" and return 0 for
*               success. If couldn't find a sequence, return 4 for failure.
***********************************************************************************/
int checkExtension(const char *fileName) {

    // Variables for a loop.
    int i, lim = strlen(fileName);

    // This loop checks the extension of the given fileName. 
    for (i = 0; i < lim; ++i) {
        if (fileName[i] == '.') {
            if (fileName[i + 1] == 'c' || fileName[i + 1] == 'C') {
                if (fileName[i + 2] == '\0') {
                    return SUCCESS;
                }
            }
        }
    }

    // If the file is have no C extension, return 4 for failure.
    return FAILURE;

}

/**********************************************************************************
* Function:     findAndCompile
* Input:        Current directory name and the path to current directory.
* Output:       0 for success, 4 for failure, -1 for significant error.
* Operation:    This function traverse the the directory entities inside the
*               current directory name and look for a C file with checkExtension()
*               function. Once found, it uses execute() function to compile it.
*               It also uses writeToCSV() function to .. write to CSV when needed. 
***********************************************************************************/
int findAndCompile(const char *name, const char *directoryPath) {

    // Initialize status variable to "not found". It will change if a C file is found.
    char status = NOT_FOUND;

    // Open current directory.
    DIR *dir = opendir(directoryPath);
    if (!dir) {
        print("Error in: opendir\n");
        return ERROR;
    }
    
    // This loop looks for the C file in the current directory.
    struct dirent *dirEnt;
    while ((dirEnt = readdir(dir)) != NULL) {

        // Check file's extension.
        if (checkExtension(dirEnt->d_name) == SUCCESS) {

            // Creates a path to the C file.
            char path[PATH_MAX] = {0};
            strcpy(path, directoryPath);
            strcat(path, "/");
            strcat(path, dirEnt->d_name);

            // Once found the '.c' file, create arguments for execute() function.
            char *command[] = {"gcc", "-o", BINARY, path, NULL};

            // Compile the C file using execute() function (that uses fork() and execvp()).
            status = execute(command, NULL);

            // If execution failed, relate as compilation error and return 4 for failure. If something went
            // wrong, return -1 for error.
            if (status != SUCCESS) {
                if (closedir(dir)) {
                    print("Error in: closedir\n");
                    return ERROR;
                }
                if (writeToCSV(name, "10" ,"COMPILATION_ERROR") == ERROR) {
                    return ERROR;
                }
                return FAILURE;
            }

            // Else, execution was success, so verify the existance of the binary file.
            if (access(BINARY, F_OK) == SUCCESS) {
                if (closedir(dir)) {
                    print("Error in: closedir\n");
                    return ERROR;
                }
                return SUCCESS;
            }

        } 
    }

    // Close directory (return -1 if failed).
    if (closedir(dir) == ERROR) {
        print("Error in: closedir\n");
        return ERROR;
    }

    // Handle case C file not found.
    if (status == NOT_FOUND) {
        if (writeToCSV(name, "0" ,"NO_C_FILE") == ERROR) {
            return ERROR;
        }
        return FAILURE;
    }

    // Handle the edge-case where C file was found, execution was successful, but for some
    // reason the binary file is not created.
    if (writeToCSV(name, "10" ,"COMPILATION_ERROR") == ERROR) {
        return ERROR;
    }

    // Return 4 for failure for any other unexpected case.
    return FAILURE;
    
}

/**********************************************************************************
* Function:     runProgram
* Input:        Current directory name and path to input file.
* Output:       Return 0 for success, -1 for error or 124 for time-out.
* Operation:    This function creates the arguments to run the compiled program in
*               the current sub-directory (if found and successfully compiled).
*               then it run it and return status. It write to the CSV if timed-out.
***********************************************************************************/
int runProgram(const char *name, const char *inputFile) {

    // Create command for execvp() -- set timeout for 5 seconds.
    char *command[] = {"timeout", "5s", BINARY, NULL};

    // Compile program using execute() function (that uses fork() and execvp()).
    int status = execute(command, inputFile);

    // Write to the CSV if operation was timed-out.
    if (status == TIMED_OUT) {
        if (writeToCSV(name, "20", "TIMEOUT") == ERROR) {
            return ERROR;
        }
    }

    // Return 0 for success, -1 for error and 4 for failure.
    return status;

}

/**********************************************************************************
* Function:     testOutput
* Input:        Current file's name and sharedAccess.
* Output:       The return value of ex31.c (1, 2 or 3), or -1 for error.
* Operation:    This function tests the output using ex31.c program and write
*               write the result to results.csv file. The function return ex31.c
*               return value or -1 if an error occured.
***********************************************************************************/
int testOutput(const char *name, const char *correctOutputFile) {

    // Make a copy to avoid 'const' warning message.
    char correctFileCopy[strlen(correctOutputFile)];
    strcpy(correctFileCopy, correctOutputFile);

    // Create command for execvp() -- to use comp.out (ex31.c) program.
    char *command[] = {COMP, OUTPUT, correctFileCopy, NULL};

    // Test using execute().
    int status = execute(command, NULL);

    // Write result to results.csv and return status code.
    // Return -1 for error in case of an unexpected result.
    switch (status) {
        case IDENTICAL: return writeToCSV(name, "100", "EXCELLENT");
        case DIFFERENT: return writeToCSV(name, "50", "WRONG");
        case SIMILAR:   return writeToCSV(name, "75", "SIMILAR");
        default:        return ERROR;
    }
    
}

/**********************************************************************************
* Function:     runTest
* Input:        Target directory, input file location, and output file location.
* Output:       0 for success, -1 for failure.
* Operation:    This is the main test function. It verifies existance of necessary
*               files, open the target directory, and for each directory entry it
*               finds, it does 3 things:
*                   1) Look for a C file and compile it (with findAndCompile()).
*                   2) Try to run the binary for 5 seconds (with runProgram()).
*                   3) Compare the output with the correct onr (with testOutput()).
*               Each and every operation is checked, and the function returns -1
*               if any significant error occured.
***********************************************************************************/
int runTest(const char *target, const char *inputFile, const char *correctOutputFile) {
    
    // Try to open the target directory.
    DIR *dir = opendir(target);
    if (!dir) {
        print("Not a valid directory\n");
        return ERROR;
    }

    // Initialize status flag variable.
    int status;

    // Traverse sub-directories and do the job.
    struct dirent *dirEnt;
    while ((dirEnt = readdir(dir)) != NULL) {

        // Save directory name in a variable for convenience reason.
        const char *name = dirEnt->d_name;

        // Avoid current and previous directories references.
        if (!strcmp(name, ".") || !strcmp(name, "..")) {
            continue;
        }

        // Create a path to the next directory.
        char path[PATH_MAX];
        strcpy(path, target);
        strcat(path, "/");
        strcat(path, dirEnt->d_name);

        // Create stat to identify directory entry type.
        struct stat pathStat;
        if (stat(path, &pathStat) == ERROR) {
            return ERROR;
        }

        // Engage only directories.
        if (!S_ISDIR(pathStat.st_mode)) {
            continue;
        }

        // Look for a C file in the sub-directory and compile it. Return Error (-1) if needed. 
        status = findAndCompile(name, path);
        if (status == ERROR) {
            closedir(dir);
            return ERROR;
        }

        // If a C file was found and successfully compiled, try to run its binary with the given input file.
        if (status == SUCCESS) {
            status = runProgram(name, inputFile);
            if (status == ERROR) {
                closedir(dir);
                return ERROR;
            }
            // If the program ran succesfully, compare its output to the given correct output file.
            if (status == SUCCESS) {
                status = testOutput(name, correctOutputFile);
                if (status == ERROR) {
                    closedir(dir);
                    return ERROR;
                }
            }
        }

        // Cleanup - remove redundent files.
        if (safeRemove(BINARY) == ERROR || safeRemove(OUTPUT) == ERROR) {
            return ERROR;
        }

    }

    // Close target directory.
    if (closedir(dir) == ERROR) {
        return ERROR;
    }

    // Return 0, which means success.
    return SUCCESS;
    
}

/**********************************************************************************
* Function:     setupChecks
* Input:        Target directory, input file location, and output file location.
* Output:       0 for success, -1 for failure.
* Operation:    This function remove old errors.txt and results.csv files and
*               verifies that the given target directory, input file, and correct
*               output file are exists and from the correct types.
***********************************************************************************/
int setupChecks(const char *directory, const char *input, const char *correct) {

    // Remove old files if exists.
    if (safeRemove(ERRORS) == ERROR || safeRemove(RESULTS) == ERROR) {
        return ERROR;
    }

    // Create stat to identify entry type.
    struct stat entry;

    // Verify target dictionary existance and type.
    if (access(directory, F_OK) != SUCCESS || stat(directory, &entry) == ERROR || !S_ISDIR(entry.st_mode)) {
        print("Not a valid directory\n");
        return ERROR;
    }

    // Verify input file existance and type.
    if (access(input, F_OK) != SUCCESS || stat(input, &entry) == ERROR || !S_ISREG(entry.st_mode)) {
        print("Input file not exist\n");
        return ERROR;
    }

    // Verify output file existance.
    if (access(correct, F_OK) != SUCCESS|| stat(correct, &entry) == ERROR || !S_ISREG(entry.st_mode)) {
        print("Output file not exist\n");
        return ERROR;
    }

    // Return 0 if all checked.
    return SUCCESS;

}

/**********************************************************************************
* Function:     main
* Input:        argc, argv -- standard input.
* Output:       0 if finished properly, or exit with code -1 if a problem occured.
* Operation:    Entry point of the program.
***********************************************************************************/
int main(int argc, char **argv) {

    // Open configuration file.
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        print("Error in: open\n");
        exit(ERROR);
    }

    // Set buffer and read configuration file.
    char buffer[CONF_MAX];
    if (read(fd, buffer, CONF_MAX) == ERROR) {
        print("Error in: read\n");
        close(fd);
        exit(ERROR);
    }

    // Close configuration file descriptor.
    if (close(fd) == ERROR) {
        print("Error in: close\n");
        exit(ERROR);
    }


    // Extract information from the configuration file.
    char *targetDirectory = strtok(buffer, "\n");
    char *inputFile = strtok(NULL, "\n");
    char *correctFile = strtok(NULL, "\n");

    if (setupChecks(targetDirectory, inputFile, correctFile) == ERROR) {
        return ERROR;
    }

    // Run test -- find C files, compile each of them, run and test outputs.
    int status = runTest(targetDirectory, inputFile, correctFile);

    // If unexpected error will occur, runTest() will return -1, and the main will exit with code -1.
    if (status != SUCCESS) {
        exit(ERROR);
    }

    // Done.
    return SUCCESS;

}