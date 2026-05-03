#include <iostream>
#include <string>

int main()
{
    std::string userInput;

    while(true)
    {
        std::cout << "db > ";
        std::getline(std::cin, userInput);
        
        if(userInput == ".exit")
        {
            break;
        }
        else
        {
            std::cout << "Unrecognized Command: " << userInput << "\n";
        }
    }
}