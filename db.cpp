#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstring>

const uint32_t COLUMN_USERNAME_SIZE = 32;
const uint32_t COLUMN_EMAIL_SIZE = 255;

const uint32_t PAGE_SIZE = 4096;
const uint32_t TABLE_MAX_PAGES = 100;

const uint32_t ID_SIZE = sizeof(uint32_t);
const uint32_t USERNAME_SIZE = sizeof(char) * (COLUMN_USERNAME_SIZE + 1);
const uint32_t EMAIL_SIZE = sizeof(char) * (COLUMN_EMAIL_SIZE + 1);

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t ROWS_PER_PAGE = PAGE_SIZE/ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = TABLE_MAX_PAGES * ROWS_PER_PAGE;

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

struct Table
{
    uint32_t numRows;
    void* pages[TABLE_MAX_PAGES];
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

void serializeRow(Row* source, void* destination)
{
    char* dest = static_cast<char*>(destination);

    memcpy(dest + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(dest + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    memcpy(dest + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}

void deserializeRow(void* source, Row* destination)
{
    char* sour = static_cast<char*>(source);

    memcpy(&(destination->id), sour + ID_OFFSET, ID_SIZE);
    memcpy(destination->username, sour + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(destination->email, sour + EMAIL_OFFSET, EMAIL_SIZE);
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