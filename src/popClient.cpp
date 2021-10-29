/**
 * @brief  Pop 3 klient
 *  klient pro stahování emailů ze serveru
 * @authors Jakub Komárek (xkomar33)
 */

#include "popClient.hpp"


popClient::popClient(int argc, char **argv)
{
    if(argc<2)
        throw invalid_argument("Nebyla zadána adresa serveru");
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
            (++i)<argc?this->port=argv[i]:throw invalid_argument("Chybí port za argumentem -p");
        }
        else if(!strcmp(argv[i],"-C"))
        {
            this->flagC=true;
            (++i)<argc?this->certFolder.assign(argv[i]):throw invalid_argument("Chybí složka z certifikáty za argumentem -C");
        }
        else if(!strcmp(argv[i],"-a"))
        {
            this->flagA=true;
            (++i)<argc?this->loginFile.assign(argv[i]):throw invalid_argument("Chybí soubor s přihlašovacími údaji za argumentem -a");
        }
        else if(!strcmp(argv[i],"-o"))
        {
            this->outFlag=true;
            (++i)<argc?this->outDir.assign(argv[i]):throw invalid_argument("Chybí složka do které se má nahrát výstup za argumentem -o");
        }
        else if((!strcmp(argv[i],"-T"))||(!strcmp(argv[i],"-S")))
        {
            if(this->flagT||this->flagS)
                throw invalid_argument("Nevalidní kombinace parametů -T a -S");
            if(!strcmp(argv[i],"-T"))
                this->flagT=true;
            else
                this->flagS=true;           
        }
        else if(!strcmp(argv[i],"-c"))
        {
            this->flagc=true;
            (++i)<argc?this->certFile.assign(argv[i]):throw invalid_argument("Chybí soubor s certiikáty za parametrem -c");
        }
        else if(!strcmp(argv[i],"-n"))
            this->newFlag=true;
        else if(!strcmp(argv[i],"-d"))
            this->delFlag=true;
        else
            throw invalid_argument("Argumen nebyl rozpoznán");
    }
    if((!this->flagA)||(!this->outFlag))
        throw invalid_argument("Chybí některý z povinných argumentů");
    init();
}

void popClient::logIn()
{
    BIO_read(cbio, buffer, buffSize);

    if (!regex_search (buffer, regex("^[+]OK POP3")))
        throw std::runtime_error("Chyba při komunikaci se serverem");

    
    BIO_puts(cbio,("user "+login+"\r\n").c_str()); 
    BIO_read(cbio, buffer, buffSize);


    if (!regex_search (buffer, regex("^[+]OK")))
        throw std::runtime_error("Chyba při komunikaci se serverem");


    BIO_puts(cbio,("pass "+password+"\r\n").c_str());
    BIO_read(cbio, buffer, buffSize);

    if (regex_search (buffer, regex("^[+]OK")))
    {}
    else if(regex_search (buffer, regex("^[-]ERR")))
    {
        throw std::runtime_error("Invalid user name or password.");
    }
    else
        throw std::runtime_error("Chyba při komunikaci se serverem");
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
                throw std::runtime_error("Nelze vytvořit složku pro výstup");
            }
        } 
        else if (errno == ENOTDIR) 
        { 
            throw std::runtime_error("Nelze přistoupit ke výstupní složce-něco v cestě není složka nebo nemáte dostatečné oprávnění pro přístup do této složky");
        } 
    }
}

void popClient::download()
{
    int mCount= getMesCount();

    for(int i=1;i<=mCount;i++)
    {
        snprintf(buffer, buffSize, "RETR %d\r\n",i);
        BIO_puts(cbio,buffer);

        string messege=downloadMessege();
        parseMessege(messege);
        if(this->delFlag)
        {         
            snprintf(buffer, buffSize, "DELE %d\r\n",i);
            BIO_puts(cbio,buffer);
            BIO_read(cbio,buffer,buffSize);
        }
    }
    BIO_puts(cbio,"QUIT\r\n");
}

string popClient::downloadMessege()
{
    string wholeMessege;
    string end="\r\n.\r\n";
    do
    {
        int recieved=BIO_read(cbio,buffer,buffSize); 
        wholeMessege.append(buffer,recieved);
    } while (!ends_with(wholeMessege,end));
    return wholeMessege;
}

void popClient::writeResults()
{
    cout<<"Staženo "<<this->downCounter;
    if(this->newFlag)
        cout<<" nových";
    cout<<" zpráv.\n";
}

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void popClient::parseMessege(string messege)
{
    string name;
    smatch m; 

    std::istringstream iss(messege);

    for (std::string line; std::getline(iss, line);)
    {
        if (regex_search (line,m, regex("^Message-ID: *<[!-~]*>")))
        {
            name=m[0];
            regex_search(name, m, regex("<[!-~]*>")); 
            name=m[0];
            name=name.substr(1,name.size()-2);
            break;
        }
    }

    string path=this->outDir+'/'+name;

    if(this->newFlag) 
    {
        if( access( path.c_str(), F_OK ) == 0 ) 
        {
            return;        
        }
    }

    ofstream target(path.c_str());
    if (!target.is_open())
    {
        cerr<<"Email: "<<path<<" nelze otevřít pro zápis, pokračuji v práci.";
        return;
    }
    messege.erase(0, messege.find("\n") + 1);
    target<<messege;
    target.flush();
    target.close();
    this->downCounter++;
}

int popClient::getMesCount()
{
    BIO_puts(cbio,"STAT\r\n");
    BIO_read(cbio, buffer, buffSize);

    if (regex_search (buffer, regex("^[+]OK [0-9]+ [0-9]+")))
    {
        string aux;
        aux.assign(buffer);
        smatch m; 
        if(regex_search(aux, m, regex("[0-9]+")))
            return stoi(m[0]);
        else
            throw std::runtime_error("Chyba při komunikaci se serverem");
    }
    else
        throw std::runtime_error("Chyba při komunikaci se serverem");
    return -1;
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
        throw std::runtime_error("Server je nedostupný");
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
        len = BIO_read(sbio, buffer, buffSize);
        if (len <= 0)
            break;
        BIO_write(out, buffer, len);
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
            throw std::runtime_error("Soubor s přihlašujicími údaji je ve špatném formátu nebo je poškozený");
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
            throw std::runtime_error("Soubor s přihlašujicími údaji je ve špatném formátu nebo je poškozený");
    }
    else 
        throw std::runtime_error("Soubor s přihlašujicími údaji nelze otevřít");
}

popClient::~popClient()
{
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
