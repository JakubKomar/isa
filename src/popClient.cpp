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
			server=argv[1];
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
			(++i)<argc?certFolder=argv[i]:throw invalid_argument("Chybí složka z certifikáty za argumentem -C");
		}
		else if(!strcmp(argv[i],"-a"))
		{
			flagA=true;
			(++i)<argc?loginFile=argv[i]:throw invalid_argument("Chybí soubor s přihlašovacími údaji za argumentem -a");
		}
		else if(!strcmp(argv[i],"-o"))
		{
			flagO=true;
			(++i)<argc?outDir=argv[i]:throw invalid_argument("Chybí složka do které se má nahrát výstup za argumentem -o");
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
			(++i)<argc?certFile=argv[i]:throw invalid_argument("Chybí soubor s certiikáty za parametrem -c");
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
		if(flagT)
			port="995";   	//defalutní port pro navázání spop3 komunikace
		else
			port="110";		//defalutní port pro navázání pop3 komunikace
	}
}

void popClient::checkHello()
{
	BIO_read(cbio, buffer, buffSize);
	if (!regex_search (buffer, regex("^[+]OK",regex_constants::icase)))
		throw runtime_error("Chyba při komunikaci se serverem-wrong hallo answer(maybe the server is not pop3 type)");
}


void popClient::logIn()
{
	if(!flagS)	//hello paket byl již přijat v případě přepnutí do zabezpečené komunikace
		checkHello();
	
	BIO_puts(cbio,("user "+login+"\r\n").c_str()); 	//zadání loginu
	BIO_read(cbio, buffer, buffSize);


	if (!regex_search (buffer, regex("^[+]OK",regex_constants::icase)))
		throw runtime_error("Chyba při komunikaci se serverem");


	BIO_puts(cbio,("pass "+password+"\r\n").c_str());	//zadání hesla
	BIO_read(cbio, buffer, buffSize);

	if (regex_search (buffer, regex("^[+]OK",regex_constants::icase)))		//přihlášení proběhlo úspěšně
	{}
	else if(regex_search (buffer, regex("^[-]ERR",regex_constants::icase)))	//chyba
	{
		throw runtime_error("Invalid user name or password");
	}
	else
		throw runtime_error("Chyba při komunikaci se serverem");
}

