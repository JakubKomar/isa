#include <string>
#include <string.h>
#include <stdio.h>
#include <stdexcept>  
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <regex>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <netdb.h>
#include <resolv.h>
#include <netdb.h>
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

    int sock=0;

    void init();
    void getLoginData();
    void initSocket();
    void nonSecure();
    void secureConnection();
public:
    void estConnection();
    popClient(int argc, char **argv);
    ~popClient();
};