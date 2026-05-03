#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>

const uint32_t COLUMN_USERNAME_SIZE = 32;
const uint32_t COLUMN_EMAIL_SIZE = 255;

enum class MetaCommandResult
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
};

enum class PrepareResult
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR
};

enum class StatementType
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
};

struct Row
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
};

struct Statement
{
    StatementType type;
    Row rowToInsert;
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

        std::istringstream iss(input);
        std::string keyword;

        if(iss >> keyword >> statement.rowToInsert.id >> statement.rowToInsert.username >> statement.rowToInsert.email)
        {
            return PrepareResult::PREPARE_SUCCESS;
        }
        else
        {
            return PrepareResult::PREPARE_SYNTAX_ERROR;
        }
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
            std::cout << "Inserting: " << statement.rowToInsert.id << " " << statement.rowToInsert.username << " " << statement.rowToInsert.email << "\n";
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
        else if(result == PrepareResult::PREPARE_SYNTAX_ERROR)
        {
            std::cout << "Syntax Error in " << userInput << ".\n";
            continue;
        }
    }
}