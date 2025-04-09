/*
Integrantes:
- Caminos Mariano
- Famea Damian
- Grigolato Facundo
- Pettinato Valentin
*/

#include <iostream>
#include <string>
#include <algorithm>

#define MAX_BLOCKS 10 // Maximum number of blocks
#define CAPACITY 10 // Number of records per block
#define MAX_OVERFLOW 10 // Maximum number of records in the overflow area
#define MAX_RECORDS 32 // Maximum number of records

int RECORDS = 9;
int N = 3;
int BLOCKS = RECORDS/N;
int OMAX = 3;

int OVER = ((N * BLOCKS) + 1);

// -------------------------------------------------------------
// ----------------- Record Class ----------------------------
// -------------------------------------------------------------

template <typename T>
class Record
{
private:
    int key;
    T value;
	int direction;
public:
	Record(int key_, T value_) : key(key_), value(value_), direction(-1) {}

    Record() : key(0), value(), direction(-1) {}

    int getKey() const { return key; }
    T getValue() const { return value; }
    int getDirection() const { return direction; }
    void setDirection(int dir) { direction = dir; }
};

// -------------------------------------------------------------
// ----------------- Block Class ---------------------------
// -------------------------------------------------------------

template <typename T>
class Block
{
private:
    Record<T> records[CAPACITY];

    int m_Size;

    int m_Capacity;
public:
    Block() : m_Size(0), m_Capacity(N) {}

    void setCapacity(int cap) { m_Capacity = cap; } // Set the capacity of the block

    bool IsFull() { return (m_Size >= CAPACITY); } // Check if the block is full

    Record<T>* getRecords() { return records; } // Get all the records in the block

    int getSize() { return m_Size; } // Get the size of the block

    void setSize(int size) { m_Size = size; } // Set the size of the block

    int AddRecord(const Record<T>& rec)
    {
        if (!IsFull())
        {
            int pos = -1;

            // Search for the position to insert the record
            for (int i = 0; i < m_Size; i++)
            {
                if (records[i].getKey() > rec.getKey())
                {
                    pos = i;
                    break;
                }
            }

            if (pos == -1) // If the position is not found, insert at the end
            {
                pos = m_Size;
            }

            // Move the records to the right to make space for the new record
            for (int i = m_Size; i > pos; i--)
            {
                records[i] = records[i - 1];
            }

            records[pos] = rec;
            m_Size++;

            return pos; // Return the position of the new record
        }

        return -1; // Return -1 if the block is full
    }
};

// -------------------------------------------------------------
// ----------------- DataArea Class ---------------------------
// -------------------------------------------------------------

template <typename T>
class DataArea
{
private:
    Block<T> m_Blocks[MAX_BLOCKS]; // Array of blocks
    Block<T> OverflowArea; // Overflow block
    
    int usedBlocks; // Number of blocks used
public:
    DataArea() : usedBlocks(0)
    {
        for (int i = 0; i < BLOCKS; i++)
        {
            m_Blocks[i].setCapacity(N);
        }
        OverflowArea.setCapacity(OMAX);

        AddBlock(); // Add the first block
    }

    int getUsedBlocks() { return usedBlocks; } // Get the number of blocks used

    Block<T>* getBlocks() { return m_Blocks; } // Get all the blocks in the Data Area

    Block<T>& getOverflow() { return OverflowArea; } // Get the overflow block

	// Add a new record to the Data Area
	int AddBlock()
	{
        if (usedBlocks < BLOCKS)
        {
            return usedBlocks++; 
        }

        return -1; // This means that the Data Area is full (no more blocks can be added)
	}

