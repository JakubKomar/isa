#include <string>
#include <string.h>
#include <stdexcept>  
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <regex>
using namespace std;
class popClient
{
private:
    string login;
    string password;
    bool flagA=false;
    string loginFile;
    string server;
    bool flagP=false;
    int port;
    bool flagT=false;
    bool flagS=false;
    bool flagc=false;
    string certFile;
    bool flagC=false;
    string certFolder;
    bool delFlag=false;
    bool newFlag=false;
    bool outFlag=false;
    string outDir;
    void init();
    void getLoginData();
public:
    popClient(int argc, char **argv);
    ~popClient();
};