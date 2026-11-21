#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <deque>
#include <thread>//allowing the program to explicitly control its own execution state.
#include <chrono>//allowing the compiler to understand exact time measurements like milliseconds.
using namespace std;
// 2)
const string BLUE = "\033[1;34m";
const string GREEN = "\033[0;32m";
const string CYAN = "\033[1;36m";
const string RESET = "\033[0m";
void bootSequence()
{
// Just removing everything else from the screen
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    cout << "  _   _                   _____ ____  \n";
    cout << " | \\ | | __ _ _ __   ___ |  ___/ ___| \n";
    cout << " |  \\| |/ _` | '_ \\ / _ \\| |_  \\___ \\ \n";
    cout << " | |\\  | (_| | | | | (_) |  _|  ___) |\n";
    cout << " |_| \\_|\\__,_|_| |_|\\___/|_|   |____/ \n\n";

    cout << " Booting NanoFS Kernel v1.0...\n";
    cout << " [";
    // 3. The Animated Loading Bar
    for (int i = 0; i < 20; i++)
    {
        cout << "#" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    cout << "] 100%\n";
    cout << " Welcome to NanoFS. Type 'help' to begin.\n\n";
}

class FileNode
{
public:
    string name;
    bool isDirectory;
    string content = "";
    FileNode *parent; // for cd .. command
    unordered_map<string, FileNode *> children;

    FileNode(string nodeName, bool isDir, FileNode *parentNode = nullptr)
    {
        name = nodeName;
        isDirectory = isDir;
        parent = parentNode;
    }

    ~FileNode()
    {
        for (auto const &p : children)
        {
            delete p.second;
        }
    }
};

// implementation of MKDIR
class FileSystem
{
private:
    FileNode *root;
    FileNode *currentDir;
    deque<string> commandHistory;

public:
    FileSystem()
    {
        root = new FileNode("/", true);
        currentDir = root;
        root->children[".trash"] = new FileNode(".trash", true, root);
    }

    ~FileSystem()
    {
        delete root;
    }

    // creating new directory
    void mkdir(string dirName)
    {
        if (currentDir->children.contains(dirName))
        {
            cout << "mkdir :  cannot create directory " << dirName << " : File Already Exists\n";
            return;
        }
        FileNode *newDir = new FileNode(dirName, true, currentDir);
        currentDir->children[dirName] = newDir;
    }

    void touch(string fileName)
    {
        if (currentDir->children.contains(fileName))
        {
            cout << "touch: cannot create file " << fileName << " : File Already Exists\n";
            return;
        }
        FileNode *newFile = new FileNode(fileName, false, currentDir);
        currentDir->children[fileName] = newFile;
    }
    void ls()
    {
        for (auto const &p : currentDir->children)
        {
            if (p.second->isDirectory)
                cout << BLUE << p.first << RESET << "\n";
            else
                cout << GREEN << p.first << RESET << "\n";
        }
    }

    void lsWildcard(string pattern)
    {
        string ext = "";
        if (!pattern.empty() && pattern[0] == '*')
        {
            ext = pattern.substr(1);
        }
        else
        {
            ext = pattern;
        }
        for (auto const &p : currentDir->children)
        {
            if (p.first.size() >= ext.size() && p.first.substr(p.first.size() - ext.size()) == ext)
            {
                if (p.second->isDirectory)
                    cout << BLUE << p.first << RESET << "\n";
                else
                    cout << GREEN << p.first << RESET << "\n";
            }
        }
    }

    void cd(string dirName)
    {
        if (dirName == "..")
        {
            if (currentDir->parent == nullptr)
            {
                cout << "No parent directory\n";
                return;
            }
            currentDir = currentDir->parent;
            return;
        }
        if (!currentDir->children.contains(dirName) || !currentDir->children[dirName]->isDirectory)
        {
            cout << "Directory: " << dirName << " doesn't exist\n";
            return;
        }
        currentDir = currentDir->children[dirName];
    }

    void pathFinder(FileNode *dir, string &path)
    {
        if (dir == root)
        {
            path = "/";
            return;
        }
        pathFinder(dir->parent, path);
        if (path == "/")
        {
            path += dir->name;
        }
        else
        {
            path += ("/" + dir->name);
        }
    }

    void pwd()
    {
        string path = "";
        pathFinder(currentDir, path);
        cout << path << '\n';
    }

    void rm(string targetName)
    {
        if (targetName == ".trash" && currentDir == root)
        {
            cout << "Cannot remove system directory '.trash'\n";
            return;
        }
        if (!currentDir->children.contains(targetName))
        {
            cout << "File : " << targetName << " Doesn't exist\n";
            return;
        }
        FileNode *temp = currentDir->children[targetName];
        currentDir->children.erase(targetName);
        delete temp;
    }

    void trash(string targetName)
    {
        if (targetName == ".trash" && currentDir == root)
        {
            cout << "Cannot move system directory '.trash' to trash\n";
            return;
        }
        if (!currentDir->children.contains(targetName))
        {
            cout << "File : " << targetName << " Doesn't exist\n";
            return;
        }

        if (root->children[".trash"]->children.contains(targetName))
        {
            delete root->children[".trash"]->children[targetName];
        }

        FileNode *temp = currentDir->children[targetName];
        root->children[".trash"]->children[targetName] = temp;
        temp->parent = root->children[".trash"];
        currentDir->children.erase(targetName);
    }

    void restore(string targetName)
    {
        if (!root->children[".trash"]->children.contains(targetName))
        {
            cout << "File : " << targetName << " not found in trash\n";
            return;
        }

        if (currentDir->children.contains(targetName))
        {
            cout << "Cannot restore: '" << targetName << "' already exists in this directory.\n";
            return;
        }

        FileNode *temp = root->children[".trash"]->children[targetName];
        currentDir->children[targetName] = temp;
        temp->parent = currentDir;
        root->children[".trash"]->children.erase(targetName);
    }

    void write(string fileName, const string &text)
    {
        if (!currentDir->children.contains(fileName))
        {
            cout << "File : " << fileName << " Doesn't exist\n";
            return;
        }
        if (currentDir->children[fileName]->isDirectory)
        {
            cout << fileName << " is a Directory!\n";
            return;
        }
        currentDir->children[fileName]->content += (text + "\n");
    }

    void empty(string fileName)
    {
        if (!currentDir->children.contains(fileName))
        {
            cout << "File : " << fileName << " Doesn't exist\n";
            return;
        }
        if (currentDir->children[fileName]->isDirectory)
        {
            cout << fileName << " is a Directory!\n";
            return;
        }
        currentDir->children[fileName]->content = "";
    }

    void cat(string fileName)
    {
        if (!currentDir->children.contains(fileName))
        {
            cout << "File : " << fileName << " Doesn't exist\n";
            return;
        }
        if (currentDir->children[fileName]->isDirectory)
        {
            cout << fileName << " is a Directory!\n";
            return;
        }

        cout << currentDir->children[fileName]->content << '\n';
    }

    string escapeNewlines(const string &str)
    {
        string result = "";
        for (char c : str)
        {
            if (c == '\n')
            {
                result += "\\n";
            }
            else
            {
                result += c;
            }
        }
        return result;
    }

    void saveHelper(FileNode *node, string path, ofstream &outFile)
    {
        if (path != "/")
        {
            path += "/";
        }
        for (auto const &i : node->children)
        {
            string childPath = path + i.first;
            // write to file path + fileName;
            if (i.second->isDirectory)
            {
                outFile << "D " << childPath << '\n';
                saveHelper(i.second, childPath, outFile);
            }
            else
            {
                outFile << "F " << childPath << " " << escapeNewlines(i.second->content) << '\n';
            }
        }
    }

    void save()
    {
        ofstream outFile("data.txt");
        if (!outFile.is_open())
        {
            cout << "Could not open the file\n";
            return;
        }
        saveHelper(root, "", outFile);

        outFile.close();
        cout << "Saved Successfully\n";
    }

    string unescapeNewLines(const string &content)
    {
        string result = "";
        for (size_t i = 0; i < content.size(); i++)
        {
            if (content[i] == '\\' && i < content.size() - 1 && content[i + 1] == 'n')
            {
                result += '\n';
                i++;
            }
            else
            {
                result += content[i];
            }
        }
        return result;
    }

    vector<string> splitPaths(const string &path)
    {
        vector<string> parts;
        string s = "";
        for (size_t i = 1; i < path.size(); i++)
        {
            if (path[i] == '/')
            {
                parts.push_back(s);
                s = "";
            }
            else
                s += path[i];
        }
        if (!s.empty())
        {
            parts.push_back(s);
        }
        return parts;
    }

    void load()
    {
        ifstream inFile("data.txt");
        if (!inFile.is_open())
        {
            cout << "Could not open the file\n";
            return;
        }
        string type, path;
        while (inFile >> type >> path)
        {
            string content = "";
            if (type == "F")
            {
                getline(inFile, content);
                if (!content.empty() && content[0] == ' ')
                {
                    content = content.substr(1);
                }
                content = unescapeNewLines(content);
            }
            vector<string> paths = splitPaths(path);
            FileNode *temp = root;
            for (size_t i = 0; i < paths.size(); i++)
            {
                if (temp->children.contains(paths[i]))
                {
                    temp = temp->children[paths[i]];
                }
                else
                {
                    if (i != paths.size() - 1 || type == "D")
                    {
                        currentDir = temp;
                        mkdir(paths[i]);
                        temp = temp->children[paths[i]];
                    }
                    else
                    {
                        currentDir = temp;
                        touch(paths[i]);
                        temp->children[paths[i]]->content = content;
                    }
                }
            }
        }
        currentDir = root;
    }

    void treeHelper(FileNode *node, int depth)
    {
        for (auto const &i : node->children)
        {
            for (int j = 0; j < depth; j++)
            {
                cout << '\t';
            }
            if (i.second->isDirectory)
            {
                cout << "|-- " << BLUE << i.first << RESET << '\n';
                treeHelper(i.second, depth + 1);
            }
            else
            {
                cout << "|-- " << GREEN << i.first << RESET << '\n';
            }
        }
    }

    void tree()
    {
        cout << "/\n";
        treeHelper(root, 0);
    }

    void history()
    {
        for (size_t i = 0; i < commandHistory.size(); i++)
        {
            cout << i + 1 << "  " << commandHistory[i] << '\n';
        }
    }

    void clearScreen()
    {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }
    void grepHelper(FileNode *node, string currentPath, const string &keyword, bool &found)
    {
        for (auto const &p : node->children)
        {
            FileNode *child = p.second;
            string nextPath = (currentPath == "/") ? ("/" + child->name) : (currentPath + "/" + child->name);

            if (child->isDirectory)
            {
                if (child->name != ".trash")
                    grepHelper(child, nextPath, keyword, found);
            }
            else
            {
                if (child->content.find(keyword) != string::npos)
                {
                    cout << GREEN << nextPath << RESET << "\n";
                    found = true;
                }
            }
        }
    }
    void grep(const string &keyword)
    {
        bool found = false;
        grepHelper(root, "/", keyword, found);
        if (!found)
            cout << "No matches found for: " << keyword << "\n";
    }
    void statsHelper(FileNode *node, int &dirCount, int &fileCount, int &totalBytes)
    {
        for (auto const &p : node->children)
        {
            FileNode *child = p.second;
            if (child->name == ".trash")
                continue;
            if (child->isDirectory)
            {
                dirCount++;
                statsHelper(child, dirCount, fileCount, totalBytes);
            }
            else
            {
                fileCount++;
                totalBytes += child->content.length();
            }
        }
    }
    void nanofetch()
    {
        int dirCount = 0;
        int fileCount = 0;
        int totalBytes = 0;
        statsHelper(root, dirCount, fileCount, totalBytes);

        string currentPath = "";
        pathFinder(currentDir, currentPath);

        cout << "\n";
        cout << CYAN << "   _  __              ___  ___ " << RESET << "  " << GREEN << "user@nanofs" << RESET << "\n";
        cout << CYAN << "  / |/ /__ ____  ___ / _/ / __/" << RESET << "  " << "--------------------" << "\n";
        cout << CYAN << " /    / _ `/ _ \\/ _ \\ _/_ \\ \\  " << RESET << "  " << BLUE << "FS: " << RESET << "NanoFS v1.0\n";
        cout << CYAN << "/_/|_/\\_,_/_//_/\\___/_/ /___/  " << RESET << "  " << BLUE << "Path: " << RESET << currentPath << "\n";
        cout << CYAN << "                               " << RESET << "  " << BLUE << "Directories: " << RESET << dirCount << "\n";
        cout << CYAN << "                               " << RESET << "  " << BLUE << "Files: " << RESET << fileCount << "\n";
        cout << CYAN << "                               " << RESET << "  " << BLUE << "Used Space: " << RESET << totalBytes << " Bytes\n";
        cout << "\n";
    }
    void run()
    {
        load();
        string input;
        while (true)
        {
            string currentPath = "";
            pathFinder(currentDir, currentPath);
            cout << GREEN << "user@nanofs:" << BLUE << currentPath << RESET << "$ ";
            getline(cin, input);

            if (input.empty())
                continue;

            commandHistory.push_back(input);
            if (commandHistory.size() > 50)
            {
                commandHistory.pop_front();
            }

            if (input == "exit")
            {
                save();
                break;
            }
            if (input == "history")
            {
                history();
                continue;
            }
            if (input == "clear")
            {
                clearScreen();
                continue;
            }
            if (input == "ls")
            {
                ls();
                continue;
            }
            if (input == "pwd")
            {
                pwd();
                continue;
            }
            if (input == "tree")
            {
                tree();
                continue;
            }
            if (input == "nanofetch")
            {
                nanofetch();
                continue;
            }

            size_t spacePos = input.find(' ');
            if (spacePos != string::npos)
            {
                string cmd = input.substr(0, spacePos);
                string arg = input.substr(spacePos + 1);

                if (cmd == "mkdir")
                    mkdir(arg);
                else if (cmd == "touch")
                    touch(arg);
                else if (cmd == "cd")
                    cd(arg);
                else if (cmd == "rm")
                    rm(arg);
                else if (cmd == "cat")
                    cat(arg);
                else if (cmd == "empty")
                    empty(arg);
                else if (cmd == "write")
                {
                    size_t secondSpace = arg.find(' ');
                    if (secondSpace != string::npos)
                    {
                        write(arg.substr(0, secondSpace), arg.substr(secondSpace + 1));
                    }
                    else
                    {
                        cout << "Usage: write <filename> <text>\n";
                    }
                }
                else if (cmd == "trash")
                    trash(arg);
                else if (cmd == "restore")
                    restore(arg);
                else if (cmd == "ls")
                    lsWildcard(arg);
                else if (cmd == "grep")
                    grep(arg);
                else if (cmd == "shred")
                    shred(arg);
                else if (cmd == "find")
                    find(arg);
                else if (cmd == "du")
                    du(arg);
                else
                    cout << "Command not recognized.\n";
            }
            else if (!input.empty())
            {
                cout << "Command not recognized.\n";
            }
        }
    }
    // 1)
    void shred(string filename)
    {
        if (currentDir->children.find(filename) != currentDir->children.end())
        { // if file is there
            FileNode *target = currentDir->children[filename];
            if (!target->isDirectory)
            {

                target->content.assign(target->content.length(), '#'); // Replaces data with "#"

                cout << "Memory Shredded successfully for: " << filename << endl;

                currentDir->children.erase(filename); // removal from unordered map
                delete target;
            }
            else
            {
                cout << "Cannot shred a directory . Use rm instead.\n";
            }
        }
        else
        {
            cout << "Error: File not found!!" << endl;
        }
    }

    void findHelper(FileNode *currentNode, string targetName, string currentPath)
    {

        if (currentNode->name == targetName)
        {
            cout << "Found: " << currentPath << endl;
        }

        for (auto const &i : currentNode->children)
        {
            FileNode *childNode = i.second;
            string nextPath;
            if (currentPath == "/")
            {
                nextPath = currentPath + childNode->name;
            }
            else
            {
                nextPath = currentPath + "/" + childNode->name;
            }

            if (childNode->isDirectory)
            {
                findHelper(childNode, targetName, nextPath);
            }
            else if (childNode->name == targetName)
            {
                cout << "Found: " << nextPath << endl;
            }
        }
    }
    // find Command 3)
    void find(string targetName)
    {
        cout << "Searching Entire System For: " << targetName << "\n";

        findHelper(root, targetName, "/");
        cout << "Search Complete." << endl;
    }
    int duHelper(FileNode *currentNode)
    {
        int totalSize = 0;
        for (auto const &i : currentNode->children)
        {
            FileNode *childNode = i.second;
            if (childNode->isDirectory)
            {
                totalSize += duHelper(childNode);
            }
            else
            {
                totalSize += childNode->content.length();
            }
        }

        return totalSize;
    }
    void du(string folderName)
    {
        if (currentDir->children.find(folderName) != currentDir->children.end())
        {
            FileNode *targetFolder = currentDir->children[folderName];

            if (targetFolder->isDirectory)
            {
                int finalSize = duHelper(targetFolder);
                cout << finalSize << " Bytes " << folderName << endl;
            }
            else
            {
                cout << targetFolder->content.length() << " Bytes " << folderName << endl;
            }
        }
        else
        {
            cout << "Error: Directory not found !!" << endl;
        }
    }
};

int main()
{
    bootSequence();
    FileSystem fs;
    fs.run();
    return 0;
}