    std::string AddRecordToData(int index, const Record<T>& rec)
    {
        if (index < 0 || index >= usedBlocks) 
        {
            return "Error: Invalid block\n";
        }

        Block<T>& actualBlock = m_Blocks[index]; // Get the actual block

        // - Check if the record is trying to be added at the end of the block -

        // (1) Get the actual size of the block
        int actualSize = actualBlock.getSize();

        // (2) Get the records to compare with the new one
        Record<T>* records = actualBlock.getRecords();

        // (3) Search for the position to insert the record
        int pos = actualSize;

        for (int i = 0; i < actualSize; i++)
        {
            if (records[i].getKey() > rec.getKey())
            {
                pos = i;
                break;
            }
        }

        // (1) If the record is going to be inserted at the end of the block
        if (pos == actualSize && actualSize < N)
        {

            // (1.1) If the block is not full we try to add a new block
            if (actualSize >= (N + 1)/2)
            {

                int actualIndex = AddBlock(); 

                // If it can create a new block, we add the record on the new block
                if (actualIndex != -1)
                {
                    Block<T>& newBlock = m_Blocks[actualIndex];

                    int pos2 = newBlock.AddRecord(rec);

                    if (pos2 < 0)
                    {
                        return "Error: New block is full";
                    }

                    return "Block " + std::to_string(actualIndex);
                }
            }

            // (1.2) If it can't create a new block, we add the record on the current block
            int actualPos = actualBlock.AddRecord(rec);

            if (actualPos >= 0)
            {
                return "Block " + std::to_string(index);
            }
            else
            {
                return "Error: Record not added";
            }
        }
        // (2) If the record is not going to be inserted at the end of the block and the block is almost full ((N + 1)/2) add the record to the overflow area
        else if (pos != actualSize && actualSize >= (N + 1)/2)
        {
            if (actualSize > 0)
            {
                records[actualSize - 1].setDirection(OVER);
            }

            return AddOverflow(rec);
        }

        // (3) If the record is not going to be inserted at the end of the block and the block is not full, add the record to the block normally
        int m_Pos = actualBlock.AddRecord(rec);
        
        if (m_Pos >= 0)
        {
            return "Block " + std::to_string(index);
        }
        else
        {
            return "Error: Record not added";
        }
    }

    std::string AddOverflow(const Record<T>& rec)
    {
        if (OverflowArea.IsFull())
        {
            return "Error: Overflow is full";
        }

        // Add the record to the overflow area
        int pos = OverflowArea.AddRecord(rec);
        
        if (pos < 0)
        {
            return "Error: Overflow is full, can't be added";
        }
        
        return "Overflow Block";
    }

    // Check if the key exists in the Data Area or Overflow Area
    // This function is used to check if the key already exists before adding it
    // to avoid duplicates, cause the key is a unique identifier for the record

    bool CheckKey(int key)
    {
        // Check if the key exists in the Data Area
        for (int i = 0; i < usedBlocks; i++)
        {
            Block<T>& block = m_Blocks[i];
            Record<T>* records = block.getRecords();
            int size = block.getSize();

            for (int j = 0; j < size; j++)
            {
                if (records[j].getKey() == key)
                {
                    return true;
                }
            }
        }

        // Check if the key exists in the Overflow Area
        Block<T>& overBlock = OverflowArea;
        Record<T>* overRecords = overBlock.getRecords();
        int overSize = overBlock.getSize();

        for (int i = 0; i < overSize; i++)
        {
            if (overRecords[i].getKey() == key)
            {
                return true;
            }
        }

        return false;
    }
};

// -------------------------------------------------------------
// ----------------- IndexArea Class ---------------------------
// -------------------------------------------------------------

template <typename T>
class IndexArea
{
private:
    std::pair<int, int> key_dir[MAX_BLOCKS];

    DataArea<T>* m_Area;

    int index;
public:
    IndexArea(DataArea<T>* area) : index(0), m_Area(area) {}

    std::pair<int, int>* getKeyDir() { return key_dir; }

    int getIndex() { return index; }

    int getIndexBlock(int key)
    {
        if (index == 0) // Return the first block if the index is empty
            return 0;

        int indexBlock = 0;

        for (int i = 0; i < index; i++)
        {
            if (key_dir[i].first <= key)
            {
                indexBlock = key_dir[i].second;
            }
            else
            {
                break;
            }
        }

        return indexBlock;
    }

