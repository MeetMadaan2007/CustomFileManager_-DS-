#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
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

    // creating new directory
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

    string escapeNewlines(const string& str) {
        string result = "";
        for (char c : str) {
            if (c == '\n') {
                result += "\\n";
            } 
            else {
                result += c;
            }
        }
        return result;
    }

    void saveHelper(FileNode * node, string path, ofstream& outFile){
        if(path != "/"){ 
            path += "/";
        } 
        for(auto i :node->children){
            string childPath = path + i.first;
            //write to file path + fileName;
            if(i.second->isDirectory){
                outFile << "D "  << childPath << '\n';
                saveHelper(i.second, childPath,outFile);    
            }    
            else{
                outFile << "F " << childPath << " " << escapeNewlines(i.second->content) << '\n';
            }
        }
    }

    void save(){
        ofstream outFile("data.txt");
        if(!outFile.is_open()){
            cout << "Could not open the file\n";
            return;
        }
        saveHelper(root, "", outFile);

        outFile.close();
        cout << "Saved Successfully\n";
    }

    string unescapeNewLines(const string & content){
        string result = "";
        for(int i = 0; i< content.size() ; i++){
            if(content[i] == '\\' && i < content.size()-1 && content[i+1] == 'n'){
                result += '\n';
                i++;
            }
            else{
                result+= content[i];
            }
        }
        return result;
    }

    vector <string> splitPaths(const string & path){
        vector <string> parts;
        string s = "";
        for(int i =1; i < path.size(); i++){
            if(path[i] == '/' ){
                parts.push_back(s);
                s = "";
            }
            else s+= path[i];
        }
        if(!s.empty()){
            parts.push_back(s);
        }
        return parts;
    }

    void load(){
        ifstream inFile("data.txt");
        if(!inFile.is_open()){
            cout << "Could not open the file\n";
            return;
        }
        string type, path;
        while(inFile >> type >> path){
            string content = "";
            if(type == "F"){
                getline(inFile , content);
                if(!content.empty() && content[0] == ' '){
                    content = content.substr(1);
                }
                content = unescapeNewLines(content);
            }
            vector <string> paths = splitPaths(path);
            FileNode *temp = root;
            for(int i = 0; i < paths.size() ; i++){
                if(temp->children.contains(paths[i])){
                    temp =temp->children[paths[i]];
                }
                else{
                    if(i != paths.size()-1 || type == "D"){
                        currentDir =temp;
                        mkdir(paths[i]);
                        temp = temp->children[paths[i]];
                    }
                    else{
                        currentDir =temp;
                        touch(paths[i]);
                        temp->children[paths[i]]->content = content;
                    }
                }
            }
        }
        currentDir = root;
    }

    void treeHelper(FileNode * node, int depth){
        for(auto i :node->children){
            for(int i = 0; i < depth ; i++){
                cout << '\t';
            }
            cout << "|-- " << i.first << '\n';
            if(i.second->isDirectory){
                treeHelper(i.second, depth+1);
            }
        }
    }

    void tree(){
        cout << "/\n";
        treeHelper(root, 0);
    }

};

int main(){

}