#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <deque>
#include <thread>//allowing the program to explicitly control its own execution state.
#include <chrono>//allowing the compiler to understand exact time measurements like milliseconds.
#include <sstream>
#include <stack>
using namespace std;
// 2)
const string BLUE = "\033[1;34m";
const string GREEN = "\033[0;32m";
const string CYAN = "\033[1;36m";
const string RESET = "\033[0m";
void bootSequeuce()
{
// Just removing everything else from the screen
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    cout << "  _   _                  Rl___  \n";
    cout << " | \\ | | __ _ _ __   ___|  ___| ___\n";
    cout << " |  \\| |/ _` | '_ \\ / _ \\ |_ / __|\n";
    cout << " | |\\  | (_| | | | | (_) |  _\\__ \\\n";
    cout << " |_| \\_|\\__,_|_| |_|\\___/|_| |___/\n\n";

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

struct HashEntry
{
    unsigned long long hashKey;
    string filePath;
    FileNode *nodeRef;
    HashEntry *next;

    HashEntry(unsigned long long key, string path, FileNode *node)
    {
        hashKey = key;
        filePath = path;
        nodeRef = node;
        next = nullptr;
    }
};

struct TrieNode
{
    TrieNode *children[128];
    bool isEndOfWord;

    TrieNode()
    {
        isEndOfWord = false;
        for (int i = 0; i < 128; i++)
        {
            children[i] = nullptr;
        }
    }

    ~TrieNode()
    {
        for (int i = 0; i < 128; i++)
        {
            if (children[i] != nullptr)
            {
                delete children[i];
            }
        }
    }
};

struct Transaction
{
    string opType;        
    string targetPath;    
    string oldPath;       
    string oldContent;    
    string newContent;    
    FileNode *nodeBackup; 

    Transaction(string op, string tPath, string oPath = "", string oCont = "", string nCont = "", FileNode *backup = nullptr)
    {
        opType = op;
        targetPath = tPath;
        oldPath = oPath;
        oldContent = oCont;
        newContent = nCont;
        nodeBackup = backup;
    }
};

// implementation of MKDIR
class FileSystem
{
private:
    FileNode *root;
    FileNode *currentDir;
    deque<string> commandHistory;
    stack<Transaction> undoStack;
    stack<Transaction> redoStack;
    bool isReplaying = false;

public:
    FileSystem()
    {
        root = new FileNode("/", true);
        currentDir = root;
        root->children[".trash"] = new FileNode(".trash", true, root);
    }

    void clearTransactionStack(stack<Transaction> &st)
    {
        while (!st.empty())
        {
            if (st.top().nodeBackup != nullptr)
            {
                delete st.top().nodeBackup;
            }
            st.pop();
        }
    }

    ~FileSystem()
    {
        clearTransactionStack(undoStack);
        clearTransactionStack(redoStack);
        delete root;
    }

    // creating new directory
    void mkdir(string dirName)
    {
        if (currentDir->children.contains(dirName))
        {
            cout << "mkdir :  cannot create directory " << dirName << " : File Already Exists";
            return;
        }
        FileNode *newDir = new FileNode(dirName, true, currentDir);
        currentDir->children[dirName] = newDir;
        string fullPath = "";
        pathFinder(newDir, fullPath);
        recordTransaction(Transaction("CREATE", fullPath, "", "", "", cloneNode(newDir, nullptr)));
    }

    void touch(string fileName)
    {
        if (currentDir->children.contains(fileName))
        {
            cout << "touch: cannot create file " << fileName << " : File Already Exists";
            return;
        }
        FileNode *newFile = new FileNode(fileName, false, currentDir);
        currentDir->children[fileName] = newFile;
        string fullPath = "";
        pathFinder(newFile, fullPath);
        recordTransaction(Transaction("CREATE", fullPath, "", "", "", cloneNode(newFile, nullptr)));
    }
    void ls()
    {
        for (auto p : currentDir->children)
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
        for (auto p : currentDir->children)
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
            cout << "Directory: " << dirName << " doesn't exists\n";
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
        if (!currentDir->children.contains(targetName))
        {
            cout << "File : " << targetName << " Doesn't exist\n";
            return;
        }
        FileNode *temp = currentDir->children[targetName];
        string fullPath = "";
        pathFinder(temp, fullPath);
        recordTransaction(Transaction("DELETE", fullPath, "", "", "", cloneNode(temp, nullptr)));
        currentDir->children.erase(targetName);
        delete temp;
    }

    void trash(string targetName)
    {
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
            cout << "File : " << fileName << " Doesn't exists\n";
            return;
        }
        if (currentDir->children[fileName]->isDirectory)
        {
            cout << fileName << " is a Directory!\n";
            return;
        }
        FileNode *target = currentDir->children[fileName];
        string oldCont = target->content;
        string fullPath = "";
        pathFinder(target, fullPath);

        target->content += (text + "\n");
        recordTransaction(Transaction("MODIFY", fullPath, "", oldCont, target->content));
    }

    void empty(string fileName)
    {
        if (!currentDir->children.contains(fileName))
        {
            cout << "File : " << fileName << " Doesn't exists\n";
            return;
        }
        if (currentDir->children[fileName]->isDirectory)
        {
            cout << fileName << " is a Directory!\n";
            return;
        }
        FileNode *target = currentDir->children[fileName];
        string oldCont = target->content;
        string fullPath = "";
        pathFinder(target, fullPath);

        target->content = "";
        recordTransaction(Transaction("MODIFY", fullPath, "", oldCont, ""));
    }

    void cat(string fileName)
    {
        if (!currentDir->children.contains(fileName))
        {
            cout << "File : " << fileName << " Doesn't exists\n";
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
        for (auto i : node->children)
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
        for (int i = 0; i < content.size(); i++)
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
        for (int i = 1; i < path.size(); i++)
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
        isReplaying = true;
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
            for (int i = 0; i < paths.size(); i++)
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
        isReplaying = false;
    }

    void treeHelper(FileNode *node, int depth)
    {
        for (auto i : node->children)
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
        for (int i = 0; i < commandHistory.size(); i++)
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

    FileNode *resolvePath(const string &path)
    {
        if (path.empty())
            return nullptr;
        if (path == "/")
            return root;

        FileNode *curr = (path[0] == '/') ? root : currentDir;
        int i = (path[0] == '/') ? 1 : 0;

        while (i < path.size())
        {
            string part = "";
            while (i < path.size() && path[i] != '/')
            {
                part += path[i];
                i++;
            }
            i++;

            if (part.empty() || part == ".")
                continue;
            if (part == "..")
            {
                if (curr->parent != nullptr)
                    curr = curr->parent;
            }
            else
            {
                if (!curr->isDirectory || !curr->children.contains(part))
                    return nullptr;
                curr = curr->children[part];
            }
        }
        return curr;
    }

    bool splitParentAndName(const string &path, FileNode *&parentNode, string &targetName)
    {
        if (path.empty() || path == "/")
            return false;

        string cleanPath = path;
        while (cleanPath.size() > 1 && cleanPath.back() == '/')
            cleanPath.pop_back();

        size_t lastSlash = cleanPath.find_last_of('/');
        if (lastSlash == string::npos)
        {
            parentNode = currentDir;
            targetName = cleanPath;
        }
        else if (lastSlash == 0)
        {
            parentNode = root;
            targetName = cleanPath.substr(1);
        }
        else
        {
            parentNode = resolvePath(cleanPath.substr(0, lastSlash));
            targetName = cleanPath.substr(lastSlash + 1);
        }

        return (parentNode != nullptr && parentNode->isDirectory && !targetName.empty());
    }

    FileNode *cloneNode(FileNode *src, FileNode *newParent)
    {
        FileNode *copy = new FileNode(src->name, src->isDirectory, newParent);
        copy->content = src->content;

        for (auto const &p : src->children)
        {
            copy->children[p.first] = cloneNode(p.second, copy);
        }
        return copy;
    }

    bool isAncestor(FileNode *potentialAncestor, FileNode *node)
    {
        FileNode *curr = node;
        while (curr != nullptr)
        {
            if (curr == potentialAncestor)
                return true;
            curr = curr->parent;
        }
        return false;
    }

    vector<string> splitArgs(const string &str)
    {
        vector<string> tokens;
        stringstream ss(str);
        string word;
        while (ss >> word)
        {
            tokens.push_back(word);
        }
        return tokens;
    }

    void recordTransaction(Transaction tx)
    {
        if (isReplaying)
            return;
        clearTransactionStack(redoStack);
        undoStack.push(tx);
    }

    void cpSingle(const string &srcPath, const string &destPath)
    {
        FileNode *srcNode = resolvePath(srcPath);
        if (srcNode == nullptr)
        {
            cout << "cp: cannot find '" << srcPath << "'\n";
            return;
        }
        if (srcNode == root || srcNode->name == ".trash")
        {
            cout << "cp: cannot copy system directory\n";
            return;
        }

        FileNode *destNode = resolvePath(destPath);
        if (destNode != nullptr)
        {
            if (!destNode->isDirectory)
            {
                cout << "cp: destination '" << destPath << "' already exists as a file\n";
                return;
            }
            if (isAncestor(srcNode, destNode))
            {
                cout << "cp: cannot copy a directory into itself\n";
                return;
            }
            if (destNode->children.contains(srcNode->name))
            {
                cout << "cp: '" << srcNode->name << "' already exists in destination\n";
                return;
            }
            FileNode *cloned = cloneNode(srcNode, destNode);
            destNode->children[srcNode->name] = cloned;

            string createdPath = "";
            pathFinder(cloned, createdPath);
            recordTransaction(Transaction("CREATE", createdPath, "", "", "", cloneNode(cloned, nullptr)));
            cout << GREEN << "Copied '" << srcPath << "' -> '" << createdPath << "'" << RESET << "\n";
        }
        else
        {
            FileNode *destParent = nullptr;
            string newName = "";
            if (!splitParentAndName(destPath, destParent, newName))
            {
                cout << "cp: invalid destination path '" << destPath << "'\n";
                return;
            }
            if (isAncestor(srcNode, destParent))
            {
                cout << "cp: cannot copy a directory into itself\n";
                return;
            }
            if (destParent->children.contains(newName))
            {
                cout << "cp: '" << newName << "' already exists\n";
                return;
            }
            FileNode *cloned = cloneNode(srcNode, destParent);
            cloned->name = newName;
            destParent->children[newName] = cloned;

            string createdPath = "";
            pathFinder(cloned, createdPath);
            recordTransaction(Transaction("CREATE", createdPath, "", "", "", cloneNode(cloned, nullptr)));
            cout << GREEN << "Copied '" << srcPath << "' -> '" << createdPath << "'" << RESET << "\n";
        }
    }

    void cp(const string &rawArgs)
    {
        vector<string> args = splitArgs(rawArgs);
        if (args.size() < 2)
        {
            cout << "Usage: cp <src1> [src2 ...] <destination>\n";
            return;
        }
        string dest = args.back();
        if (args.size() > 2)
        {
            FileNode *destNode = resolvePath(dest);
            if (destNode == nullptr || !destNode->isDirectory)
            {
                cout << "cp: target '" << dest << "' must be an existing directory for multiple files\n";
                return;
            }
        }
        for (int i = 0; i < args.size() - 1; i++)
        {
            cpSingle(args[i], dest);
        }
    }

    void mvSingle(const string &srcPath, const string &destPath)
    {
        FileNode *srcNode = resolvePath(srcPath);
        if (srcNode == nullptr)
        {
            cout << "mv: cannot find '" << srcPath << "'\n";
            return;
        }
        if (srcNode == root || srcNode->name == ".trash")
        {
            cout << "mv: cannot move system directory\n";
            return;
        }

        string oldFullPath = "";
        pathFinder(srcNode, oldFullPath);

        FileNode *destNode = resolvePath(destPath);
        if (destNode != nullptr)
        {
            if (!destNode->isDirectory)
            {
                cout << "mv: destination '" << destPath << "' already exists as a file\n";
                return;
            }
            if (isAncestor(srcNode, destNode))
            {
                cout << "mv: cannot move a directory inside itself (cycle detected)\n";
                return;
            }
            if (destNode->children.contains(srcNode->name))
            {
                cout << "mv: '" << srcNode->name << "' already exists in destination\n";
                return;
            }

            srcNode->parent->children.erase(srcNode->name);
            srcNode->parent = destNode;
            destNode->children[srcNode->name] = srcNode;

            string newFullPath = "";
            pathFinder(srcNode, newFullPath);
            recordTransaction(Transaction("MOVE", newFullPath, oldFullPath));
            cout << GREEN << "Moved '" << oldFullPath << "' -> '" << newFullPath << "'" << RESET << "\n";
        }
        else
        {
            FileNode *destParent = nullptr;
            string newName = "";
            if (!splitParentAndName(destPath, destParent, newName))
            {
                cout << "mv: invalid destination path '" << destPath << "'\n";
                return;
            }
            if (isAncestor(srcNode, destParent))
            {
                cout << "mv: cannot move a directory inside itself (cycle detected)\n";
                return;
            }
            if (destParent->children.contains(newName))
            {
                cout << "mv: '" << newName << "' already exists\n";
                return;
            }

            srcNode->parent->children.erase(srcNode->name);
            srcNode->name = newName;
            srcNode->parent = destParent;
            destParent->children[newName] = srcNode;

            string newFullPath = "";
            pathFinder(srcNode, newFullPath);
            recordTransaction(Transaction("MOVE", newFullPath, oldFullPath));
            cout << GREEN << "Moved/Renamed '" << oldFullPath << "' -> '" << newFullPath << "'" << RESET << "\n";
        }
    }

    void mv(const string &rawArgs)
    {
        vector<string> args = splitArgs(rawArgs);
        if (args.size() < 2)
        {
            cout << "Usage: mv <src1> [src2 ...] <destination>\n";
            return;
        }
        string dest = args.back();
        if (args.size() > 2)
        {
            FileNode *destNode = resolvePath(dest);
            if (destNode == nullptr || !destNode->isDirectory)
            {
                cout << "mv: target '" << dest << "' must be an existing directory for multiple files\n";
                return;
            }
        }
        for (int i = 0; i < args.size() - 1; i++)
        {
            mvSingle(args[i], dest);
        }
    }

    unsigned long long computeRollingHash(const string &data)
    {
        const unsigned long long P = 131;
        unsigned long long hashVal = 0;
        for (char c : data)
        {
            hashVal = hashVal * P + static_cast<unsigned char>(c);
        }
        return hashVal;
    }

    void collectFilesForHash(FileNode *node, string currentPath, HashEntry *buckets[], int bucketCount)
    {
        for (auto const &p : node->children)
        {
            FileNode *child = p.second;
            if (child->name == ".trash")
                continue;

            string nextPath = (currentPath == "/") ? ("/" + child->name) : (currentPath + "/" + child->name);
            if (child->isDirectory)
            {
                collectFilesForHash(child, nextPath, buckets, bucketCount);
            }
            else if (!child->content.empty())
            {
                unsigned long long h = computeRollingHash(child->content);
                int idx = h % bucketCount;
                HashEntry *entry = new HashEntry(h, nextPath, child);
                entry->next = buckets[idx];
                buckets[idx] = entry;
            }
        }
    }

    void dedup(bool cleanMode = false)
    {
        const int BUCKET_COUNT = 64;
        HashEntry *buckets[BUCKET_COUNT] = {nullptr};
        collectFilesForHash(root, "/", buckets, BUCKET_COUNT);

        int duplicateGroups = 0;
        int wastedBytes = 0;

        for (int i = 0; i < BUCKET_COUNT; i++)
        {
            HashEntry *curr = buckets[i];
            while (curr != nullptr)
            {
                if (curr->nodeRef != nullptr)
                {
                    vector<HashEntry *> matches;
                    HashEntry *runner = curr->next;
                    while (runner != nullptr)
                    {
                        if (runner->nodeRef != nullptr &&
                            runner->hashKey == curr->hashKey &&
                            runner->nodeRef->content == curr->nodeRef->content)
                        {
                            matches.push_back(runner);
                        }
                        runner = runner->next;
                    }

                    if (!matches.empty())
                    {
                        duplicateGroups++;
                        cout << CYAN << "Duplicate Group #" << duplicateGroups << " (" << curr->nodeRef->content.size() << " Bytes):" << RESET << "\n";
                        cout << "  [KEEP] " << GREEN << curr->filePath << RESET << "\n";

                        for (HashEntry *dup : matches)
                        {
                            wastedBytes += dup->nodeRef->content.size();
                            if (cleanMode)
                            {
                                cout << "  [DELETED] " << dup->filePath << "\n";
                                FileNode *target = dup->nodeRef;
                                recordTransaction(Transaction("DELETE", dup->filePath, "", "", "", cloneNode(target, nullptr)));
                                target->parent->children.erase(target->name);
                                delete target;
                            }
                            else
                            {
                                cout << "  [DUPLICATE] " << dup->filePath << "\n";
                            }
                            dup->nodeRef = nullptr; 
                        }
                    }
                }
                curr = curr->next;
            }
        }

        for (int i = 0; i < BUCKET_COUNT; i++)
        {
            HashEntry *curr = buckets[i];
            while (curr != nullptr)
            {
                HashEntry *temp = curr;
                curr = curr->next;
                delete temp;
            }
        }

        if (duplicateGroups == 0)
            cout << "No duplicate files found across the file system.\n";
        else if (!cleanMode)
            cout << "\nTotal Wasted Space: " << wastedBytes << " Bytes. Run 'dedup clean' to remove duplicates.\n";
        else
            cout << "\nCleanup Complete! Freed " << wastedBytes << " Bytes.\n";
    }

    void trieInsert(TrieNode *tRoot, const string &word)
    {
        TrieNode *curr = tRoot;
        for (char c : word)
        {
            unsigned char idx = static_cast<unsigned char>(c);
            if (idx >= 128)
                continue;
            if (curr->children[idx] == nullptr)
            {
                curr->children[idx] = new TrieNode();
            }
            curr = curr->children[idx];
        }
        curr->isEndOfWord = true;
    }

    void indexTreeIntoTrie(FileNode *node, string currentPath, TrieNode *tRoot)
    {
        for (auto const &p : node->children)
        {
            FileNode *child = p.second;
            if (child->name == ".trash")
                continue;

            trieInsert(tRoot, child->name);
            string nextPath = (currentPath == "/") ? ("/" + child->name) : (currentPath + "/" + child->name);
            trieInsert(tRoot, nextPath);

            if (child->isDirectory)
            {
                indexTreeIntoTrie(child, nextPath, tRoot);
            }
        }
    }

    void trieCollectWords(TrieNode *node, string currentWord, vector<string> &results)
    {
        if (node->isEndOfWord)
        {
            results.push_back(currentWord);
        }
        for (int i = 0; i < 128; i++)
        {
            if (node->children[i] != nullptr)
            {
                trieCollectWords(node->children[i], currentWord + static_cast<char>(i), results);
            }
        }
    }

    void suggest(const string &prefix)
    {
        TrieNode *tRoot = new TrieNode();

        vector<string> commands = {
            "mkdir", "touch", "ls", "cd", "pwd", "rm", "trash", "restore",
            "write", "empty", "cat", "save", "load", "tree", "history",
            "clear", "grep", "nanofetch", "shred", "find", "du",
            "cp", "mv", "dedup", "suggest", "undo", "redo", "exit"};

        for (const string &cmd : commands)
            trieInsert(tRoot, cmd);

        indexTreeIntoTrie(root, "/", tRoot);

        TrieNode *curr = tRoot;
        for (char c : prefix)
        {
            unsigned char idx = static_cast<unsigned char>(c);
            if (idx >= 128 || curr->children[idx] == nullptr)
            {
                cout << "No suggestions matching '" << prefix << "'\n";
                delete tRoot;
                return;
            }
            curr = curr->children[idx];
        }

        vector<string> matches;
        trieCollectWords(curr, prefix, matches);

        cout << CYAN << "Suggestions for '" << prefix << "':" << RESET << "\n";
        for (const string &m : matches)
        {
            cout << "  -> " << GREEN << m << RESET << "\n";
        }

        delete tRoot; 
    }

    void applyReverseTransaction(const Transaction &tx, stack<Transaction> &targetStack)
    {
        isReplaying = true;

        if (tx.opType == "CREATE")
        {
            FileNode *node = resolvePath(tx.targetPath);
            if (node != nullptr && node != root)
            {
                FileNode *backup = cloneNode(node, nullptr);
                node->parent->children.erase(node->name);
                if (currentDir == node || isAncestor(node, currentDir))
                    currentDir = root;
                delete node;
                targetStack.push(Transaction("DELETE", tx.targetPath, "", "", "", backup));
            }
        }
        else if (tx.opType == "DELETE")
        {
            FileNode *parentNode = nullptr;
            string name = "";
            if (splitParentAndName(tx.targetPath, parentNode, name) && tx.nodeBackup != nullptr)
            {
                FileNode *restored = cloneNode(tx.nodeBackup, parentNode);
                restored->name = name;
                parentNode->children[name] = restored;
                targetStack.push(Transaction("CREATE", tx.targetPath, "", "", "", cloneNode(restored, nullptr)));
            }
        }
        else if (tx.opType == "MODIFY")
        {
            FileNode *node = resolvePath(tx.targetPath);
            if (node != nullptr && !node->isDirectory)
            {
                string currentContent = node->content;
                node->content = tx.oldContent;
                targetStack.push(Transaction("MODIFY", tx.targetPath, "", currentContent, tx.oldContent));
            }
        }
        else if (tx.opType == "MOVE")
        {
            FileNode *node = resolvePath(tx.targetPath);
            FileNode *oldParent = nullptr;
            string oldName = "";
            if (node != nullptr && splitParentAndName(tx.oldPath, oldParent, oldName))
            {
                node->parent->children.erase(node->name);
                node->name = oldName;
                node->parent = oldParent;
                oldParent->children[oldName] = node;
                targetStack.push(Transaction("MOVE", tx.oldPath, tx.targetPath));
            }
        }

        isReplaying = false;
    }

    void undo()
    {
        if (undoStack.empty())
        {
            cout << "Nothing to undo.\n";
            return;
        }
        Transaction tx = undoStack.top();
        undoStack.pop();
        applyReverseTransaction(tx, redoStack);
        if (tx.nodeBackup != nullptr)
            delete tx.nodeBackup;
        cout << CYAN << "Undo successful (" << tx.opType << " on " << tx.targetPath << ")" << RESET << "\n";
    }

    void redo()
    {
        if (redoStack.empty())
        {
            cout << "Nothing to redo.\n";
            return;
        }

        Transaction tx = redoStack.top();
        redoStack.pop();
        applyReverseTransaction(tx, undoStack);
        if (tx.nodeBackup != nullptr)
            delete tx.nodeBackup;
        cout << CYAN << "Redo successful (" << tx.opType << " on " << tx.targetPath << ")" << RESET << "\n";
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
            if (input == "dedup")
            {
                dedup(false);
                continue;
            }
            if (input == "undo")
            {
                undo();
                continue;
            }
            if (input == "redo")
            {
                redo();
                continue;
            }

            int spacePos = input.find(' ');
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
                    int secondSpace = arg.find(' ');
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
                else if (cmd == "cp")
                    cp(arg);
                else if (cmd == "mv")
                    mv(arg);
                else if (cmd == "dedup" && arg == "clean")
                    dedup(true);
                else if (cmd == "suggest")
                    suggest(arg);
                else if (cmd == "shred")
                    shred(arg);
                else if (cmd == "find")
                    find(arg);
                else if (cmd == "du")
                    du(arg);
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

                delete target;
                currentDir->children.erase(filename); // removal from unordered map
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
            FileNode *childeNode = i.second;
            if (childeNode->isDirectory)
            {
                totalSize += duHelper(childeNode);
            }
            else
            {
                totalSize += childeNode->content.length();
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
    FileSystem fs;
    fs.run();
    return 0;
}
