#include "popClient.hpp"


popClient::popClient(int argc, char **argv)
{
    if(argc<2)
        throw invalid_argument("Missing adress of server in arg");
    else
    {
        if(!strcmp(argv[1],"-h"))
        {
            cerr<<"popcl <server> [-p <port>] [-T|-S [-c <certfile>] [-C <certaddr>]] [-d] [-n] -a <auth_file> -o <out_dir>\n";
            exit(0);
        }
        else
            this->server.assign(argv[1]);
    }

    for(int i=2;i<argc;i++)
    {
        if(!strcmp(argv[i],"-p"))
        {
            this->flagP=true;
            (++i)<argc?this->port=stoi(argv[i]):throw invalid_argument("Missing port");
        }
        else if(!strcmp(argv[i],"-C"))
        {
            this->flagC=true;
            (++i)<argc?this->certFolder.assign(argv[i]):throw invalid_argument("Missing cert. folder");
        }
        else if(!strcmp(argv[i],"-a"))
        {
            this->flagA=true;
            (++i)<argc?this->loginFile.assign(argv[i]):throw invalid_argument("Missing login/pass file");
        }
        else if(!strcmp(argv[i],"-o"))
        {
            this->outFlag=true;
            (++i)<argc?this->outDir.assign(argv[i]):throw invalid_argument("Missing output folder");
        }
        else if((!strcmp(argv[i],"-T"))||(!strcmp(argv[i],"-S")))
        {
            if(this->flagT||this->flagS)
                throw invalid_argument("Invalid combination of params -T and -S");
            if(!strcmp(argv[i],"-T"))
                this->flagT=true;
            else
                this->flagS=true;           
        }
        else if(!strcmp(argv[i],"-c"))
        {
            this->flagc=true;
            (++i)<argc?this->certFile.assign(argv[i]):throw invalid_argument("Missing certificate file");
        }
        else if(!strcmp(argv[i],"-n"))
            this->newFlag=true;
        else if(!strcmp(argv[i],"-d"))
            this->delFlag=true;
        else
            throw invalid_argument("Arg not recognize");
    }
    if((!this->flagA)||(!this->outFlag))
        throw invalid_argument("missing reguaired param/s");
    init();
}

void popClient::init()
{
    getLoginData();
}   

void popClient::getLoginData()
{
    ifstream  file (this->loginFile);
    if (file.is_open())
    {
        std::string aux;
        getline(file,aux);
        if (regex_match (aux, regex("^username *= *[!-~]* *$")))
        {
            smatch m; 
   
            // regex_search that searches pattern regexp in the string mystr  
            regex_search(aux, m, regex("[!-~]* *$")); 
            aux=m[0];
            regex_search(aux, m, regex("[!-~]*"));  
            this->login=m[0];
        }
        else
            throw std::runtime_error("Login file in wrong format or damaged");
        getline(file,aux);
        if (regex_match (aux, regex("^password *= *[!-~]* *$")))
        {
            smatch m; 
   
            // regex_search that searches pattern regexp in the string mystr  
            regex_search(aux, m, regex("[!-~]* *$")); 
            aux=m[0];
            regex_search(aux, m, regex("[!-~]*"));  
            this->password=m[0];
        }
        else
            throw std::runtime_error("Login file in wrong format or damaged");
    }
    else 
        throw std::runtime_error("Login file cant be opened");
}
popClient::~popClient()
{
}
