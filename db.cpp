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

enum class StatementType
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
};

struct Statement
{
    StatementType type;
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

PrepareResult prepareStatement(const std::string& input, Statement& statement)
{
    if(input.length() >= 6 && input.substr(0,6)=="insert")
    {
        statement.type = StatementType::STATEMENT_INSERT;
        return PrepareResult::PREPARE_SUCCESS;
    }
    if(input.length() >= 6 && input.substr(0,6)=="select")
    {
        statement.type = StatementType::STATEMENT_SELECT;
        return PrepareResult::PREPARE_SUCCESS;
    }
    
    return PrepareResult::PREPARE_UNRECOGNIZED_STATEMENT;
}

void executeStatement(Statement& statement)
{
    switch(statement.type)
    {
        case StatementType::STATEMENT_INSERT:
            std::cout << "This is where the insert statement executes.\n";
            break;

        case StatementType::STATEMENT_SELECT:
            std::cout << "This is where the select statement executes.\n";
            break;
    }
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

        Statement statement;
        
        PrepareResult result = prepareStatement(userInput,statement);
        if(result == PrepareResult::PREPARE_SUCCESS)
        {
            executeStatement(statement);
            continue;
        }
        else if(result == PrepareResult::PREPARE_UNRECOGNIZED_STATEMENT)
        {
            std::cout << "Unrecognized Keyword at the start of " << userInput << " .\n";
            continue;
        }
    }
}