/**
 * @brief Pop3 klient s podporu tsl/ssl
 * @details klient pro stahování emailů z POP3 serveru
 * @authors Jakub Komárek (xkomar33)
 */

#include <iostream>
#include <fstream>
#include <string>
#include <regex>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <sys/stat.h>
#include <unistd.h>

using namespace std;

class popClient
{
private:
    //přihlašovací údaje
    string login;   
    string password;

    //soubor z přihlašovacími údaji
    string loginFile;
    bool flagA=false;

    //adresa a port serveru
    string server;
    string port;
    bool flagP=false;

    bool flagT=false;
    bool flagS=false;

    //cesta k souboru s certifikáty
    string certFile;
    bool flagc=false;

    //složka s certifikáty
    string certFolder;
    bool flagC=false;

    bool flagD=false;
    bool flagN=false;

    //cesta ke složce, do které se ukládají emaily
    string outDir;
    bool flagO=false;

    //buffer pro komunikaci se serverem
    char buffer[1024];
    int buffSize=sizeof(buffer);

    //rozhraní pro komunikaci se serverem
    BIO *cbio=NULL;

    //počítadlo stažených souborů
    int downCounter=0;

    /**
     * @brief inicializace vstupní složky-pokud neexistuje, vytvoříse
     */ 
    void outDirInit();

    /**
     * @brief zíkává přihlašovací údaje ze souboru
     */ 
    void getLoginData();

    /**
     * @brief inicializace knihovny openSSL
     */ 
    void openSSLinit();

    /**
     * @brief Navázání nezabezpečeného spojení
     */ 
    void nonSecureConnet();

     /**
     * @brief načte certifikáty a konfigurci ssl
     * @return nosič informací ssl
     */ 
    SSL_CTX * loadCetificates();

    /**
     * @brief Navázání zabezpečeného spojení
     */ 
    void secureConnet();

    /**
     * @brief Navázání nezabezpečeného spojení a přepnutí do šifrovaného
     * @details sekce napsána za pomocí dokumentace: https://www.openssl.org/docs/man1.1.0/man3/BIO_new_ssl_connect.html
     */ 
    void switchToSecure();

    /**
     * @brief Zkontroluje uvítací zprávu ze serveru
     * @details sekce napsána za pomocí dokumentace: https://www.openssl.org/docs/man1.1.0/man3/BIO_new_ssl_connect.html
     */ 
    void checkHello();

    /**
     * @brief Autentizace na serveru pomocí loginu a hesla
     */ 
    void logIn();

    /**
     * @brief Získání počtu zpráv uložených na serveru
     * @return počet zpráv na serveru
     */ 
    int getMesCount();

    /**
     * @brief stažení všech zpráv na serveru
     */ 
    void download();

    /**
     * @brief stažení jedné zprávy
     * @return celá zpráva
     */ 
    string downloadMessege();

    /**
     * @brief zpracování zprávy a uložení do výstupní složky
     * @param messege celá zpráva
     * @return true-zprava úspěšně uložena,false-nebyla uložena
     */ 
    bool parseMessege(string messege);

    /**
     * @brief generuje v rámci výstupní složky unikátní náhodné jméno pro soubor neobsahující massege id
     * @return náhodné jméno
     */ 
    string genName();
public:

    /**
     * @brief Konstruktor-inicializuje se ze vstupních parametrů
     */ 
    popClient(int argc, char **argv);

    /**
     * @brief Destruktor
     */ 
    ~popClient();


    /**
     * @brief inicializační funkce před hlavním během programu
     */ 
    void init();

    /**
     * @brief Navázání spojení se serverem
     */ 
    void estConnection();

    /**
     * @brief Hlavní běh programu-stažení všech zpráv
     */ 
    void run();

    /**
     * @brief Vypsání výsledků
     */ 
    void writeResults();

    /**
     * @brief Odalokace zdrojů
     */ 
    void cleanUp();
};

/**
 * @brief kontroluje jestli se sufix shoduje z porovnávaným řetězcem
 * funkce převzata z https://stackoverflow.com/a/2072890
 * @param value porovnávaný string
 * @param ending sufix
 * @return true-shoduje se, false-neshoduje se
*/ 
inline bool ends_with(std::string const & value, std::string const & ending);