    void UpdateIndex(int indexBlock, int key)
    {
        // Search for the indexBlock in the index (key_dir vector)
        int pos = -1;

        for (int i = 0; i < index; i++)
        {
            if (key_dir[i].second == indexBlock)
            {
                pos = i;
                break;
            }
        }

        if (pos != -1) 
        {
            key_dir[pos].first = key;
        }
        else
        {
            key_dir[index++] = { key, indexBlock };
        }

        // Sort the index (key_dir vector) with bubble sort
        for (int i = 0; i < index - 1; i++)
        {
            for (int j = 0; j < index - i - 1; j++)
            {
                if (key_dir[j].first > key_dir[j + 1].first)
                {
                    std::swap(key_dir[j], key_dir[j + 1]);
                }
            }
        }

    }
};

// -------------------------------------------------------------
// ----------------- Manager Class --------------------------------
// -------------------------------------------------------------

template<typename T>
class Manager
{
private:
    IndexArea<T> m_IndexArea;
    DataArea<T> m_DataArea;

public:

    Manager() 
        : m_DataArea(), m_IndexArea(&m_DataArea) 
        {
            if (m_DataArea.getUsedBlocks() > 0)
            {
                m_IndexArea.UpdateIndex(0, -1);
            }
        }

    void Add(int key, T value)
    {

        // Check if the key exists
        if (m_DataArea.CheckKey(key) == true)
        {
            std::cout << "\tKey " << key << " already exists." << std::endl;
            return;
        }

        Record<T> rec(key, value);

        // Get the index of the block
        int indexBlock = m_IndexArea.getIndexBlock(key);

        // Add the record to the Data Area
        std::string result = m_DataArea.AddRecordToData(indexBlock, rec);

        std::string m_Result = result.substr(0, 6); // This is used to check if the result was "Block "

        if (result == "Overflow Block"
            || m_Result == "Block ")
        {
            std::cout << "\tKey " << key << " added successfully in " << result << std::endl;
            
            // It only updates the index if it was inserted in a main block
            if (m_Result == "Block ") // If the record was added in a main block
            {

                int index = std::stoi(result.substr(6)); // Get the index of the block

                Block<T>* blocks = m_DataArea.getBlocks();
                Block<T>& block = blocks[index];

                // Check if the block is not empty and update the index
                if (block.getSize() > 0)
                {
                    int newKey = block.getRecords()[0].getKey(); // Get the first key of the block to update the index
                    m_IndexArea.UpdateIndex(index, newKey);
                }
            }
        }
        else // If the record was not added successfully
        {
            std::cout << "\tError adding key " << key << " => (" << result << ")" << std::endl;
        }
    }

    void Search(int key)
    {
        int indexBlock = m_IndexArea.getIndexBlock(key);

        if (indexBlock < 0 || indexBlock >= m_DataArea.getUsedBlocks())
        {
            std::cout << "\tRecord with key " << key << " not found (invalid block)." << std::endl;
            return;
        }

        // Search in the main block
        Block<T>& block = m_DataArea.getBlocks()[indexBlock];

        Record<T>* records = block.getRecords();

        int size = block.getSize();

        for (int i = 0; i < size; i++)
        {
            if (records[i].getKey() == key)
            {
                std::cout << "\tRecord found (Block " << indexBlock << "): "
                          << "key = " << records[i].getKey() << ", value = " << records[i].getValue() 
                          << ", dir = " << records[i].getDirection() << std::endl;
                return;
            }
        }

        // If the record is not in the main block, search in the overflow area
        Block<T>& over = m_DataArea.getOverflow();

        Record<T>* over_records = over.getRecords();

        int over_size = over.getSize();

        for (int i = 0; i < over_size; i++)
        {
            if (over_records[i].getKey() == key)
            {
                std::cout << "\tRecord found in the Overflow Area: "
                          << "key = " << over_records[i].getKey() << ", value = " << over_records[i].getValue() 
                          << ", dir = " << over_records[i].getDirection() << std::endl;
                return;
            }
        }

        // If the record is not in the overflow area either
        std::cout << "\tRecord with key " << key << " not found." << std::endl;
    }

