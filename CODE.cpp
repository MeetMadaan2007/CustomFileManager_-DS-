#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
using namespace std;

class FileNode{
public:
    string name;
    bool isDirectory;
    FileNode *parent; // for cd .. command
    unordered_map  <string, FileNode*> children;

    FileNode(string nodeName, bool isDir, FileNode *parentNode = nullptr){
        name = nodeName;
        isDirectory = isDir;
        parent = parentNode;
    }

    ~FileNode() {
        for (auto const& p : children) {
            delete p.second;
        }
    }
};

// implementation of MKDIR
class FileSystem{
private:
    FileNode *root;
    FileNode *currentDir;
public:
    FileSystem(){
        root = new FileNode("/", true);
        currentDir = root;
    }

    // creating new directory AI nhi h
    void mkdir(string dirName){
        if(currentDir->children.contains(dirName)){
            cout << "mkdir :  cannot create directory " << dirName << " : File Already Exists";
            return;
        }
        FileNode * newDir = new FileNode (dirName, true, currentDir);
        currentDir->children[dirName] = newDir;
    }


    void touch(string fileName){
        if(currentDir->children.contains(fileName)){
            cout << "touch: cannot create file " << fileName << " : File Already Exists";
            return;
        }
        FileNode * newFile = new FileNode(fileName, false, currentDir);
        currentDir->children[fileName]= newFile;
    }
    void ls(){
        for(auto p :currentDir->children){
            cout << p.first << "\n";
        }
    }
};

int main(){

}