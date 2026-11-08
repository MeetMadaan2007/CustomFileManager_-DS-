#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
using namespace std;

class FileNode{
public:
    string name;
    bool isDirectory;
    string content = "";
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

    void cd(string dirName){
        if(dirName == ".."){
            if(currentDir->parent == nullptr){
                cout << "No parent directory\n";
                return;
            }
            currentDir = currentDir->parent;
            return;
        }
        if(!currentDir->children.contains(dirName) || !currentDir->children[dirName]->isDirectory){
            cout << "Directory: " << dirName << " doesn't exists\n";
            return;
        }
        currentDir = currentDir->children[dirName];
    }

    void pathFinder(FileNode * dir, string & path){
        if(dir == root){
            path = "/";
            return;
        }
        pathFinder(dir->parent, path);
        if(path == "/"){
            path += dir->name;
        }
        else{
            path += ("/"+ dir->name);
        }
    }

    void pwd(){
        string path = "";
        pathFinder(currentDir, path);
        cout << path << '\n';
    }

    void rm(string targetName){
        if(!currentDir->children.contains(targetName)){
            cout << "File : " << targetName << " Doesn't exist\n";
            return;
        }
        FileNode *temp = currentDir->children[targetName];
        currentDir->children.erase(targetName);
        delete temp;
    }

    void write(string fileName, const string & text){
        if(!currentDir->children.contains(fileName) ){
            cout << "File : " << fileName << " Doesn't exists\n";
            return;
        }
        if(currentDir->children[fileName]->isDirectory){
            cout << fileName << " is a Directory!\n";
            return;
        }
        currentDir->children[fileName]->content += (text + "\n");
    }

    void empty(string fileName){
        if(!currentDir->children.contains(fileName) ){
            cout << "File : " << fileName << " Doesn't exists\n";
            return;
        }
        if(currentDir->children[fileName]->isDirectory){
            cout << fileName << " is a Directory!\n";
            return;
        }
        currentDir->children[fileName]->content = "";
    }

    void cat(string fileName){
        if(!currentDir->children.contains(fileName) ){
            cout << "File : " << fileName << " Doesn't exists\n";
            return;
        }
        if(currentDir->children[fileName]->isDirectory){
            cout << fileName << " is a Directory!\n";
            return;
        }

        cout << currentDir->children[fileName]->content <<'\n';
    }
};

int main(){

}