    void Show()
    {
        std::cout << "\n\t------------------------------------------" << std::endl;
        std::cout << "\n\t\t~~~ Index Area ~~~ \n" << std::endl;

        std::pair<int, int>* m_Pair = m_IndexArea.getKeyDir();

        int index = m_IndexArea.getIndex();

        for (int i = 0; i < index; i++)
        {
            std::cout << "\t\t[~] Key: " << m_Pair[i].first << " => Dir: " << (m_Pair[i].second * N) << std::endl;
        }

        ShowDataArea();
    }

    void ShowDataArea()
    {
        std::cout << "\n\t\t-----------------" << std::endl;
        std::cout << "\n\t\t~~~ Data Area ~~~ \n" << std::endl;

        Block<T>* blocks = m_DataArea.getBlocks();

        // Show all the blocks in the Main Area

        for (int i = 0; i < BLOCKS; i++)
        {
            std::cout << "\t\tBlock: " << i << std::endl;

            // Get the current block
            Block<T>& c_Block = blocks[i];

            Record<T>* records = c_Block.getRecords();

            std::cout << "\t------------------------------------------" << std::endl;
            for (int j = 0; j < N; j++)
            {
                std::cout << "\t~ Key: " << records[j].getKey() << " => Value: " << records[j].getValue() << " => Direction: " << records[j].getDirection() << std::endl;
            }
            std::cout << "\t------------------------------------------" << std::endl;
        }

        // Show the Overflow Area

        std::cout << "\n\t\t[Overflow Area] \n" << std::endl;

        Block<T>& m_Over = m_DataArea.getOverflow();

        Record<T>* over_records = m_Over.getRecords();

        for (int i = 0; i < OMAX; i++)
        {
            std::cout << "\t~ Key: " << over_records[i].getKey() << " => Value: " << over_records[i].getValue() << " => Direction: " << over_records[i].getDirection() << std::endl;
        }
        std::cout << "\n\t------------------------------------------" << std::endl;
    }

    
};

// -------------------------------------------------------------
// ----------------- Menu Function -----------------------------
// -------------------------------------------------------------

void Menu()
{
    Manager<std::string> m_Archive;

    while (true)
    {
        std::cout << "\n\t ----------- Menu ----------- " << std::endl; 

        std::cout << "\n\t[1] Add a record" << std::endl;
        std::cout << "\t[2] Search a record" << std::endl;
        std::cout << "\t[3] Show index/data area" << std::endl;
        std::cout << "\t[4] Exit" << std::endl;


        int choice;

        int key;

        std::string value;

        std::cout << "\n\t[~] Enter your choice:  ";
        std::cin >> choice;

        switch (choice)
        {
        case 1:
            std::cout << "\n\t[~] Enter the key:  ";
            std::cin >> key;

            std::cout << "\n\t[~] Enter the value:  ";
            std::cin >> value;

            if (key <= 0)
            {
                std::cout << "\n\tInvalid key.\n" << std::endl;
                break;
            }

            m_Archive.Add(key, value);
            break;
        case 2:
            std::cout << "\n\t[~] Enter the key:  ";
            std::cin >> key;

            m_Archive.Search(key);
            break;
        case 3:
            m_Archive.Show();
            break;
        case 4:
            return;
            break;
        default:
            std::cout << "\n\tInvalid choice.\n" << std::endl;
            break;
        }
        
    }
}

