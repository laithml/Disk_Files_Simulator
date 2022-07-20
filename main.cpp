#include <iostream>

#include <vector>

#include <cassert>

#include <cstring>

using namespace std;

#define DISK_SIZE 16

class FsFile {
    int file_size;
    int block_in_use;
    int index_block;
    int block_size;

public:
    FsFile(int _block_size) {
        block_size = _block_size;
        file_size = 0;
        block_in_use = 0;
        index_block = -1;
    }

    int getfile_size() const {
        return file_size;
    }

    int getBlockInUse() const {
        return block_in_use;
    }

    void setBlockInUse(int blockInUse) {
        block_in_use = blockInUse;
    }

    int getIndexBlock() const {
        return index_block;
    }

    void setIndexBlock(int indexBlock) {
        index_block = indexBlock;
    }

    void addByte() {
        file_size++;
    }

};

// ============================================================================

class FileDescriptor {
    string file_name;
    FsFile * fs_file;
    bool inUse;

public:

    FileDescriptor(string FileName, FsFile * fsi) {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
    }
    ~FileDescriptor(){
        delete fs_file;
    }

    string getFileName() {
        return file_name;
    }

    bool isInUse() const {
        return inUse;
    }

    FsFile * getFsFile() const {
        return fs_file;
    }

    void setFileName(const string & fileName) {
        file_name = fileName;
    }

    void setFsFile(FsFile * fsFile) {
        fs_file = fsFile;
    }

    void setInUse(bool inUse) {
        FileDescriptor::inUse = inUse;
    }
};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
#define MAX_FILE_SIZE sizeOfBlock * sizeOfBlock
// ============================================================================

class fsDisk {
    FILE * sim_disk_fd;
    bool is_formated;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.
    int BitVectorSize;
    int * BitVector;
    int sizeOfBlock;

    // (5) MainDir --
    // Structure that links the file name to its FsFile
    vector < FileDescriptor * > MainDir;

    // (6) OpenFileDescriptors --
    //  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    vector < int > OpenFileDescriptors;

    // ------------------------------------------------------------------------
public:

    // ------------------------------------------------------------------------
    fsDisk() {
        sim_disk_fd = fopen(DISK_SIM_FILE, "w+");
        assert(sim_disk_fd);

        for (int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }

        fflush(sim_disk_fd);
        is_formated = false;
    }

    ~fsDisk() {
        for (auto file:MainDir) {
            delete file;
        }
        MainDir.clear();
        OpenFileDescriptors.clear();
        delete[] BitVector;
        fclose(sim_disk_fd);
    }
    void listAll() {
        int i = 0;

        for (; i < MainDir.size();) {
            if (!MainDir[i] -> getFileName().empty() && MainDir[i] -> getFsFile() != nullptr)
                cout << "index: " << i << ": FileName: " << MainDir[i] -> getFileName() << " , isInUse: " << MainDir[i] -> isInUse() << endl;
            i++;
        }

        unsigned char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++) {
            cout << "(";
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread( & bufy, 1, 1, sim_disk_fd);
            cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }

    // ------------------------------------------------------------------------
    void fsFormat(int blockSize = 4) {
        if(is_formated){
            for (auto &file:MainDir) {
                if(!file->getFileName().empty())
                    DelFile((file->getFileName()));
            }
        }
        BitVectorSize = DISK_SIZE / blockSize;
        BitVector = new int[BitVectorSize];
        int k = 0;
        while (k < BitVectorSize) {
            BitVector[k] = 0;
            k++;
        }
        for (int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }

        sizeOfBlock = blockSize;
        is_formated = true;
    }

    // ------------------------------------------------------------------------
    int CreateFile(string fileName) {
        if (!is_formated || sim_disk_fd == nullptr)
            return -1;
        for (auto & i: MainDir) {
            if (i -> getFileName() == fileName) {
                cerr << "The file is exist" << endl;
                return -1;
            }
        }
        FsFile * fs = new FsFile(sizeOfBlock);

        int i = 0;
        for (auto & file: MainDir) {
            if (file -> getFileName().empty() == 1 && file -> getFsFile() == nullptr) {
                file -> setFileName(fileName);
                file -> setFsFile(fs);
                OpenFileDescriptors.push_back(i);
                file -> setInUse(true);
                return i;
            }
            i++;
        }

        FileDescriptor * newFile = new FileDescriptor(fileName, fs);
        MainDir.push_back(newFile);
        OpenFileDescriptors.push_back(MainDir.size() - 1);
        return MainDir.size() - 1;

    }

