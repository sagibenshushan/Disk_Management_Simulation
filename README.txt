
Authored by Sagi Ben Shushan
209351147



Disk Management Simulation
Authored by sagi ben shushan
209351147

==Description for final_Exe ==
This program simulates the memory management in the unix system.
In the system we chose, we will use the index allocation method.
The system produces a user interface that allows characters to be inserted into the disc while providing the ability to delete, create, open, reset, write, read and terminate.



==functions for final_Exe==

listAll- This method prints all the disk files in order and arrange them according to blocks.

FsFormat- This method resets the previous format and produces a structure adapted to the new format received as input to the function

CreateFile-This method generates a new file, if the file already exists it returns -1.
If it does not exist, create it and update its usage field to 1

WriteToFile-This method is the core of the program.
In this process we will want to write to disk.
First of all we will check that there is space on the disk and that all the data received as input is normal.
Then we will try to write them to a file. If there is enough space, we will write them in full.
Otherwise, we will write as much as the system can

ReadFromFile-This method reads from the file.
If the file does not exist, the function returns -1

DeleteFile-This method deletes a file.
If the file does not exist, return -1.
If the file is open it has been defined that it cannot be deleted so it will also return -1.
If the file exists and is closed we will delete the file and initialize all fields related to it in all existing data structures.



==Program Files==
main.cpp

==How to compile EX5?==
compile: g++ main.cpp  -o main
run: ./main

===Input:==
user input

==Output:==
depends on the input number that the user has choosen.




