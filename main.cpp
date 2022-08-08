#include <iostream>
#include <map>
#include <assert.h>
#include <fcntl.h>
#include <list>
#include<stdlib.h>
#include <string.h>

using namespace std;

#define DISK_SIZE 256


class FsFile {
    int file_size;
    int block_in_use;
    int index_block;
    int block_size;

public:
    FsFile(int _block_size) {
        file_size = 0;
        block_in_use = 0;
        block_size = _block_size;
        index_block = -1;
    }

    void setIndexBlock(int &ind)
    {
        index_block = ind;
    }

    int getIndexBlock()
    {
        return index_block;
    }

    int getFileSize() {
        return file_size;
    }

    void setFileSize() {
        file_size++;
    }

    int getBlockSize() {
        return block_size;
    }


};
// ============================================================================
class FileDescriptor {
public:
    string file_name;
    FsFile* fs_file;
    bool inUse;

public:
    FileDescriptor(string FileName, FsFile* fsi) {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
    }

    string getFileName() {
        return file_name;
    }

};
// ============================================================================

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"

class fsDisk {
public:

    FILE *sim_disk_fd;
    bool is_formated;
    int BitVectorSize;
    int *BitVector;
    int blockSize;
    map<int,FileDescriptor*> OpenFileDescriptors;
    list<FileDescriptor*> MainDir;

public:
    fsDisk() {
        sim_disk_fd = fopen(DISK_SIM_FILE, "w+");
        assert(sim_disk_fd);

        for(int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0",1,1,sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        is_formated = false;
        blockSize = -1;
        list<FileDescriptor> MainDir;
        list<FileDescriptor> OpenFileDescriptors;
    }

    // ------------------------------------------------------------------------
    void listAll() {

        int i = 0;

        for(list<FileDescriptor*>::iterator it = MainDir.begin(); it != MainDir.end(); it++)
        {

            cout << "index: " << i << ": FileName: " << (*it)->getFileName()  <<  " , isInUse: " << (*it)->inUse <<endl;
            i++;
        }

        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++)
        {
            cout << "(";
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread(&bufy, 1, 1, sim_disk_fd);
            cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }

    // ------------------------------------------------------------------------
    void fsFormat( int blockSize = 4) {
        if(is_formated){
            delete[] BitVector;
            for (list<FileDescriptor*>::iterator it = MainDir.begin(); it != MainDir.end(); it++) {
                delete (*it)->fs_file;
                delete (*it);
            }

            MainDir.clear();
            OpenFileDescriptors.clear();
            for(int i = 0; i < DISK_SIZE; i++) {
                int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
                ret_val = fwrite("\0",1,1,sim_disk_fd);
                assert(ret_val == 1);
            }

        }
        this->blockSize=blockSize;
        BitVectorSize = DISK_SIZE/blockSize;
        BitVector = new int[BitVectorSize];
        for(int i = 0; i < BitVectorSize; i++) // set all blocks are empty
        {
            BitVector[i] = 0;
        }


        is_formated = true;
        cout << "FORMAT DISK: number of blocks: " << BitVectorSize << endl;
    }

    // ------------------------------------------------------------------------
    int find_first_empty_block()
    {
        for (int i = 0; i < BitVectorSize; i++)
        {
            if (BitVector[i] == 0)
            {
                return i;
            }
        }
        return -1; // no available blocks
    }

    // ------------------------------------------------------------------------
    int CreateFile(string fileName) {
        for (list<FileDescriptor*>::iterator it = MainDir.begin(); it != MainDir.end(); it++) {
            if ((*it)->getFileName() == fileName) {
                cout << "\n ----this file is already exists!----" << endl;
                return -1;
            }
        }

        if (!is_formated) // check if disk formatted
        {
            return -1;
        }

        FsFile *fsi = new FsFile(blockSize);  // fsi - new file object

        FileDescriptor* newfile = new FileDescriptor(fileName,fsi); // link the new file object with file name in new FileDescriptor object

        MainDir.push_back(newfile);
        int j = 0;
        while(1) {
            if(OpenFileDescriptors.find(j) == OpenFileDescriptors.end()){
                OpenFileDescriptors.insert({j,newfile});
                break;
            }
            j++;
        }
        return j;
    }

    // ------------------------------------------------------------------------
    int DelFile( string FileName ) {
        bool exist = false;
        if(!is_formated) // check if disk formatted
            return -1;

        for(list<FileDescriptor*>::iterator it = MainDir.begin(); it != MainDir.end(); it++) {// find and delete file from the main directory
            if ((*it)->getFileName() == FileName) {
                exist = true;
                if((*it)->inUse){
                    cout<<"the file is open, close it before delete!"<<endl;
                    return -1;
                }
            }

        }
        if(!exist){
            puts("this file is not exist!");
            return -1;
        }

        for(map<int,FileDescriptor*>::iterator it = OpenFileDescriptors.begin(); it != OpenFileDescriptors.end(); it++) // find and delete file from the open files list, if exists
        {
            if(it->second->getFileName() == FileName){
                puts("the file is open you have to close it before!");
                return -1;
            }
        }

            for(list<FileDescriptor*>::iterator it = MainDir.begin(); it != MainDir.end(); it++) // find and delete file from the main directory
        {
            string fname = (*it)->getFileName();
            if(fname == FileName)
            {
                int index_block = (*it)->fs_file->getIndexBlock();
                int block_size = blockSize; //it->fs_file->block_size;

                for (int i = 0; i < block_size ; i++) // i - counter for blocks
                {
                    int blockNumber = ReadfromIndexBlock(index_block, i, block_size);

                    if(blockNumber >= 0) {
                        BitVector[blockNumber] = 0;
                        int start = blockNumber * block_size;
                        int end = start + block_size;

                        fseek(sim_disk_fd, start, SEEK_SET);
                        for (int blckindx = 0; blckindx < block_size; blckindx++) {
                            fwrite("\0", 1, 1, sim_disk_fd);
                        }
                    }
                }

                // empty index block
                int start = index_block * block_size;
                int end = start + block_size;
                fseek(sim_disk_fd, start, SEEK_SET);
                for(int blckindx = 0; blckindx < block_size ; blckindx ++)
                {
                    fwrite("\0",1,1,sim_disk_fd);
                }
                if(index_block>=0){
                    BitVector[index_block] = 0;
                }
                delete (*it)->fs_file;
                delete (*it);
                MainDir.erase(it);
                break;
            }
        }
        int fd = 0;
        for(map<int,FileDescriptor*>::iterator it = OpenFileDescriptors.begin(); it != OpenFileDescriptors.end(); it++) // find and delete file from the open files list, if exists
        {
            string fname = it->second->getFileName();
            if(fname == FileName) {

                OpenFileDescriptors.erase(it);
                return it->first;
            }
            fd ++;
        }

        return fd;
    }

    // ------------------------------------------------------------------------
    int OpenFile(string fileName) {

        if(!is_formated){
            puts("the file is not formated yet!");
            return -1;
        }
        for (map<int, FileDescriptor*>::iterator it = OpenFileDescriptors.begin();it != OpenFileDescriptors.end(); it++) {
            string fname = it->second->getFileName();
            if (fname == fileName) {
                return -1; // return error if the file already opened
            }
        }
        int j = 0;
        for (list<FileDescriptor*>::iterator it = MainDir.begin();it != MainDir.end(); it++) // check if file exists in main directory
        {
            string fname = (*it)->getFileName();
            if (fname == fileName) {
                (*it)->inUse = true;
                while (1) {
                    if (OpenFileDescriptors.find(j) == OpenFileDescriptors.end()) {
                        OpenFileDescriptors.insert({j, (*it)});
                        break;
                    }
                    j++;
                }
                break;
            }
        }

        return j;
    }

    // ------------------------------------------------------------------------
    string CloseFile(int fd) {

        map<int,FileDescriptor*>::iterator it = OpenFileDescriptors.find(fd);
        string fileName = it->second->getFileName();

        for(list<FileDescriptor*>::iterator it = MainDir.begin(); it != MainDir.end(); it++) // check if file exists in main directory
        {
            string fname = (*it)->getFileName();
            if(fname == fileName){

                (*it)->inUse = false;
                break;
            }
        }

        for(map<int,FileDescriptor*>::iterator it = OpenFileDescriptors.begin(); it != OpenFileDescriptors.end(); it++) // find and delete file from the open files list, if exists
        {
            string fname = it->second->getFileName();
            if(fname == fileName){

                OpenFileDescriptors.erase(it);
                break;
            }
        }

        return fileName;
    }

    // ------------------------------------------------------------------------
    int WriteToFile(int fd, char *buf, int len) {
        // Validation checks:
        if (!is_formated)  // disk not formatted
            return -1;

        if (fd < 0) // file descriptor not valid
            return -1;
        map<int,FileDescriptor*>::iterator it = OpenFileDescriptors.find(fd);
        if(it == OpenFileDescriptors.end())
            return -1;
        int index_block = it->second->fs_file->getIndexBlock();
        if (!it->second->inUse)   // file not opened
            return -1;
        if(index_block==-1){
            index_block = find_first_empty_block();  //find find first empty block for index block
            if (index_block == -1) //no available blocks found
            {
                cout<<"ERROR"<<endl;
                return -1;
            }
            BitVector[index_block] = 1;
            it->second->fs_file->setIndexBlock(index_block);
        }
        int filesize = it->second->fs_file->getFileSize();
        int block_size = it->second->fs_file->getBlockSize();

        if ((filesize + len) > (block_size * block_size)) {
            return -1; // not enough space in file
        }

        // write
        int startBlock = GetLastBlock(index_block, block_size);
        int start = 0;
        if (startBlock >= 0) {
            start = startBlock * block_size;
        } else {
            return -1;
        }

        int prevblocknum = (int) start / block_size;
        int j = 0;  //j = index of text
        for (int i = start; i < DISK_SIZE && j < len; i++) // till first of - end of text or end of DISK_SIZE
        {
            int blockNumber = (int) i / block_size;
            if (blockNumber != prevblocknum) {
                if (BitVector[blockNumber] != 0) {
                    continue;
                }
            }
            WriteIndexBlock(index_block, blockNumber, block_size);
            fseek(sim_disk_fd, i, SEEK_SET);
            char bufy;
            fread(&bufy, 1, 1, sim_disk_fd);

            if (bufy != '\0') {

                continue;
            }
            BitVector[blockNumber] = 1;
            fseek(sim_disk_fd, i, SEEK_SET);
            fwrite(&buf[j], 1, 1, sim_disk_fd);

            it->second->fs_file->setFileSize();
            j++;

            prevblocknum = blockNumber;
        }

        return len;
    }

    // ------------------------------------------------------------------------
    void WriteIndexBlock(int index_block, int blockid, int block_size)
    {
        int start = index_block * block_size;
        int end = start + block_size;

        bool isDone = false;

        char blockidchr = '0' + blockid;

        for(int i = start; i < end && !isDone; i++) // i - index for blocks, condition - till max needed block AND end of text
        {
            fseek(sim_disk_fd, i, SEEK_SET);
            char bufy;
            fread(&bufy, 1, 1, sim_disk_fd);

            if(bufy == blockidchr) {
                isDone = true;
                continue;
            }
            if(bufy == '\0'){

                fseek(sim_disk_fd, i, SEEK_SET);
                fwrite(&blockidchr,1,1,sim_disk_fd);

                isDone = true;
                continue;

            }
        }
    }

    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len ) {

        // Validation checks:
        if(!is_formated)  // disk not formatted
            return -1;

        if(fd >= OpenFileDescriptors.size() || fd < 0) // file descriptor not valid
        {
            return -1;
        }
        map<int,FileDescriptor*>::iterator it = OpenFileDescriptors.find(fd);

        if(len > it->second->fs_file->getFileSize()){
            cout<<"SIZE ERROR"<<endl;
        }

        if(!it->second->inUse)   // file not opened
        {
            return -1;
        }

        int block_size = it->second->fs_file->getBlockSize();

        int index_block = it->second->fs_file->getIndexBlock();

        int j = 0; // j - index for text to read

        for (int i = 0; i < block_size && j < len ; i++) // i - counter for blocks
        {
            int blockNumber = ReadfromIndexBlock(index_block, i, block_size);
            int start = blockNumber * block_size;
            //int end = start + block_size;

            fseek(sim_disk_fd, start, SEEK_SET);
            for(int blckindx = 0; blckindx < block_size && j < len; blckindx ++)
            {
                fread(&buf[j],1,1,sim_disk_fd);
                j++;
            }
        }
        buf[len]='\0';
        return j;
    }

