/**
 * @brief Pop3 klient s podporu tsl/ssl
 * @details klient pro stahování emailů z POP3 serveru
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
			cout<<"Popcl je program pro stahování emailových zpráv z POP3 serveru.\n\n"
				"popcl <server> [-p <port>] [-T|-S [-c <certfile>] [-C <certaddr>]] [-d] [-n] -a <auth_file> -o <out_dir>\n\n"
				"<server> IP adresa/doménové jméno serveru.\n"
				"-p <port> specifikuje číslo portuserveru.\n"
				"-T zapíná šifrování celé komunikace (pop3s).\n"
				"-S naváže nešifrované spojení se serverem a pomocí příkazu STLS přejde na šifrovanou variantu protokolu.\n"
				"-c <certfile> definuje soubor s certifikáty, který se použije pro ověření platnosti certifikátu SSL/TLS předloženého serverem.\n"
				"-C <certaddr> určuje adresář, ve kterém se vyhledávají certifikáty, které se použijí pro ověření platnosti certifikátu SSL/TLS předloženého serverem.\n"
				"-d po stažení se zprávy odstraní ze serveru.\n"
				"-n stažení pouze nových zpráv.\n"
				"-a <auth_file> umístění souboru s přihlašovacíma údajemi.\n"
				"-o <out_dir> specifikuje výstupní adresář <out_dir>, do kterého program stažené zprávy ukládá.\n";
			exit(0);
		}
		else
			server.assign(argv[1]);
	}

	for(int i=2;i<argc;i++)
	{
		if(!strcmp(argv[i],"-p"))
		{
			flagP=true;
			(++i)<argc?port=argv[i]:throw invalid_argument("Chybí port za argumentem -p");
		}
		else if(!strcmp(argv[i],"-C"))
		{
			flagC=true;
			(++i)<argc?certFolder.assign(argv[i]):throw invalid_argument("Chybí složka z certifikáty za argumentem -C");
		}
		else if(!strcmp(argv[i],"-a"))
		{
			flagA=true;
			(++i)<argc?loginFile.assign(argv[i]):throw invalid_argument("Chybí soubor s přihlašovacími údaji za argumentem -a");
		}
		else if(!strcmp(argv[i],"-o"))
		{
			flagO=true;
			(++i)<argc?outDir.assign(argv[i]):throw invalid_argument("Chybí složka do které se má nahrát výstup za argumentem -o");
		}
		else if((!strcmp(argv[i],"-T"))||(!strcmp(argv[i],"-S")))
		{
			if(flagT||flagS)
				throw invalid_argument("Nevalidní kombinace parametů -T a -S");
			if(!strcmp(argv[i],"-T"))
				flagT=true;
			else
				flagS=true;           
		}
		else if(!strcmp(argv[i],"-c"))
		{
			flagc=true;
			(++i)<argc?certFile.assign(argv[i]):throw invalid_argument("Chybí soubor s certiikáty za parametrem -c");
		}
		else if(!strcmp(argv[i],"-n"))
			flagN=true;
		else if(!strcmp(argv[i],"-d"))
			flagD=true;
		else
			throw invalid_argument("Argumen nebyl rozpoznán");
	}
	if((!flagA)||(!flagO))
		throw invalid_argument("Chybí některý z povinných argumentů");
	if(!flagP)
	{
		if(flagT||flagS)
			port="993";   //maybe
		else
			port="110";

	}
	init();
}

void popClient::logIn()
{
	BIO_read(cbio, buffer, buffSize);

	if (!regex_search (buffer, regex("^[+]OK POP3")))
		throw std::runtime_error("Chyba při komunikaci se serverem");

	
	BIO_puts(cbio,("user "+login+"\r\n").c_str()); 	//zadání loginu
	BIO_read(cbio, buffer, buffSize);


	if (!regex_search (buffer, regex("^[+]OK")))
		throw std::runtime_error("Chyba při komunikaci se serverem");


	BIO_puts(cbio,("pass "+password+"\r\n").c_str());	//zadání hesla
	BIO_read(cbio, buffer, buffSize);

	if (regex_search (buffer, regex("^[+]OK")))		//přihlášení proběhlo úspěšně
	{}
	else if(regex_search (buffer, regex("^[-]ERR")))	//chyba
	{
		throw std::runtime_error("Invalid user name or password.");
	}
	else
		throw std::runtime_error("Chyba při komunikaci se serverem");
}

void popClient::outDirInit()    //čerpáno z tohoto příspěvku: https://stackoverflow.com/a/18101042
{
	struct stat info;
	int statRC = stat( outDir.c_str(), &info );
	if( statRC != 0 )
	{
		if (errno == ENOENT) 
		{ 
			if (mkdir(outDir.c_str(),0) != 0)
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
		snprintf(buffer, buffSize, "RETR %d\r\n",i);	//požadavek na stažení zpávy s číslem i
		BIO_puts(cbio,buffer);

		string messege=downloadMessege();
		parseMessege(messege);

		if(flagD)
		{         
			snprintf(buffer, buffSize, "DELE %d\r\n",i);	//požadavek na smazání zpávy s číslem i
			BIO_puts(cbio,buffer);		
			BIO_read(cbio,buffer,buffSize);	//zahození odpovědi
		}
	}
	BIO_puts(cbio,"QUIT\r\n");	//opuštění relace
}

string popClient::downloadMessege()
{
	string wholeMessege;
	string end="\r\n.\r\n";
	do
	{
		int recieved=BIO_read(cbio,buffer,buffSize); 
		wholeMessege.append(buffer,recieved);
	} while (!ends_with(wholeMessege,end));		//stahování dokud se nenarazí na ukončující sekvenci
	return wholeMessege;
}

void popClient::writeResults()
{
	cout << "Staženo " << downCounter;
	if(flagN){cout << " nových";}
	cout << " zpráv.\n";
}

inline bool ends_with(std::string const & value, std::string const & ending) // funkce převzata z: https://stackoverflow.com/a/2072890
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void popClient::parseMessege(string messege)
{
	string name;
	smatch m; 

	std::istringstream iss(messege);

	for (std::string line; std::getline(iss, line);)    //iterátor jsem získal na: https://stackoverflow.com/a/12514641
	{
		if (regex_search (line,m, regex("^Message-ID: *<[!-~]*>")))		//získání unikátního identifikátor z emailu->jméno souboru
		{
			name=m[0];
			regex_search(name, m, regex("<[!-~]*>")); 
			name=m[0];
			name=name.substr(1,name.size()-2);
			break;
		}
	}

	string path=outDir+'/'+name;

	if(flagN) 
	{
		if( access( path.c_str(), F_OK ) == 0 ) //pokud záznam existuje, neukládámeho znovu
			return;        
	}

	ofstream target(path.c_str());
	if (!target.is_open())
	{
		cerr<<"Soubor: "<<path<<" nelze otevřít pro zápis, pokračuji v práci.";
		return;
	}
	messege.erase(0, messege.find("\n") + 1);//smazání prvního řádku "+ok ...."
	target<<messege;	//nahrátí zprávy do souboru
	target.close();
	downCounter++;
}

int popClient::getMesCount()
{
	BIO_puts(cbio,"STAT\r\n");	//požadavek na výpis počtu zpráv
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
	if(flagS)
	{
		secureConnet();
	}
	else if(flagT)
	{
		nonSecureConnet();
		switchToSecure();
	}
	else
	{
		nonSecureConnet();
	} 
}

void popClient::nonSecureConnet()
{
	cbio = BIO_new_connect((server+":"+port).c_str());
	if (BIO_do_connect(cbio) <= 0) {
		throw std::runtime_error("Server je nedostupný");
	}
}

void popClient::switchToSecure()
{
	//todo
}

void popClient::secureConnet()
{
	BIO *out;
	int len;
	SSL_CTX *ctx;
	SSL *ssl;

	ctx = SSL_CTX_new(SSLv23_client_method());

	if (flagc) 
	{
		if(!SSL_CTX_load_verify_locations(ctx, certFile.c_str(), NULL))
			throw std::runtime_error("Nelze načíst soubor s certifikáty");
	}
	else if (flagC) 
	{
		if(!SSL_CTX_load_verify_locations(ctx, NULL, certFolder.c_str()))
			throw std::runtime_error("Nelze načíst složku s certifikáty"); 
	}
	else
		SSL_CTX_set_default_verify_paths(ctx);

	cbio = BIO_new_ssl_connect(ctx);
	BIO_get_ssl(cbio, &ssl);
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
	BIO_set_conn_hostname(cbio, (server+":"+port).c_str());

	out = BIO_new_fp(stdout, BIO_NOCLOSE);
	if (BIO_do_connect(cbio) <= 0) {
		fprintf(stderr, "Error connecting to server\n");
		ERR_print_errors_fp(stderr);
		cleanUp();
		exit(1);
	}
	if (BIO_do_handshake(cbio) <= 0) {
		fprintf(stderr, "Error establishing SSL connection\n");
		ERR_print_errors_fp(stderr);
		cleanUp();
		exit(1);
	}

	/* XXX Could examine ssl here to get connection info */

	BIO_puts(cbio, "GET / HTTP/1.0\n\n");
	for ( ; ; ) {
		len = BIO_read(cbio, buffer, buffSize);
		if (len <= 0)
			break;
		BIO_write(out, buffer, len);
	}
	exit(0);
}

void popClient::cleanUp()
{
	BIO_free_all(cbio);
}

void popClient::init()
{
	getLoginData();
	openSSLinit();
}   

void popClient::getLoginData()
{
	ifstream  file (loginFile);
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
			login=m[0];
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
			password=m[0];
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
