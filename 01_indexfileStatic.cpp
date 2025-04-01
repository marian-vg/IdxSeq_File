#include <iostream>
#include <algorithm>
#include <string>

#define BLOCKS 3 // Number of blocks
#define N 3 // Number of record per block

#define PMAX N*BLOCKS // Maximum number of records per Data Area on the index file

#define OVER (PMAX+1) // Index where the overflow block starts
#define OMAX 3 // Limit of the overflow block

// -------------------------------------------------------------
// ----------------- Record Class ----------------------------
// -------------------------------------------------------------

template <typename T>
class Record
{
private:
    int key;
    T value;
	int direction; // direction to the record that has moved to the overflow area, or a new block
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

template <typename T, int CAPACITY>
class Block
{
private:
    Record<T> records[CAPACITY];

    int m_Size;
public:
    Block() : m_Size(0) {}

    bool IsFull() { return (m_Size >= CAPACITY); } // Check if the block is full

    Record<T>* getRecords() { return records; } // Get all the records in the block

    int getSize() { return m_Size; } // Get the size of the block

    int AddRecord(const Record<T>& rec)
    {
        if (!IsFull())
        {
            auto it = std::upper_bound(records, records + m_Size, rec, [](const auto& a, const auto& b)
            {
                return a.getKey() < b.getKey();
            }); // Find the position to insert the record
            
            int pos = it - records; // Get the position of the iterator

            // Move the records to the right to make space for the new record
            for (int i = m_Size; i > pos; i--)
            {
                records[i] = records[i - 1];
            }

            records[pos] = rec; // Insert the new record
            m_Size++; // Increase the size of the block

            return pos; // Return the position of the new record
        }

        return -1; // Return -1 if the block is full
    }
};

// -------------------------------------------------------------
// ----------------- DataArea Class ---------------------------
// -------------------------------------------------------------

template <typename T, int CAPACITY, int MAX_BLOCKS, int CAP_OVERFLOW>
class DataArea
{
private:
    Block<T, CAPACITY> m_Blocks[MAX_BLOCKS]; // Array of blocks
    Block<T, CAP_OVERFLOW> OverflowArea; // Overflow block
    
    int usedBlocks; // Number of blocks used
public:
    DataArea() : usedBlocks(0)
    {
        AddBlock(); // Add the first block
    }

    int getUsedBlocks() { return usedBlocks; } // Get the number of blocks used

    Block<T, CAPACITY>* getBlocks() { return m_Blocks; } // Get all the blocks in the Data Area

    Block<T, CAP_OVERFLOW>& getOverflow() { return OverflowArea; } // Get the overflow block

	// Add a new record to the Data Area
	int AddBlock()
	{
        if (usedBlocks < MAX_BLOCKS)
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

        Block<T, CAPACITY>& actualBlock = m_Blocks[index]; // Get the actual block

        // Check if the record is trying to be added at the end of the block

        // Get the actual size of the block
        int actualSize = actualBlock.getSize();

        // Get the records to compare with the new one
        Record<T>* records = actualBlock.getRecords();

        auto it = std::upper_bound(records, records + actualSize, rec, [](const auto& a, const auto& b)
        {
            return a.getKey() < b.getKey();
        }); // Find the position to insert the record

        int pos = it - records; // Get the position of the iterator (it)

        if (pos == actualSize 
           && actualSize >= (CAPACITY + 1)/2) // If the block is full 
        {
            // Try to add a new block
            int index2 = AddBlock();

            if (index2 != -1)
            {
                Block<T, CAPACITY>& newBlock = m_Blocks[index2];
                
                int pos2 = newBlock.AddRecord(rec);
                
                if (pos2 < 0)
                {
                    return "Error: New block is full";
                }

                return "Block " + std::to_string(index2);
            }
            else // No more blocks can be added, so the record will be added to the overflow area
            {
                return AddOverflow(rec);
            }
        }
        else // If the block is not full
        {
            int pos = actualBlock.AddRecord(rec); // Get the position of the new record
        
            if (pos >= 0) // If the position is valid and the block is not full
            {
                return "Block " + std::to_string(index);
            }
            else
            {
                return "Error: Record not added";
            }
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
        
        // Update the direction of the previous record
        UpdateDir(rec.getKey());

        return "Overflow Block";
    }

    void UpdateDir(int key)
    {
        // I will search for the record previous to the new one

        // Search for the record
        Record<T>* m_Rec = FindRecord(key);

        if (m_Rec != nullptr)
        {
            m_Rec->setDirection(OVER);
        }

    }

    Record<T>* FindRecord(int key)
    {
        Record<T>* prevRec = nullptr;

        // Search for the record that has the key greater than the new one
        for (int i = 0; i < usedBlocks; i++)
        {
            Block<T, CAPACITY>& block = m_Blocks[i];
            Record<T>* records = block.getRecords();
            int size = block.getSize();

            for (int j = 0; j < size; j++)
            {
                if (records[j].getKey() < key)
                {
                    
                    if (prevRec == nullptr 
                        || records[j].getKey() > prevRec->getKey())
                    {
                        prevRec = &records[j]; // Update the previous record
                    }
                }
            }
        }

        return prevRec;
    }
};

// -------------------------------------------------------------
// ----------------- IndexArea Class ---------------------------
// -------------------------------------------------------------

template <typename T, int BLOCK_COUNT>
class IndexArea
{
private:
    std::pair<int, int> key_dir[BLOCK_COUNT];