    // ------------------------------------------------------------------------
    int GetLastBlock(int index_block, int block_size)
{
    char buffer;
    int lastBlock = -1;
    int data_block;
    int used_bytes_in_block = 0;
    fseek(sim_disk_fd, index_block*block_size, SEEK_SET);
    for(int i = 0; i < block_size; i++){                                   ////////////////////// changed
        fread(&buffer, 1, 1,sim_disk_fd);
        if(atoi(&buffer) > lastBlock)
            lastBlock = atoi(&buffer); // Iterate through index block to get last data block number.
    }

    if(lastBlock == 0) {
        if(find_first_empty_block() >= 1){  // todo >=
            lastBlock = find_first_empty_block();
        }
        else return -1;
    }
    fseek(sim_disk_fd, lastBlock*block_size, SEEK_SET);
    for(int i = 0; i<block_size;i++){
//        data_block = fread(&buffer, 1, 1,sim_disk_fd);
        fread(&buffer, 1, 1,sim_disk_fd);
        if(buffer != '\0'){
            used_bytes_in_block++;
        }
    }
    if(used_bytes_in_block == block_size){
        int position = find_first_empty_block();
        if (position<0){
            fprintf(stderr,"Memory is full!\n");
            return -1;
        }
        BitVector[position] = 1;
        return position;
    }
    else return lastBlock;

}

    // ------------------------------------------------------------------------
    int ReadfromIndexBlock(int index_block, int indx, int block_size)
{
    int start = index_block * block_size + indx;
    fseek(sim_disk_fd, start, SEEK_SET);
    char bufy=0;
    fread(&bufy, 1, 1, sim_disk_fd);
    int blockid = bufy - '0';
    return blockid;
}

    // ------------------------------------------------------------------------
    int ReadfromBlock(int blockid, int block_size){
    int start = blockid * block_size ;

    for(int i = 0; i < block_size; i++)
    {
        fseek(sim_disk_fd, start, SEEK_SET);
        char bufy;
        fread(&bufy, 1, 1, sim_disk_fd);

       // cout << "ReadfromBlock bufy: " << bufy << " i: " << i << endl;

        if(bufy == '\0') {
            return i;
        }
    }
//    cout << "ReadfromBlock return -1 " << endl;
    return -1;
}

    ~fsDisk() {
        for (list<FileDescriptor*>::iterator it = MainDir.begin(); it != MainDir.end(); it++) {
            delete (*it)->fs_file;
            delete (*it);
        }
        MainDir.clear();
        fclose(sim_disk_fd);
        OpenFileDescriptors.clear();
        delete[] BitVector;
    }
};




