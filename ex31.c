// Shlomi Ben-Shushan

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

// Define possible results according to exercise instructions.
#define IDENTICAL 1
#define SIMILAR   3
#define DIFFERENT 2

/**********************************************************************************
* Function:     selectiveReadByte
* Input:        int fd - File Descriptor.
* Output:       char - one byte.
* Operation:    This function uses fd to read the first byte which isn't a space or
*               a line-break and return it. If there is no such byte untill th end
*               of the file, it will return -1, which is not a valic ASCII char.
***********************************************************************************/
char selectiveReadByte(int fd) {
    char ch = 0;
    if (!read(fd, &ch, 1))
        return -1;
    if (ch != ' ' && ch != '\n')
        return ch;
    return selectiveReadByte(fd);
}

/**********************************************************************************
* Function:     areSimilar
* Input:        ch1, ch2 -- two characters.
* Output:       1 if ch1 is similar to ch2, 0 otherwise.
* Operation:    This function checks for similarity between two given characters.
*               It suppose the given characrers are not spaces nor line-breaks, so 
*               according to the exercise's instructions, similarity is defined by
*               if one is equal to the other, or one is a capital of the other.        
***********************************************************************************/
char areSimilar(char ch1, char ch2) {
    
    // if the characters are the same, or one is the capital of the other, return 1 for true.
    if (ch1 == ch2)
        return 1;
    if ('A' <= ch1 && ch1 <= 'Z' && ch2 == ch1 + 32)
        return 1;
    if ('a' <= ch1 && ch1 <= 'z' && ch2 == ch1 - 32)
        return 1;

    // Else, return 0 for false, means there is a significant difference between the characters.
    return 0;

}

/**********************************************************************************
* Function:     main
* Input:        argc, argv -- standard input.
* Output:       int -- 1 for identical, 2 for different, and 3 for similar.
* Operation:    Entry point of the program.
***********************************************************************************/
int main(int argc, char **argv) {

    // Not enough arguments.
    if (argc < 3) {
        exit(-1);
    }

    // Try to open source file.
    int srcFD = open(argv[1], O_RDONLY);
    if (srcFD == -1) {
        printf("Error in: open");
        exit(-1);
    }

    // Try to open destination file.
    int dstFD = open(argv[2], O_RDONLY);
    if (dstFD == -1) {
        printf("Error in: open");
        close(srcFD);
        exit(-1);
    }

    // Initialize variables for read() function.
    char srcChar = 0;
    char dstChar = 0;
    int srcRecv = 1;
    int dstRecv = 1;

    // Check for identity -- read() stops at the first different pair of chars (if exists).
    srcRecv = read(srcFD, &srcChar, srcRecv);
    dstRecv = read(dstFD, &dstChar, dstRecv);   
    while ((srcRecv > 0) && (dstRecv > 0) && (srcChar == dstChar)) {
        srcRecv = read(srcFD, &srcChar, srcRecv);
        dstRecv = read(dstFD, &dstChar, dstRecv);   
    }

    // If the files' contents are identical, we can close resources and return 1.
    if (srcChar == dstChar) {
        close(srcFD);
        close(dstFD);
        return IDENTICAL;
    }

    // If contents aren't the same, change strategy to find differnce.

    // Handle edge-case: one file is completely read and the difference starts in the second file after that.
    if (srcChar == ' ' || srcChar == '\n') {
        srcChar = selectiveReadByte(srcFD);
        if (srcChar == -1) {
            dstChar = selectiveReadByte(dstFD);
            if (dstChar == -1) {
                return SIMILAR;
            }
        }
    }

    // Same edge-case handling, but for the complement case where the other file is read first.
    if (dstChar == ' ' || dstChar == '\n') { 
        dstChar = selectiveReadByte(dstFD);
        if (dstChar == -1) {
            srcChar = selectiveReadByte(srcFD);
            if (srcChar == -1) {
                return SIMILAR;
            }
        }
    }

    // Difference checking loop.
    do {

        // If srcChar and dstChar are different, stop, release resources and return 2.
        if (!areSimilar(srcChar, dstChar)) {
            close(srcFD);
            close(dstFD);
            return DIFFERENT;
        }

        // Else, read another pair of non-space non-line-break characters, and check them as well. 
        srcChar = selectiveReadByte(srcFD);
        dstChar = selectiveReadByte(dstFD);

    } while (srcChar != -1 && dstChar != -1);

    // Close File Descriptors.
    close(srcFD);
    close(dstFD);

    // Make a final decision, if one file completely read while the other contains illegal characters,
    // return 2 for different. Else, return 3 for similar.
    if ((srcChar == -1 && dstChar != -1) || (dstChar == -1 && srcChar != -1)) { 
        return DIFFERENT;
    }
    return SIMILAR;

}