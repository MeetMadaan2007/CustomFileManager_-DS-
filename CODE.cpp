#include <iostream>
#include <vector>
#include <string>
using namespace std;

class FileNode
{
public:
    string name;
    bool isDirectory;
    FileNode *parent; // for cd .. command
    vector<FileNode *> children;

    FileNode(string nodeName, bool isDir, FileNode *parentNode = NULL)
    {
        name = nodeName;
        isDirectory = isDir;
        parent = parentNode;
    }

    ~FileNode()
    {
        for (FileNode *child : children)
        {
            delete child;
        }
    }
};

// implementation of MKDIR
class FileSystem
{
private:
    FileNode *root;
    FileNode *currentDir;
    FileSystem()
    {
        root = new FileNode("/", true);
    }

    // creating new directory AI nhi h
    void mkdir(string dirName)
    {
        for (FileNode *child : currentDir->children)
        {
            if (child->name == dirName && child->isDirectory)
            {
                cout << "mkdir : cannot create directory '" << dirName << "' : FILE EXISTS" << endl;
                return;
            }
        }
        FileNode *newDir = new FileNode(dirName, true, currentDir);
        currentDir->children.push_back(newDir);
    }
    void touch(string fileName)
    {
    }
    void ls()
    {
    }
};