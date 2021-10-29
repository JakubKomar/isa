/**
 * @brief Pop3 klient s podporu tsl/ssl
 * @details klient pro stahování emailů z POP3 serveru
 * @authors Jakub Komárek (xkomar33)
 */
#include "main.hpp"

int main(int argc, char *argv[]) 
{
    try
    {
        popClient client(argc,argv);
        client.estConnection();
        client.run();
        client.writeResults();
        client.cleanUp();
    }
    catch(invalid_argument & e)
    {
        cerr << e.what() << "\nnevalidní argument, skuste: popc -h" << '\n';
        return 1;
    }
    catch (exception & e) 
    {
        cout << "Runtime err: "<< e.what() << '\n';
        return 2;
    }
    
    return 0;
}