void popClient::outDirInit()
{
	struct stat info;
	if(stat( outDir.c_str(), &info ))
	{
		if (errno == ENOENT) 
		{ 
			if (mkdir(outDir.c_str(),0) != 0)
			{
				throw runtime_error("Nelze vytvořit složku pro výstup");
			}
		} 
		else if (errno == ENOTDIR) 
		{ 
			throw runtime_error("Nelze přistoupit ke výstupní složce-něco v cestě není složka nebo nemáte dostatečné oprávnění pro přístup do této složky");
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
			BIO_read(cbio,buffer,buffSize);	//zahození odpovědi serveru
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
	if(flagN)
		cout << " nových";
	cout << " zpráv.\n";
}

inline bool ends_with(string const & value, string const & ending) // funkce převzata z: https://stackoverflow.com/a/2072890
{
	if (ending.size() > value.size()) 
		return false;
	return equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void popClient::parseMessege(string messege)
{
	string name="";
	smatch m; 

	istringstream iss(messege);

	for (string line; getline(iss, line);)    
	{
		if (regex_search (line,m, regex("^Message-ID: *<[!-~]*>",regex_constants::icase)))		//získání unikátního identifikátor z emailu->jméno souboru
		{
			name=m[0];
			regex_search(name, m, regex("<[!-~]*>")); 
			name=m[0];
			name=name.substr(1,name.size()-2);
			break;
		}
	}
	if(name=="")	//massege id nenalezeno->generování náhodného jména
		name=genName();

	string path=outDir+'/'+name+".eml";

	if(flagN) 
	{
		if( access( path.c_str(), F_OK ) == 0 ) 		//pokud záznam existuje, neukládáme ho znovu
			return;        
	}

	ofstream target(path.c_str());
	if (!target.is_open())
	{
		cerr<<"Soubor: "<<path<<" nelze otevřít pro zápis, pokračuji v práci.\n";
		cerr<<messege;
		return;
	}
	messege.erase(0, messege.find("\n") + 1);			//smazání prvního řádku "+ok ...."
	messege.erase(messege.length()-5,messege.length());	//smazání ukončující sekvence
	target<<messege;									//nahrátí zprávy do souboru
	target.close();
	downCounter++;
}

string popClient::genName()
{
	string path;
	string name;
	do{
		unsigned long int num=rand();
		name=to_string(num);
		path=outDir+'/'+name+".eml";
	}while (access( path.c_str(), F_OK ) == 0);
	return name;
}

int popClient::getMesCount()
{
	BIO_puts(cbio,"STAT\r\n");		//požadavek na výpis počtu zpráv
	BIO_read(cbio, buffer, buffSize);

	if (regex_search (buffer, regex("^[+]OK [0-9]+ [0-9]+",regex_constants::icase)))
	{
		string aux=buffer;
		smatch m; 
		if(regex_search(aux, m, regex("[0-9]+")))
			return stoi(m[0]);
		else
			throw runtime_error("Chyba při komunikaci se serverem");
	}
	else
		throw runtime_error("Chyba při komunikaci se serverem");
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
	if(flagT)
		secureConnet();
	else 
	{
		if(flagS)
		{
			nonSecureConnet();
			switchToSecure();
		}
		else
			nonSecureConnet();
	}
}

SSL_CTX * popClient::loadCetificates()
{
	SSL_CTX *ctx=SSL_CTX_new(SSLv23_client_method());
	if (flagc) 
	{
		if(!SSL_CTX_load_verify_locations(ctx, certFile.c_str(), NULL))		//načtení ze soboru
			throw runtime_error("Nelze načíst soubor s certifikáty");
	}
	else if (flagC) 
	{
		if(!SSL_CTX_load_verify_locations(ctx, NULL, certFolder.c_str()))	//načtení ze složky
			throw runtime_error("Nelze načíst složku s certifikáty"); 
	}
	else
		SSL_CTX_set_default_verify_paths(ctx);
	return ctx;
}

void popClient::nonSecureConnet()
{
	cbio = BIO_new_connect((server+":"+port).c_str());
	if (BIO_do_connect(cbio) <= 0) {
		throw runtime_error("Server je nedostupný");
	}
}

void popClient::switchToSecure()
{
	checkHello();

	BIO_puts(cbio,"STLS\r\n");	//požadavek na přepnutí do šifrované komunikace
	BIO_read(cbio, buffer, buffSize);
	if (!regex_search (buffer, regex("^[+]OK",regex_constants::icase)))
		throw runtime_error("Chyba při komunikaci se serverem-server pravděpodobně nepodporuje STLS příkaz");
	
	SSL_CTX *ctx=loadCetificates();
	BIO *sslB;
	if ((sslB = BIO_new_ssl(ctx, 1)) == NULL)
        throw runtime_error("Chyba při inicializaci ssl");
	if ((cbio = BIO_push(sslB, cbio)) == NULL)	//přepnutí do šifrované komunikace
        throw runtime_error("Běhová chyba: nepodařilo se reinicializovat soket pro šifrovanou komunikaci");
	
	if (BIO_do_connect(cbio) <= 0) 	//"přepojení" na šifrovanou komunikaci
		throw runtime_error("Server je nedostupný");

	SSL *ssl;
	BIO_get_ssl(cbio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	if (ssl == NULL) 
		runtime_error("Nepodařilo se navázat spojení se serverem");

	if(SSL_get_verify_result(ssl) != X509_V_OK)
		throw runtime_error("Ověření platnosti certifikátu se nezdařilo");
}

void popClient::secureConnet()
{
	SSL_CTX *ctx=loadCetificates();
	cbio = BIO_new_ssl_connect(ctx);

	SSL *ssl;
	BIO_get_ssl(cbio, &ssl);
	if (ssl == NULL) 
		runtime_error("Nepodařilo se navázat spojení se serverem");
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	BIO_set_conn_hostname(cbio, (server+":"+port).c_str());
	if (BIO_do_connect(cbio) <= 0) 
		throw runtime_error("Nepodařilo se navázat spojení se serverem");

	if(SSL_get_verify_result(ssl) != X509_V_OK)
		throw runtime_error("Ověření platnosti certifikátu se nezdařilo");
}

void popClient::cleanUp()
{
	if(cbio)
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
		string aux;
		getline(file,aux);
		if (regex_match (aux, regex("^username *= *[!-~]* *$",regex_constants::icase)))
		{
			smatch m; 
			regex_search(aux, m, regex("[!-~]* *$")); 
			aux=m[0];
			regex_search(aux, m, regex("[!-~]*"));  
			login=m[0];
		}
		else
			throw runtime_error("Soubor s přihlašujicími údaji je ve špatném formátu nebo je poškozený");
		getline(file,aux);
		if (regex_match (aux, regex("^password *= *[!-~]* *$",regex_constants::icase)))
		{
			smatch m; 
			regex_search(aux, m, regex("[!-~]* *$")); 
			aux=m[0];
			regex_search(aux, m, regex("[!-~]*"));  
			password=m[0];
		}
		else
			throw runtime_error("Soubor s přihlašujicími údaji je ve špatném formátu nebo je poškozený");
	}
	else 
		throw runtime_error("Soubor s přihlašujicími údaji nelze otevřít");
}

popClient::~popClient()
{
}
