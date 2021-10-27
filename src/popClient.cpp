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

void popClient::estConnection()
{
    initSocket();
    secureConnection();
}

void popClient::secureConnection()
{
    SSL_CTX *ctx;
    if(this->flagS)
    {
        ctx=SSL_CTX_new(TLS_client_method());
    }
    else
        {}
}



void popClient::nonSecure()
{

}

void popClient::initSocket()
{ 
    struct hostent *server=gethostbyname(this->server.c_str()); //nalezení aliasu ke jménu
    if (server == NULL) 
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    printf( "%s ", inet_ntoa( *( struct in_addr*)( server -> h_addr_list[0])));    //kontrolní výpis adresy z aliasu
    
    struct sockaddr_in serv_addr;       //adresa,port,..
    serv_addr.sin_family = AF_INET;     //ipv4 format
    serv_addr.sin_port = htons(this->port);//převod portu na float
    serv_addr.sin_addr= *( struct in_addr*)( server -> h_addr_list[0]); //ip adresa

    this->sock = socket(AF_INET, SOCK_STREAM, 0); //inicializace soketu
    if (sock < 0)
    {
        std::cerr << "Error: socket cration failed" << std::endl;
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        close(sock);
        exit(1);
    }

    char *hello = "Hello from client";
    char buffer[1024] = {0};
    int valread;

    send(this->sock , hello , strlen(hello) , 0 );
    printf("Hello message sent\n");
    valread = read(this->sock , buffer, 1024);
    printf("%s\n",buffer );
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
void error(const char *msg)
{
    perror(msg);
    exit(0);
}