    // ------------------------------------------------------------------------
    int OpenFile(string fileName) {
        if (!is_formated)
            return -1;
        int i = 0;
        for (auto & file: MainDir) {
            if (file -> getFileName() == fileName && !file->isInUse()) {
                file -> setInUse(true);
                OpenFileDescriptors.push_back(i);
                return i;
            }
            i++;
        }
        return -1;
    }

    // ------------------------------------------------------------------------
    string CloseFile(int fd) {
        if (!is_formated || fd > (int)MainDir.size() - 1 || fd < 0 || !MainDir[fd] -> isInUse())
            return "-1";

        int i = 0;
        for (auto open: OpenFileDescriptors) {
            if (open == fd) {
                OpenFileDescriptors.erase(i + OpenFileDescriptors.begin());
                MainDir[fd] -> setInUse(false);
            }
            i++;
        }

        return "Success";
    }

    // ------------------------------------------------------------------------
    int WriteToFile(int fd, char * buf, int len) {
        if (fd > (int) MainDir.size()-1|| fd < 0 || !is_formated)
            return -1;
        if (!MainDir[fd] -> isInUse())
            return -1;
        int blockIndex = MainDir[fd] -> getFsFile() -> getIndexBlock();
        unsigned char temp[sizeOfBlock];
        int j = 0;
        int dataInd = -1, existBlockFree = 0;
        int i = sizeOfBlock - 1;
        //read the index block and store into temp
        if (blockIndex != -1) {
            for (int l = blockIndex; j < sizeOfBlock; ++l, j++) {
                int ret_val = fseek(sim_disk_fd, l, SEEK_SET);
                ret_val = fread( & temp[j], 1, 1, sim_disk_fd);
                assert(ret_val == 1);
            }

            while (i >= 0) {
                if (temp[i] != '\0') {
                    if ((existBlockFree = howManyByteIsClear((int) temp[i]*sizeOfBlock)) != 0) {
                        dataInd = (int) temp[i] * sizeOfBlock;
                        break;
                    }

                }
                i--;
            }
        }
        //data block
        int done = 0;
        while (done < len && MainDir[fd] -> getFsFile() -> getfile_size() < MAX_FILE_SIZE) {
            if (existBlockFree == 0) {
                int prev = MainDir[fd] -> getFsFile() -> getBlockInUse();
                if (blockIndex == -1) {
                    blockIndex = getNewBlock();
                    MainDir[fd] -> getFsFile() -> setIndexBlock(blockIndex);
                    MainDir[fd] -> getFsFile() -> setBlockInUse(prev + 1);
                }
                dataInd = getNewBlock();
                if (dataInd == -1) //the disk is full
                    return -1;
                prev = MainDir[fd] -> getFsFile() -> getBlockInUse();
                //this for data block
                MainDir[fd] -> getFsFile() -> setBlockInUse(prev + 1);
                unsigned char c = (unsigned char) dataInd / sizeOfBlock;
                int dataIndexInsideBlockIndex = blockIndex + (sizeOfBlock - howManyByteIsClear(blockIndex));
                int ret_val = fseek(sim_disk_fd, dataIndexInsideBlockIndex, SEEK_SET);
                ret_val = fwrite( & c, 1, 1, sim_disk_fd);
                assert(ret_val == 1);
                existBlockFree = sizeOfBlock;

            }

            int ret_val = fseek(sim_disk_fd, dataInd + sizeOfBlock - existBlockFree, SEEK_SET);
            ret_val = fwrite( & buf[done], 1, 1, sim_disk_fd);
            assert(ret_val == 1);
            MainDir[fd] -> getFsFile() -> addByte();
            existBlockFree--;
            done++;
        }
        if (done < len)
            cerr << "reached to the max size of file" << endl;
        cout << "file name " << MainDir[fd] -> getFileName() << " file Size " << MainDir[fd] -> getFsFile() -> getfile_size() << " block in use " << MainDir[fd] -> getFsFile() -> getBlockInUse() <<
             " index block " << MainDir[fd] -> getFsFile() -> getIndexBlock() << endl;

        return 1;
    }

