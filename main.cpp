#include <iostream>

#include <vector>

#include <cassert>

#include <cstring>

using namespace std;

#define DISK_SIZE 80


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
    FsFile *fs_file;
    bool inUse;

public:

    FileDescriptor(string FileName, FsFile *fsi) {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
    }

    ~FileDescriptor() {
        delete fs_file;
    }

    string getFileName() {
        return file_name;
    }

    bool isInUse() const {
        return inUse;
    }

    FsFile *getFsFile() const {
        return fs_file;
    }

    void setFileName(const string &fileName) {
        file_name = fileName;
    }

    void setInUse(bool inUse1) {
        FileDescriptor::inUse = inUse1;
    }
};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
#define MAX_FILE_SIZE sizeOfBlock * sizeOfBlock
// ============================================================================

class fsDisk {
    FILE *sim_disk_fd;
    bool is_formated;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.
    int BitVectorSize;
    int *BitVector;
    int sizeOfBlock;

    // (5) MainDir --
    // Structure that links the file name to its FsFile
    vector<FileDescriptor *> MainDir;

    // (6) OpenFileDescriptors --
    //  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    vector<int> OpenFileDescriptors;

    // ------------------------------------------------------------------------
public:

    // ------------------------------------------------------------------------
    fsDisk() {
        sim_disk_fd = fopen(DISK_SIM_FILE, "w+");
        assert(sim_disk_fd);

        for (int i = 0; i < DISK_SIZE; i++) {
            fseek(sim_disk_fd, i, SEEK_SET);
            size_t ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }

        fflush(sim_disk_fd);
        is_formated = false;
    }

    ~fsDisk() {
        for (auto file: MainDir) {
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
            if (!MainDir[i]->getFileName().empty())
                cout << "index: " << i << ": FileName: " << MainDir[i]->getFileName() << " , isInUse: " << MainDir[i]->isInUse() << endl;
            i++;
        }

        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++) {
            cout << "(";
            fseek(sim_disk_fd, i, SEEK_SET);
            size_t ret_val = fread(&bufy, 1, 1, sim_disk_fd);
            assert(ret_val);
            cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }

    // ------------------------------------------------------------------------
    void fsFormat(int blockSize = 4) {
        if (is_formated) {
            int i = 0;
            for (auto &file: MainDir) {
                if (file->isInUse())
                    CloseFile(i);
                DelFile((file->getFileName()));
                i++;
            }
            delete[] BitVector;
        }
        BitVectorSize = DISK_SIZE / blockSize;
        BitVector = new int[BitVectorSize];
        int k = 0;
        while (k < BitVectorSize) {
            BitVector[k] = 0;
            k++;
        }
        for (int i = 0; i < DISK_SIZE; i++) {
            fseek(sim_disk_fd, i, SEEK_SET);
            size_t ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }

        sizeOfBlock = blockSize;
        is_formated = true;
    }

    // ------------------------------------------------------------------------
    int CreateFile(string fileName) {
        if (!is_formated || sim_disk_fd == nullptr)
            return -1;
        for (auto &i: MainDir) {
            if (i->getFileName() == fileName) {
                cerr << "The file is exist" << endl;
                return -1;
            }
        }
        FsFile *fs;
        fs = new FsFile(sizeOfBlock);
        FileDescriptor *newFile;
        newFile = new FileDescriptor(fileName, fs);
        int i = 0;
        for (auto &file: MainDir) {
            if (file->getFileName().empty()) {
                delete MainDir[i];
                MainDir.erase(MainDir.begin() + i);
                MainDir.insert(MainDir.begin() + i, newFile);
                OpenFileDescriptors.push_back(i);
                file->setInUse(true);
                return i;
            }
            i++;
        }
        MainDir.push_back(newFile);
        OpenFileDescriptors.push_back((int) MainDir.size() - 1);
        return (int) MainDir.size() - 1;

    }

