# FileComparison

This repositry documents a file comperator I've implemented as part of Operation Systems course I took at Bar-Ilan University.

## Part 1 - ex31.c

In this part I've implemented a program that gets pathes to two files and returns:
- **1**, if the files are identical - both files contains exactly the same contant.
- **3**, is the files are similar - the difference between the files is about spaces and lower/upper-case letters.
- **2**, if the files are different - the files are not identical nor similar.

For example, all the following files are similar:
<b>File A:</b>
12ab23
<b>File B:</b>
12Ab23
<b>File C:</b>
12aB23
<b>File D:</b>
12AB23
<b>File E:</b>
12 aB 23
<b>File F:</b>
12
ab2
3


## Part 2 - ex32.c

In this part I've implemented a program that gets a path to a configuration file that contains 3 lines:
Line 1: Path to a folder that contains subfolders. At the first level (one inside), each folder represents
User, and should contain a C file.
Line 2: Path to file containing input.
Line 3: A path to a file that contains the correct output for the line 2 input file.
The configuration file will end in a line drop character.

Note: simulation files attached ex3_resources.zip file.

This program enters each subdirectory of the directory given in line 1 of the configuration file, look for a C file (in each folder), compile it (if found), run it, and then use ex31.c program to compare the output to the correct output as shown in the file located in the path given in line 3 of the configuration file.
The output of the program is a CSV file that gives grades for every sub-program output according to ex31.c test (map subdirectory name to a numberic grade).

**Grading System:**
1. NO_C_FILE	<b>0</b>
2. COMPILATION_ERROR	<b>10</b>
3. TIMEOUT	<b>20</b>
4. WRONG	<b>50</b>
5. SIMILAR	<b>75</b>
6. EXCELLENT	<b>100</b>

## IDE and tools

1. Visual Studio Code
2. Notpad++


