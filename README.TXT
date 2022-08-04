DISK_FILE_SIMULATOR Authored by Laith Al-Mouhtaseb 211353297

==Description==
This program is simulating the disk management using index allocation, first we have a file that's presenting as our hard disk to work on it,
we have a three classes to help us:
1.FsFile: this class contains the low level data of every file we create on our disk, the block size, blocks that's file is using, file size
and the index of the index block.
2.FileDescriptor: this class is the connecting between the FsFile and the name of the file.
3.FsDisk:this class is our disk,contains all operators for disk.


==functions==

--Public
FsDisk():
constructor to open a file, and file it '\0'

listAll():
print the files at our disk and print the disk file to show what's there,
when printing the disk content if the char is invisible add it to him 32 and print different char, but inside the file its different value.

fsFormat():
if the disk is already formatted and want to format it again, first close every opened file then delete all the files and there's contains and delete the BitVector,
else if this the first time want to format the disk, first split the disk to blocks according to the block size that the user request,
then initiate the bitVector to zero with size of num of blocks inside the disk this vector to tell us which blocks are free inside the disk.
and fill the file to '\0', update the blockSize and is_formatted.

CreateFile(string):
get a name for the file and search if there an exist file in the same name in the disk,MainDir is a vector that contains every files inside the disk,
if there's no file at the same name so we create an object of FsFile then connect it with his name using FileDescriptor object,
at the end add the file to MainDir vector and add his name to OpenFileDescriptor vector and make it inUse (opened) and return the fd of it according to the index of it inside the openFileDescriptor.
return values: success = fd, error = -1.

OpenFile(string):
get the filename that the user want to open,search if the file is exist in the MairDir and make it inUse,
then add his name to open fileDescriptor and return the fd of it according to the index of it inside the openFileDescriptor.
return values: success = fd, error = -1.

CloseFile(int):
get the fd and check if its exist in OpenFileDescriptor,
then replace his name inside OpenFileDescriptor to "",
this for the next Open or Create file get the first index of "" element inside OpenfileDescriptor to be his fd.
return values: success = the file name, error "-1".

DelFile(string):
get the file name to Delete it, it must be closed before you can delete it, search for the file in the MainDir and check if its closed,
then call ClearAllDataOfFile function to free the contents of the file from the hard disk and free his blocks, at the end delete it form the vector.
return values: success = 1, error = -1.

WriteToFile(int,string,int):
get the fd of the file to write inside it, what to write on it, what is the length of the input.
get the file name of the file from OpenFileDescriptor vector, to search for the file into MainDir,
first get the index of index_block of the file, if this first time so get a 2 new blocks one for index_block and other for data, update the index of block_index.
inside the index_block save the index of data block, then start write inside the file,
while the file size is less than max file size continue, and every character that written subtract one from free location at this block.
if there's no free location at data block and the file size is less than the max file size, get a new data block.
we type the index of block inside the file as a character according to his ascii value.
to check if there's empty space inside the last used data block, get the value of ((size of block) - ((file size) mod (block size))) equal to free space size.
return value: success = how many character fit inside the file, error = -1.

ReadFromFile(int,string,len):
get the fd of the file to write inside it, where to store the read, what is the length to read from file.
get the file name of the file from OpenFileDescriptor vector, to search for the file into MainDir,
get the index of block index if there's, get data block block one by one to reach the length that requested.
return value: success how many characters read, error = -1.


--Private
getNewBlock: search for free block inside the hard disk, run at BitVector and search for 0 value.
return values: success = index of the block * blockSize, error = -1 "Disk is Full"

ClearAllDataOfFile(int): the index of file inside MainDir vector, get BlockIndex and enter block block of data and replace all elements to "\0",
at the end clear the index block, at every block its just cleared go to bitVector and update his value to 0 "free".

==Program Files==
main.cpp

==How to compile==
compile: g++ main.cpp -o main
run: ./main

==input==
number of the operator that user want to do.

==output==
update the DISK_FILE_SIMULATOR file