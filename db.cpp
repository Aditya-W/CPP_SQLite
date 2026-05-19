#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>

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

struct Pager
{
    int fileDescriptor;
    uint32_t fileLength;
    void* pages[TABLE_MAX_PAGES];
};

struct Table
{
    uint32_t numRows;
    Pager* pager;
};

struct Cursor
{
    Table* table;
    uint32_t rowNum;
    bool endOfTable; // boolean flag to tell us if we have reached the end of the table
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

// opens a file and initializes a pager
Pager* pagerOpen(const char* filename)
{
    // _O_RDWR means Read/Write. _O_CREAT means create it if it doesn't exist.
    // _S_IREAD | _S_IWRITE sets the Windows file permissions.
    int fd = _open(filename, _O_RDWR | _O_CREAT | _O_BINARY , _S_IREAD | _S_IWRITE);

    if(fd == -1)
    {
        std::cout << "Unable to open file.\n";
        exit(EXIT_FAILURE);
    }

    // how many bytes are in the file
    uint32_t fileLength = _filelength(fd);

    Pager* pager = new Pager();
    pager->fileDescriptor = fd;
    pager->fileLength = fileLength;

    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        pager->pages[i] = nullptr;
    }

    return pager;
}

// creating a new table object
Table* dbOpen(const char* filename)
{
    Pager* pager = pagerOpen(filename);

    Table* table = new Table();
    table->pager = pager;
    table->numRows = pager->fileLength / ROW_SIZE;

    return table;
}

// this function saves data to hard drive and empties cache before exiting
void dbClose(Table* table)
{
    uint32_t numFullPages = table->numRows / ROWS_PER_PAGE;
    uint32_t numAdditionalRows = table->numRows % ROWS_PER_PAGE;    

    Pager* pager = table->pager;

    for(uint32_t i = 0; i < numFullPages; i++)
    {
        // We only want to save pages that actually exist in the cache
        if(pager->pages[i] == nullptr)
            continue;

        // Moving the hard drive's write head to the correct location
        _lseek(pager->fileDescriptor, i * PAGE_SIZE, SEEK_SET);
        // Writing PAGE_SIZE bytes from RAM into file
        _write(pager->fileDescriptor, pager->pages[i], PAGE_SIZE);
    }

    if(numAdditionalRows > 0)
    {
        if(pager->pages[numFullPages] != nullptr)
        {
            _lseek(pager->fileDescriptor, numFullPages * PAGE_SIZE, SEEK_SET);
            _write(pager->fileDescriptor, pager->pages[numFullPages], numAdditionalRows * ROW_SIZE);
        }
    }

    // Freeing up memory
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        void* page = pager->pages[i];

        if(page)
        {
            delete[] static_cast<char*>(page);
            pager->pages[i] = nullptr;
        }
    }

    delete pager;
    delete table;

    // Safely releasing file back to Windows
    _close(pager->fileDescriptor);
}

// creating a brand new cursor object
Cursor* tableStart(Table* table)
{
    Cursor* cursor = new Cursor();
    cursor->table = table;
    cursor->rowNum = 0;

    if(table->numRows == 0)
        cursor->endOfTable = true;
    else
        cursor->endOfTable = false;

    return cursor;
}

// This function creates a cursor that represents the very end of our database
// where a new row should be appended
Cursor* tableEnd(Table* table)
{
    Cursor* cursor = new Cursor();
    cursor->table = table;
    cursor->rowNum = table->numRows;
    cursor->endOfTable = true;

    return cursor;
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

// returns if page exists in pager ram
// otherwise reads and loads from hard drive
void* getPage(Pager* pager, uint32_t pageNum)
{
    if(pageNum > TABLE_MAX_PAGES)
    {
        std::cout << "Tried to fetch page number out of bounds. " << pageNum << "\n";
        exit(EXIT_FAILURE);
    }

    // Cache miss - page doesn't exist in memory
    if(pager->pages[pageNum] == nullptr)
    {
        void* page = new char[PAGE_SIZE];

        // If page already exists in hard drive
        uint32_t numPages = pager->fileLength / PAGE_SIZE;

        // If file length isn't a perfect multiple
        // last saved page is partially full
        if(pager->fileLength % PAGE_SIZE)
        {
            numPages += 1;
        }

        if(pageNum <= numPages)
        {
            // move the file reader head to the exact byte where the page started
            _lseek(pager->fileDescriptor, pageNum * PAGE_SIZE, SEEK_SET);

            // read PAGE_SIZE bytes from file descriptor to our page
            int bytesRead = _read(pager->fileDescriptor, page, PAGE_SIZE);
            if(bytesRead == -1)
            {
                std::cout << "Error reading file.\n";
                exit(EXIT_FAILURE);
            }
        }

        // Save the loaded page into our pager
        pager->pages[pageNum] = page;
    }

    return pager->pages[pageNum];
}

// asks pager for page, calculates the offset and returns the pointer(memory address)
void* cursorValue(Cursor* cursor)
{
    //calculating which page will contain the row based on its number
    uint32_t pageNum = cursor->rowNum / ROWS_PER_PAGE;

    //fetching the page
    void* page = getPage(cursor->table->pager, pageNum);

    //calculating rowoffset and byteoffset
    uint32_t rowOffset = cursor->rowNum % ROWS_PER_PAGE;
    uint32_t byteOffset = rowOffset * ROW_SIZE;

    //returning exact memory address by adding the byte offset
    return static_cast<char*>(page) + byteOffset;
}

// function to move the cursor forward
void cursorAdvance(Cursor* cursor)
{
    cursor->rowNum += 1;
    
    if(cursor->rowNum >= cursor->table->numRows)
    {
        cursor->endOfTable = true;
    }
}

MetaCommandResult doMetaCommand(const std::string& command, Table* table)
{
    if(command == ".exit")
    {
        dbClose(table); // saving everything to hard drive
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
            //getting a bookmark to the end of memory
            Cursor* cursor = tableEnd(table);
            // getting the memory address that the bookmark points to
            void* destination = static_cast<void*>(cursorValue(cursor));
            // writing the data
            serializeRow(&(statement.rowToInsert), destination);
            table->numRows += 1;
            // deleting the object so that we don't leak RAM
            delete cursor;

            std::cout << "Inserting: " << statement.rowToInsert.id << " " << statement.rowToInsert.username << " " << statement.rowToInsert.email << "\n";
            return ExecuteResult::EXECUTE_SUCCESS;
            break;
        }

        case StatementType::STATEMENT_SELECT:
        {
            Cursor* cursor = tableStart(table);

            while(!(cursor->endOfTable))
            {
                Row row;
                
                void* source = static_cast<void*>(cursorValue(cursor));
                deserializeRow(source, &row);
                std::cout << "( " << row.id << ", " << row.username << ", " << row.email << " )\n";

                cursorAdvance(cursor);
            }

            delete cursor;

            return ExecuteResult::EXECUTE_SUCCESS;
            break;
        }
    }
    
    return ExecuteResult::EXECUTE_SUCCESS;
}

int main()
{
    std::string userInput;

    Table* table = dbOpen("mydb.db");

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
            switch(doMetaCommand(userInput, table))
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