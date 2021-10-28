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
            (++i)<argc?this->port=argv[i]:throw invalid_argument("Missing port");
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




void popClient::logIn()
{
    char tmpbuf[1024];
    int len = BIO_read(cbio, tmpbuf, 1024);strAddEnd(tmpbuf,len);

    if (!regex_search (tmpbuf, regex("^[+]OK POP3")))
    {
        fprintf(stderr, "Pop 3 comunication error\n");
        exit(1);
    }

    BIO_puts(cbio,("user "+login+"\r\n").c_str()); 
    len = BIO_read(cbio, tmpbuf, 1024); strAddEnd(tmpbuf,len);

    if (!regex_search (tmpbuf, regex("^[+]OK")))
    {
        fprintf(stderr, "Pop 3 comunication error\n");
        exit(1);
    }

    BIO_puts(cbio,("pass "+password+"\r\n").c_str());
    len = BIO_read(cbio, tmpbuf, 1024); strAddEnd(tmpbuf,len);

    if (regex_search (tmpbuf, regex("^[+]OK")))
    {
        cerr<<"login succes\n";
    }
    else if(regex_search (tmpbuf, regex("^[-]ERR")))
    {
        fprintf(stderr, "Invalid user name or password.\n");
        exit(1);
    }
    else
    {   
        fprintf(stderr, "Pop 3 comunication error\n");
        exit(1);
    }
}

void popClient::outDirInit()    //https://stackoverflow.com/questions/18100097/portable-way-to-check-if-directory-exists-windows-linux-c
{
    struct stat info;
    int statRC = stat( this->outDir.c_str(), &info );
    if( statRC != 0 )
    {
        if (errno == ENOENT) 
        { 
            if (mkdir(this->outDir.c_str(),0) != 0)
            {
                cerr<<"Cant create directory";
                exit(-1);
            }
        } 
        else if (errno == ENOTDIR) 
        { 
            cerr<<"somethig in path is not directory";
            exit(-1);
        } 
    }
}


void popClient::download()
{
    char buff[1024];
    int mCount= getMesCount();

    for(int i=1;i<=mCount;i++)
    {
        out= BIO_new_file(".temp","w");
        if(out==NULL)
            throw std::runtime_error("Temporary file cant be open");
        snprintf(buff, sizeof(buff), "RETR %d\r\n",i);
        BIO_puts(cbio,buff);
        

        int recieved=BIO_read(cbio,buff,sizeof(buff)); 
        int eol=countEol(buff,recieved);
        string aux;
        int len=0;
        aux.append(buff);
        smatch m; 
    
        if (regex_search (aux,m, regex("[+]OK [0-9]+ octets")))
        {
            aux=m[0];
            regex_search (aux,m, regex("[0-9]+"));
            len=stoi(m[0]);
        }
        else
            continue;
        
        
        int j=recieved;
        BIO_write(out,buff,recieved);
        while(j<len)
        {
            recieved=BIO_read(cbio,buff,sizeof(buff));
            BIO_write(out,buff,recieved);
            j+=recieved;
        }
        BIO_free_all(out);
        parseMessege();
    }
}

int countEol(char*  text,int len)
{
    int couter=0;
    for( int i=0;i<len;i++)
    {
        if(text[i]=='\n'||text[i]=='\r')
        {
            couter++;
        }
    }
    return couter;
}

void popClient::parseMessege()
{
    ifstream  file (".temp");
    string name;
    if (file.is_open())
    {
        while(getline(file,name))
        {
            if (regex_search (name, regex("^Message-ID: *<[!-~]*>")))
            {
                smatch m; 
                regex_search(name, m, regex("<[!-~]*>")); 
                name=m[0];
                name=name.substr(1,name.size()-2);
                cout<<name<<"\n";
                break;
            }
        } 
    }
    else 
        throw std::runtime_error("Temp file opening fail");
    if(!this->newFlag||(1)) //a zároveň záznam o zprávě neexistuje  
    {
        file.seekg(0);
        string aux;
        getline(file,aux);

        string path=this->outDir+'/'+name;
        ofstream target(path.c_str());
        if (!target.is_open())
        {
            cout<<"Target file opening fail";
            file.close();
            return;
        }
        while(getline(file,aux))
        {
            target<<aux;
        }
        target.flush();
        target.close();
    }

    file.close();
}

