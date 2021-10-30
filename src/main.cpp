/**
 * @brief Pop3 klient s podporu tsl/ssl
 * @details klient pro stahování emailů z POP3 serveru
 * @authors Jakub Komárek (xkomar33)
 */
#include "main.hpp"

int main(int argc, char *argv[]) 
{
    auto retCode=0;
    try
    {
        popClient client(argc,argv);
        client.init();
        try
        {
            client.estConnection();
            client.run();
            client.writeResults();
        }
        catch (exception & e)   
        {
            cout << "Běhová chyba: "<< e.what() << ".\n";
            retCode=2;
        }
        client.cleanUp();   
    }
    catch(invalid_argument & e)
    {
        cerr << e.what() << "Nevalidní argument, zkuste: popc -h" << ".\n";
        retCode=1;
    }

    return retCode;
}