    // ------------------------------------------------------------------------
    int DelFile(string FileName) {
        if (!is_formated)
            return -1;
        int i = 0;
        for (auto & file: MainDir) {
            if (file -> getFileName() == FileName) {
                if (file -> isInUse())
                    CloseFile(i);
                clearAllDataOfFile(file -> getFsFile() -> getIndexBlock());
                file -> setFileName("");
                delete file -> getFsFile();
                file -> setFsFile(nullptr);
                return i;
            }

            i++;
        }

        return -1;

    }

    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char * buf, int len) {
        if (!is_formated || fd > (int) MainDir.size()-1|| fd < 0 || !MainDir[fd] -> isInUse())
            return -1;
        int blockIndex = MainDir[fd] -> getFsFile() -> getIndexBlock();
        unsigned char temp[sizeOfBlock];
        int j = 0;
        int dataInd;
        //read the index block and store into temp
        if (blockIndex == -1) {
            return -1;
        }
        for (int l = blockIndex; j < sizeOfBlock; ++l, j++) {
            int ret_val = fseek(sim_disk_fd, l, SEEK_SET);
            ret_val = fread( & temp[j], 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }

        //data block
        int done = 0, i = 0;
        while (temp[i] != '\0' && done < MainDir[fd]->getFsFile()->getfile_size()) {
            j = 0;
            dataInd = (int) temp[i] * sizeOfBlock;
            while (j < sizeOfBlock&& done<len) {
                int ret_val = fseek(sim_disk_fd, dataInd + j, SEEK_SET);
                ret_val = fread(   (unsigned char*) &  buf[done], 1, 1, sim_disk_fd);
                assert(ret_val == 1);
                MainDir[fd] -> getFsFile() -> addByte();
                done++;
                j++;
            }
            i++;
        }

        buf[done]='\0';
        return 1;
    }
private:
    int howManyByteIsClear(int blockInd) {
        int ind = blockInd, counter = 0;
        for (int i = 0; i < sizeOfBlock; i++) {
            char c;
            int ret_val = fseek(sim_disk_fd, i + ind, SEEK_SET);
            ret_val = fread( & c, 1, 1, sim_disk_fd);
            assert(ret_val == 1);
            if (c == '\0')
                counter++;
        }
        return counter;
    }

    int getNewBlock() {
        int i = 0;
        for (; i < BitVectorSize; ++i) {
            if (BitVector[i] == 0) {
                BitVector[i] = 1;
                return i * sizeOfBlock;
            }
        }

        return -1;

    }

    void clearAllDataOfFile(int ind) {
        unsigned char c;
        for (int i = 0; i < sizeOfBlock; i++) {
            int ret_val = fseek(sim_disk_fd, i + ind, SEEK_SET);
            ret_val = fread( & c, 1, 1, sim_disk_fd);
            assert(ret_val);
            ret_val = fseek(sim_disk_fd, i + ind, SEEK_SET);
            ret_val = fwrite( "\0", 1, 1, sim_disk_fd);
            assert(ret_val);
            if (c != '\0') {
                int dataInd = (int) c * sizeOfBlock;
                BitVector[(int) c]=0;
                for (int j = 0; j < sizeOfBlock; ++j) {
                    int ret_val1 = fseek(sim_disk_fd, j + dataInd, SEEK_SET);
                    ret_val1 = fwrite("\0", 1, 1, sim_disk_fd);
                    assert(ret_val1);
                }
            }
        }
        BitVector[ind]=0;

    }

};

int main() {
    int blockSize;
    int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;

    fsDisk * fs = new fsDisk();
    int cmd_;

    while (true) {
        cin >> cmd_;
        switch (cmd_) {
            case 0: // exit
                delete fs;
                exit(0);

            case 1: // list-file
                fs -> listAll();
                break;

            case 2: // format
                cin >> blockSize;
                fs -> fsFormat(blockSize);
                break;

            case 3: // creat-file
                cin >> fileName;
                _fd = fs -> CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 4: // open-file
                cin >> fileName;
                _fd = fs -> OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5: // close-file
                cin >> _fd;
                fileName = fs -> CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 6: // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs -> WriteToFile(_fd, str_to_write, strlen(str_to_write));
                break;

            case 7: // read-file
                cin >> _fd;
                cin >> size_to_read;
                fs -> ReadFromFile(_fd, str_to_read, size_to_read);
                cout << "ReadFromFile: " << str_to_read << endl;
                break;

            case 8: // delete file
                cin >> fileName;
                _fd = fs -> DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }

}