int popClient::getMesCount()
{
    int messegeC=0;
    char tmpbuf[1024];
    BIO_puts(cbio,"STAT\r\n");
    int len = BIO_read(cbio, tmpbuf, 1024);

    strAddEnd(tmpbuf,len);
    cout<<tmpbuf;
    if (regex_search (tmpbuf, regex("^[+]OK [0-9]+ [0-9]+")))
    {
        string aux;
        aux.assign(tmpbuf);
        smatch m; 
        regex_search(aux, m, regex("[0-9]+")); ;
        if(m.size()<1)
        {//err
        }
        if(m.size()>=0)
            return stoi(m[0]);
    }
    else
    {
        //err
    }
    return 0;
}

void popClient::strAddEnd(char *buffer,int wage)
{
    buffer[wage]='\0';
}


void popClient::run()
{
    logIn();
    outDirInit();
    download();
}

void popClient::openSSLinit()
{
    SSL_load_error_strings();
    ERR_load_crypto_strings();

    OpenSSL_add_all_algorithms();
    SSL_library_init();
}

void popClient::estConnection()
{
    if(this->flagS)
    {
        this->secureConnet();
    }
    else if(this->flagT)
    {
        this->nonSecureConnet();
        this->switchToSecure();
    }
    else
    {
        this->nonSecureConnet();
    } 
}

void popClient::nonSecureConnet()
{
    cbio = BIO_new_connect((this->server+":"+this->port).c_str());
    if (BIO_do_connect(this->cbio) <= 0) {
        fprintf(stderr, "Error connecting to server\n");
        ERR_print_errors_fp(stderr);
        cleanUp();
        exit(1);
    }
}

void popClient::switchToSecure()
{
    //todo
}

void popClient::secureConnet()
{
    BIO *sbio, *out;
    int len;
    char tmpbuf[1024];
    SSL_CTX *ctx;
    SSL *ssl;

    ctx = SSL_CTX_new(TLS_client_method());

    /* XXX Set verify paths and mode here. */

    sbio = BIO_new_ssl_connect(ctx);
    BIO_get_ssl(sbio, &ssl);
    if (ssl == NULL) {
        fprintf(stderr, "Can't locate SSL pointer\n");
        ERR_print_errors_fp(stderr);
        cleanUp();
        exit(1);
    }

    /* Don't want any retries */
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    /* XXX We might want to do other things with ssl here */

    /* An empty host part means the loopback address */
    BIO_set_conn_hostname(sbio, (this->server+":"+this->port).c_str());

    out = BIO_new_fp(stdout, BIO_NOCLOSE);
    if (BIO_do_connect(sbio) <= 0) {
        fprintf(stderr, "Error connecting to server\n");
        ERR_print_errors_fp(stderr);
        cleanUp();
        exit(1);
    }
    if (BIO_do_handshake(sbio) <= 0) {
        fprintf(stderr, "Error establishing SSL connection\n");
        ERR_print_errors_fp(stderr);
        cleanUp();
        exit(1);
    }

    /* XXX Could examine ssl here to get connection info */

    BIO_puts(sbio, "GET / HTTP/1.0\n\n");
    for ( ; ; ) {
        len = BIO_read(sbio, tmpbuf, 1024);
        if (len <= 0)
            break;
        BIO_write(out, tmpbuf, len);
    }
}

void popClient::cleanUp()
{
    BIO_free_all(this->cbio);
}

void popClient::init()
{
    getLoginData();
    openSSLinit();
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