    // ------------------------------------------------------------------------
    int OpenFile(string fileName) {
        if (!is_formated)
            return -1;
        int i = 0;
        for (auto &file: MainDir) {
            if (file->getFileName() == fileName && !file->isInUse()) {
                file->setInUse(true);
                OpenFileDescriptors.push_back(i);
                return i;
            }
            i++;
        }
        return -1;
    }

    // ------------------------------------------------------------------------
    string CloseFile(int fd) {
        if (!is_formated || fd > (int) MainDir.size() - 1 || fd < 0 || !MainDir[fd]->isInUse())
            return "-1";

        int i = 0;
        for (auto open: OpenFileDescriptors) {
            if (open == fd) {
                OpenFileDescriptors.erase(i + OpenFileDescriptors.begin());
                MainDir[fd]->setInUse(false);
            }
            i++;
        }

        return MainDir[fd]->getFileName();
    }

    // ------------------------------------------------------------------------
    int WriteToFile(int fd, char *buf, int len) {
        if (fd > (int) MainDir.size() - 1 || fd < 0 || !is_formated)
            return -1;
        if (!MainDir[fd]->isInUse())
            return -1;
        size_t ret_val;
        char temp;
        int f_size = MainDir[fd]->getFsFile()->getfile_size();
        int dataInd = -1, existBlockFree = 0;
        int offsetOfIndexBlock = 0;
        int prev = MainDir[fd]->getFsFile()->getBlockInUse();
        int blockIndex = MainDir[fd]->getFsFile()->getIndexBlock();
        if (blockIndex == -1) {
            blockIndex = getNewBlock();
            if (blockIndex == -1) //the disk is full
                return -1;
            MainDir[fd]->getFsFile()->setIndexBlock(blockIndex);
            MainDir[fd]->getFsFile()->setBlockInUse(prev + 1);
            dataInd = getNewBlock();
            if (dataInd == -1) //the disk is full
                return -1;
            prev = MainDir[fd]->getFsFile()->getBlockInUse();
            //this for data block
            MainDir[fd]->getFsFile()->setBlockInUse(prev + 1);
            unsigned char c = dataInd / sizeOfBlock;

            fseek(sim_disk_fd, blockIndex + offsetOfIndexBlock, SEEK_SET);
            ret_val = fwrite(&c, 1, 1, sim_disk_fd);
            assert(ret_val == 1);
            existBlockFree = sizeOfBlock;
        } else {
            if (f_size % sizeOfBlock != 0) {
                offsetOfIndexBlock = MainDir[fd]->getFsFile()->getBlockInUse() - 2;
                fseek(sim_disk_fd, blockIndex + offsetOfIndexBlock, SEEK_SET);
                ret_val = fread(&temp, 1, 1, sim_disk_fd);
                assert(ret_val == 1);
                dataInd = (int) temp * sizeOfBlock;
                existBlockFree = sizeOfBlock - (f_size % sizeOfBlock);
            }
        }
        int done = 0;
        while (f_size < MAX_FILE_SIZE && done < len) {
            if (existBlockFree == 0) {
                offsetOfIndexBlock = MainDir[fd]->getFsFile()->getBlockInUse() - 1;
                dataInd = getNewBlock();
                if (dataInd == -1) //the disk is full
                    return -1;
                prev = MainDir[fd]->getFsFile()->getBlockInUse();
                //this for data block
                MainDir[fd]->getFsFile()->setBlockInUse(prev + 1);
                unsigned char c =dataInd / sizeOfBlock ;
                fseek(sim_disk_fd, blockIndex + offsetOfIndexBlock, SEEK_SET);
                ret_val = fwrite(&c, 1, 1, sim_disk_fd);
                assert(ret_val == 1);
                existBlockFree = sizeOfBlock;
            }

            for (int j = 0; j < existBlockFree && done < len; j++) {
                fseek(sim_disk_fd, dataInd + (f_size % sizeOfBlock), SEEK_SET);
                ret_val = fwrite(&buf[done], 1, 1, sim_disk_fd);
                assert(ret_val == 1);
                MainDir[fd]->getFsFile()->addByte();
                existBlockFree--;
                done++;
                f_size = MainDir[fd]->getFsFile()->getfile_size();
            }
        }
        return done;
    }

