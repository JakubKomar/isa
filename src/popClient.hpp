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
#include <sys/stat.h>

#include <unistd.h>
#include <netdb.h>

using namespace std;
int countEol(char * text,int len);

class popClient
{
private:
    string login;
    string password;

    bool flagA=false;
    string loginFile;

    string server;
    bool flagP=false;
    string port;

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

    BIO *cbio, *out;

    char buff[1024];

    void init();
    void openSSLinit();
    void getLoginData();
    void nonSecureConnet();
    void secureConnet();
    void switchToSecure();
    void logIn();
    void strAddEnd(char *buffer,int wage);
    void download();
    string downloadMessege();
    int getMesCount();
    void outDirInit();
    void parseMessege(string messege);
    void deleEndSeq(char * buff,int *received);

public:
    void estConnection();
    void run();
    popClient(int argc, char **argv);
    void cleanUp();
    ~popClient();
};

inline bool ends_with(std::string const & value, std::string const & ending);