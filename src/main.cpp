/**
 * @brief   Pop3 client
 * klient pro stahování emailů ze serveru
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
    catch(const std::invalid_argument& e)
    {
        cerr << e.what()<<"\ninvalid argument - try -h" << '\n';
        return 1;
    }
    catch (std::exception &ex) 
    {
        std::cout << "Runtime err: "<< ex.what() << "\n";
    }
    
    return 0;
}