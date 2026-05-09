#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <unistd.h>

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
    PREPARE_STRING_TOO_LONG,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR
};

enum class StatementType
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
};

enum class ExecuteResult
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
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

Table* newTable()
{
    Table* table = new Table();
    table->numRows = 0;
    
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        table->pages[i] = nullptr;
    }

    return table;
}

// Takes a source row and a destination
// and writes to the source to memory
void serializeRow(Row* source, void* destination)
{
    char* dest = static_cast<char*>(destination);

    memcpy(dest + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(dest + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    memcpy(dest + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}

// Reads from memory and writes to row object
void deserializeRow(void* source, Row* destination)
{
    char* sour = static_cast<char*>(source);

    memcpy(&(destination->id), sour + ID_OFFSET, ID_SIZE);
    memcpy(destination->username, sour + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(destination->email, sour + EMAIL_OFFSET, EMAIL_SIZE);
}

void* rowSlot(Table* table, uint32_t rowNum)
{
    //calculating which page will contain the row based on its number
    uint32_t pageNum = rowNum / ROWS_PER_PAGE;

    //fetching the page
    void* page = table->pages[pageNum];
    
    //if page is empty we allocate memory to it
    if(page == nullptr)
    {
        page = table->pages[pageNum] = new char[PAGE_SIZE];
    }

    //calculating rowoffset and byteoffset
    uint32_t rowOffset = rowNum % ROWS_PER_PAGE;
    uint32_t byteOffset = rowOffset * ROW_SIZE;

    //returning exact memory address by adding the byte offset
    return static_cast<char*>(page) + byteOffset;
}

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
        std::string username;
        std::string email;
        uint32_t id;

        if(iss >> keyword >> id >> username >> email)
        {
            if(username.length() > COLUMN_USERNAME_SIZE || email.length() > COLUMN_EMAIL_SIZE)
            {
                return PrepareResult::PREPARE_STRING_TOO_LONG;
            }

            statement.rowToInsert.id = id;

            strncpy(statement.rowToInsert.username, username.c_str(), COLUMN_USERNAME_SIZE);
            statement.rowToInsert.username[COLUMN_USERNAME_SIZE] = '\0';

            strncpy(statement.rowToInsert.email, email.c_str(), COLUMN_EMAIL_SIZE);
            statement.rowToInsert.email[COLUMN_EMAIL_SIZE] = '\0';

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

ExecuteResult executeStatement(Statement& statement, Table* table)
{
    switch(statement.type)
    {
        case StatementType::STATEMENT_INSERT:
        {
            if(table->numRows >= TABLE_MAX_ROWS)
            {
                return ExecuteResult::EXECUTE_TABLE_FULL;
            }
            void* destination = static_cast<void*>(rowSlot(table, table->numRows));
            serializeRow(&(statement.rowToInsert), destination);
            table->numRows += 1;
            std::cout << "Inserting: " << statement.rowToInsert.id << " " << statement.rowToInsert.username << " " << statement.rowToInsert.email << "\n";
            return ExecuteResult::EXECUTE_SUCCESS;
            break;
        }

        case StatementType::STATEMENT_SELECT:
        {
            for(uint32_t i = 0; i < table->numRows; i++)
            {
                Row row;
                void* source = static_cast<void*>(rowSlot(table, i));
                deserializeRow(source, &row);
                std::cout << "( " << row.id << ", " << row.username << ", " << row.email << " )\n";
            }
            return ExecuteResult::EXECUTE_SUCCESS;
            break;
        }
    }
    
    return ExecuteResult::EXECUTE_SUCCESS;
}

int main()
{
    std::string userInput;

    Table* table = newTable();

    bool isInteractive = isatty(STDIN_FILENO);

    while(true)
    {
        if(isInteractive)
            std::cout << "db > " << std::flush;
            
        if(!std::getline(std::cin, userInput))
            break;

        if(!userInput.empty() && userInput.back() == '\r')
            userInput.pop_back();

        if(userInput.empty())
            continue;
        
        if(userInput[0] == '.')
        {
            switch(doMetaCommand(userInput))
            {
                case MetaCommandResult::META_COMMAND_SUCCESS:
                    continue;
                case MetaCommandResult::META_COMMAND_UNRECOGNIZED_COMMAND:
                    std::cout << "Unrecognized Command... " << userInput << "\n";
                    continue;
            }
        }

        Statement statement;
        
        PrepareResult result = prepareStatement(userInput,statement);
        if(result == PrepareResult::PREPARE_SUCCESS)
        {
            if(executeStatement(statement, table) == ExecuteResult::EXECUTE_TABLE_FULL)
            {
                std::cout << "Error: Table is full\n";
            }
            std::cout << "Executed\n";
            continue;
        }
        else if(result == PrepareResult::PREPARE_STRING_TOO_LONG)
        {
            std::cout << "String is too long.\n";
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