void Init()
{
    while (true)
    {
        std::cout << "\n\t\t ------------------------------------------" << std::endl;
        std::cout << "\n\t\t SETTINGS \n";
        std::cout << "\n\t\t[1] Set the number of records per block (N)" << std::endl;
        std::cout << "\n\t\t[2] Set the maximum number of records (RECORDS)" << std::endl;
        std::cout << "\n\t\t[3] Set the maximum number of records for the overflow area (OMAX)" << std::endl;
        std::cout << "\n\t\t[4] Keep default settings (N = 3, RECORDS = 9, OMAX = 3)" << std::endl;
        std::cout << "\n\t\t[5] Exit" << std::endl;
        std::cout << "\n\n\t\t ~~~~~ the max values for each parameter are * " << MAX_RECORDS << "(MAX_RECORDS) * " << CAPACITY << "(RECORDS_PER_BLOCK) * " << MAX_OVERFLOW << "(OVERFLOW_AREA) * " << std::endl;
        std::cout << "\n\t\t ~~~~~ the maximum number of records might can be divided by the number of records per block (N) ~~~~~" << std::endl;

        int choice;

        std::cout << "\n\t\t[~] Enter your choice:  ";
        std::cin >> choice;

        std::cout << "\n\t\t ------------------------------------------" << std::endl;

        switch (choice)
        {
            case 1:
                std::cout << "\n\t\t[~] Enter the number of records per block:  ";
                std::cin >> N;

                if (N <= 0 || N > CAPACITY)
                {
                    std::cout << "\n\tInvalid number of records per block.\n" << std::endl;
                    N = 3; 
                    break;
                }

                std::cout << "\n\t\t[*] the new number of records per block is * " << N << " *" << std::endl;

                break;

            case 2:
                std::cout << "\n\t\t[~] Enter the maximum number of records:  ";
                std::cin >> RECORDS;

                if (RECORDS <= 0 || RECORDS > MAX_RECORDS)
                {
                    std::cout << "\n\tInvalid maximum number of records.\n" << std::endl;
                    RECORDS = 9; 
                    break;
                }

                std::cout << "\n\t\t[*] the new maximum number of records is * " << RECORDS << " *" << std::endl;
                break;

            case 3:
                std::cout << "\n\t\t[~] Enter the maximum number of records for the overflow area:  ";
                std::cin >> OMAX;

                if (OMAX <= 0)
                {
                    std::cout << "\n\tInvalid maximum number of records for the overflow area.\n" << std::endl;
                    OMAX = 3; // Set to default value
                    break;
                }

                std::cout << "\n\t\t[*] the new maximum number of records for the overflow area is * " << OMAX << " *" << std::endl;

                break;

            case 4:
                std::cout << "\n\t\tKeep/set settings default (BLOCKS = 3, N = 3, OMAX = 3) :)\n" << std::endl;

                return;

                break;

            case 5:
                std::cout << "\n\t\tExiting...\n" << std::endl;

                return;

                break;

            default:
                std::cout << "\n\tInvalid choice.\n" << std::endl;

                break;
        }
    }
}

// -------------------------------------------------------------
// ----------------- Main Function -----------------------------
// -------------------------------------------------------------

int main()
{
    Init();

    Menu();

    //[*] Test code (if you don't want to use the menu and init function) [*]

    // Manager<std::string> m_Archive;

    // // Block (0) first
    // m_Archive.Add(1, "Value 10");
    // m_Archive.Add(3, "Value 12");
    // m_Archive.Add(4, "Value 16");

    // // Block (1) second
    // m_Archive.Add(2, "Value 14");
    // m_Archive.Add(6, "Value 18");
    // m_Archive.Add(8, "Value 20");

    // // Block (2) third
    // m_Archive.Add(5, "Value 19");
    // m_Archive.Add(10, "Value 21");
    // m_Archive.Add(9, "Value 22");

    // // Overflow Area
    // m_Archive.Add(13, "Valor 23");

    // // Overflow Area
    // m_Archive.Add(14, "Value 24");

    // // Search

    // std::cout << "\nSearch tests: " << std::endl;

    // std::cout << "[~]\t";
    // m_Archive.Search(12);
    // std::cout << "[~]\t";
    // m_Archive.Search(13);
    // std::cout << "[~]\t";
    // m_Archive.Search(14);

    // std::cout << "\nSearch an invalid key: " << std::endl;

    // std::cout << "\n[~]\t";
    // m_Archive.Search(25);

    // std::cout << "\nShowing the Index Area: " << std::endl;
    // m_Archive.Show();

    // std::cin.get();
}