    DataArea<T, N, BLOCKS, OMAX>* m_Area;

    int index;
public:
    IndexArea(DataArea<T, N, BLOCKS, OMAX>* area) : index(0), m_Area(area) {}

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
        auto it = std::find_if(key_dir, key_dir + index, [indexBlock](auto& pair) { return pair.second == indexBlock; });

        if (it != (key_dir + index)) // If the indexBlock is found
        {
            it->first = key; // Update the key
        }
        else // If the indexBlock is not found
        {
            key_dir[index++] = { key, indexBlock }; // Add the new key and indexBlock
        }

        // Sort the index
        std::sort(key_dir, key_dir + index, [](auto& a, auto& b) { return a.first < b.first; });
    }
};

// -------------------------------------------------------------
// ----------------- Manager Class --------------------------------
// -------------------------------------------------------------

template<typename T>
class Manager
{
private:
    IndexArea<T, BLOCKS> m_IndexArea;
    DataArea<T, N, BLOCKS, OMAX> m_DataArea;

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
        Record<T> rec(key, value);

        // Get the index of the block
        int indexBlock = m_IndexArea.getIndexBlock(key);

        // Add the record to the Data Area
        std::string result = m_DataArea.AddRecordToData(indexBlock, rec);

        if (result == "Overflow Block"
            || (result.rfind("Block", 0) == 0)) // If the record was added successfully
        {
            std::cout << "\tKey " << key << " added successfully in " << result << std::endl;
            
            // It only updates the index if it was inserted in a main block
            if (result.rfind("Block", 0) == 0)
            {
                int pos = result.find(" "); // Find the position of the space after "Block"
                int index = std::stoi(result.substr(pos + 1)); // Get the index of the block

                Block<T, N>* blocks = m_DataArea.getBlocks();
                Block<T, N>& block = blocks[index];

                // Check if the block is not empty and update the index
                if (block.getSize() > 0)
                {
                    int newKey = block.getRecords()[0].getKey(); // Get the first key of the block to update the index
                    m_IndexArea.UpdateIndex(index, newKey);
                }
            }
        }
        else
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
        Block<T, N>& block = m_DataArea.getBlocks()[indexBlock];

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
        Block<T, OMAX>& over = m_DataArea.getOverflow();

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

        Block<T, N>* blocks = m_DataArea.getBlocks();

        // Show all the blocks in the Main Area

        for (int i = 0; i < BLOCKS; i++)
        {
            std::cout << "\t\tBlock: " << i << std::endl;

            // Get the current block
            Block<T, N>& c_Block = blocks[i];

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

        Block<T, OMAX>& m_Over = m_DataArea.getOverflow();

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

// -------------------------------------------------------------
// ----------------- Main Function -----------------------------
// -------------------------------------------------------------

int main()
{

    Menu();

    // Test code (if you don't want to use the menu)

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

    return 0;
}
