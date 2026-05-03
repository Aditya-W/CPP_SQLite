#include <iostream>
#include <string>

enum class MetaCommandResult
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
};

enum class PrepareResult
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
};

MetaCommandResult doMetaCommand(const std::string& command)
{
    if(command == ".exit")
    {
        std::cout << "Exiting database...\n";
        exit(EXIT_SUCCESS);
        return MetaCommandResult::META_COMMAND_SUCCESS;
    }
    
    return MetaCommandResult::META_COMMAND_UNRECOGNIZED_COMMAND;
}

PrepareResult prepareStatement(const std::string& input)
{
    if(input.length() >= 6 && input.substr(0,6)=="insert")
    {
        return PrepareResult::PREPARE_SUCCESS;
    }
    if(input.length() >= 6 && input.substr(0,6)=="select")
    {
        return PrepareResult::PREPARE_SUCCESS;
    }
    
    return PrepareResult::PREPARE_UNRECOGNIZED_STATEMENT;
}

int main()
{
    std::string userInput;

    while(true)
    {
        std::cout << "db > ";
        std::getline(std::cin, userInput);
        
        if(userInput[0] == '.')
        {
            if(doMetaCommand(userInput) == MetaCommandResult::META_COMMAND_UNRECOGNIZED_COMMAND)
            {
                std::cout << "Unrecognized Command... " << userInput << "\n";
                continue;
            }
        }
        
        PrepareResult result = prepareStatement(userInput);
        if(result == PrepareResult::PREPARE_SUCCESS)
        {
            std::cout << "Executed\n";
            continue;
        }
        else if(result == PrepareResult::PREPARE_UNRECOGNIZED_STATEMENT)
        {
            std::cout << "Unrecognized Keyword at the start of " << userInput << " .\n";
            continue;
        }
    }
}