    // ------------------------------------------------------------------------
    int DelFile(string FileName) {
        if (!is_formated)
            return -1;
        int i = 0;
        for (; i < MainDir.size();) {
            if (MainDir[i]->getFileName() == FileName) {
                if (MainDir[i]->isInUse())
                    return -1;
                if (MainDir[i]->getFsFile()->getIndexBlock() != -1) {
                    clearAllDataOfFile(i);
                }
                MainDir[i]->setFileName("");
                return i;
            }

            i++;
        }

        return -1;

    }

    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len) {
        buf[0] = '\0';
        if (!is_formated || fd > (int) MainDir.size() - 1 || fd < 0 || !MainDir[fd]->isInUse()) {
            return -1;
        }
        int blockIndex = MainDir[fd]->getFsFile()->getIndexBlock();
        unsigned char c;
        int j;
        int dataInd;
        //read the index block and store into temp
        if (blockIndex == -1) {
            return -1;
        }
        int done = 0;
        for (int l = blockIndex; done < len && done < MainDir[fd]->getFsFile()->getfile_size(); ++l) {
            fseek(sim_disk_fd, l, SEEK_SET);
            size_t ret_val1 = fread(&c, 1, 1, sim_disk_fd);
            assert(ret_val1 == 1);
            dataInd = (int) c * sizeOfBlock;
            j = 0;
            while (j < sizeOfBlock && done < len && done < MainDir[fd]->getFsFile()->getfile_size()) {
                fseek(sim_disk_fd, dataInd + j, SEEK_SET);
                size_t ret_val = fread(&buf[done], 1, 1, sim_disk_fd);
                assert(ret_val == 1);
                done++;
                j++;
            }
        }

        buf[done] = '\0';
        return done;
    }

private:
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

    void clearAllDataOfFile(int fd) {
        unsigned char c;
        int ind = MainDir[fd]->getFsFile()->getIndexBlock();

        int done = 0, i = 0, dataInd;
        while (done < MainDir[fd]->getFsFile()->getfile_size()) {
            fseek(sim_disk_fd, i + ind, SEEK_SET);
            size_t ret_val = fread(&c, 1, 1, sim_disk_fd);
            assert(ret_val);
            fseek(sim_disk_fd, i + ind, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val);
            BitVector[(int) c] = 0;
            dataInd = (int) c * sizeOfBlock;
            i++;
            for (int j = 0; j < sizeOfBlock && done < MainDir[fd]->getFsFile()->getfile_size(); j++) {
                fseek(sim_disk_fd, j + dataInd, SEEK_SET);
                ret_val = fwrite("\0", 1, 1, sim_disk_fd);
                assert(ret_val);
                done++;
            }
        }
        if (ind/sizeOfBlock < BitVectorSize)
            BitVector[ind/sizeOfBlock] = 0;

    }

};

int main() {
    int blockSize;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;


//    fs->fsFormat(1);
//    char s[20];
//    for(int i=0;i<127;i++){
//        sprintf(s,"%d",i);
//        fs->CreateFile(s);
//        fs->WriteToFile(i,"1", 1);
//    }
    while (true) {
        cin >> cmd_;
        switch (cmd_) {
            case 0: // exit
                delete fs;
                exit(0);

            case 1: // list-file
                fs->listAll();
                break;

            case 2: // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;

            case 3: // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 4: // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5: // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 6: // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile(_fd, str_to_write, (int) strlen(str_to_write));
                break;

            case 7: // read-file
                cin >> _fd;
                cin >> size_to_read;
                fs->ReadFromFile(_fd, str_to_read, size_to_read);
                cout << "ReadFromFile: " << str_to_read << endl;
                break;

            case 8: // delete file
                cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }

}

