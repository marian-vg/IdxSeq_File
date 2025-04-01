#include <iostream>
#include <vector>
#include <algorithm>


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

    bool operator<(const Record<T>& other) const { return key < other.key; }
};

// -------------------------------------------------------------
// ----------------- Block Class ---------------------------
// -------------------------------------------------------------

template <typename T>
class Block
{
private:
    std::vector<Record<T>> records;

    int capacity; // Maximum number of records per block
public:
    Block(int cap) : capacity(cap) { records.reserve(capacity); } // Reserve space for N records

    bool IsFull() { return (records.size() >= capacity); } // Check if the block is full

    std::vector<Record<T>>& getRecords() { return records; } // Get all the records in the block

    int AddRecord(const Record<T>& rec)
    {
        if (!IsFull())
        {
            auto it = std::upper_bound(records.begin(), records.end(), rec); // Find the position to insert the record
            
            int pos = it - records.begin(); // Get the position of the iterator

            records.insert(it, rec); // Insert the new record (rec) in position (it) 

            // Sort the records
            std::sort(records.begin(), records.end(), [](auto& a, auto& b) { return a.getKey() < b.getKey(); });

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
    std::vector<Block<T>> m_Blocks;
    Block<T> OverflowArea;
    
    int capacity; // Registers per block
    int maxBlocks; // Maximum number of blocks -> defined by the user
    int usedBlocks; // Number of blocks used
public:
    DataArea(int cap, int nBlocks_, int capOverflow) : OverflowArea(capOverflow), capacity(cap), maxBlocks(nBlocks_), usedBlocks(0)
    {
        m_Blocks.reserve(maxBlocks); // Reserve space for the maximum number of blocks
        AddBlock(); // Add the first block
    }

    int getUsedBlocks() { return usedBlocks; } // Get the number of blocks used

    std::vector<Block<T>>& getBlocks() { return m_Blocks; } // Get all the blocks in the Data Area

    Block<T>& getOverflow() { return OverflowArea; } // Get the overflow block

	// Add a new record to the Data Area
	int AddBlock()
	{
        if (usedBlocks < maxBlocks)
        {
            m_Blocks.emplace_back(capacity);
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

        // Check if the record is trying to be added at the end of the block

        // Get the actual size of the block
        int actualSize = actualBlock.getRecords().size();

        // Get the records to compare with the new one
        auto& records = actualBlock.getRecords();

        auto it = std::upper_bound(records.begin(), records.end(), rec, [](const auto& a, const auto& b){
            return a.getKey() < b.getKey();
        }); // Find the position to insert the record

        int pos = it - records.begin(); // Get the position of the iterator (it)

        if (pos == actualSize && actualSize >= (capacity + 1)/2) // If the block is full 
        {
            // Try to add a new block
            int index2 = AddBlock();

            if (index2 != -1)
            {
                Block<T>& newBlock = m_Blocks[index2];
                
                int pos2 = newBlock.AddRecord(rec);
                
                if (pos2 < 0)
                {
                    return "Error: New block is full";
                }

                std::string result = "Block " + std::to_string(index2);
                return result;
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
                std::string result = "Block " + std::to_string(index);
                return result;
            }
            else
            {
                return "Error: Record not added";
            }
        }

        // int pos = actualBlock.AddRecord(rec); // Get the position of the new record
        
        // if (pos >= 0) // If the position is valid and the block is not full
        // {
        //     std::string result = "Block " + std::to_string(index);
        //     return result;
        // }
        // else // If the block is full
        // {
        //     // Try to add a new block
        //     int index2 = AddBlock();

        //     if (index2 != -1)
        //     {
        //         Block<T>& newBlock = m_Blocks[index2];
                
        //         int pos2 = newBlock.AddRecord(rec);
                
        //         if (pos2 < 0)
        //         {
        //             return "Error: New block is full";
        //         }

        //         std::string result = "Block " + std::to_string(index2);
        //         return result;
        //     }
        //     else // No more blocks can be added, so the record will be added to the overflow area
        //     {
        //         return AddOverflow(rec);
        //     } 
        // }
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
        for (auto& blocks : m_Blocks)
        {
            for (auto& rec : blocks.getRecords())
            {
                if (rec.getKey() < key)
                {
                    if (prevRec == nullptr || rec.getKey() > prevRec->getKey())
                    {
                        prevRec = &rec;
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

template <typename T>
class IndexArea
{
private:
    std::vector<std::pair<int, int>> key_dir;
    DataArea<T>* m_Area;
public:
    IndexArea(DataArea<T>* area) : m_Area(area) {}

    std::vector<std::pair<int, int>>& getKeyDir() { return key_dir; }

    int getIndexBlock(int key)
    {
        if (key_dir.empty()) // Return the first block if the index is empty
            return 0;

        int indexBlock = 0;

        for (int i = 0; i < key_dir.size(); i++)
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
        auto it = std::find_if(key_dir.begin(), key_dir.end(), [indexBlock](auto& pair) { return pair.second == indexBlock; });

        if (it != key_dir.end()) // If the indexBlock is found
        {
            it->first = key; // Update the key
        }
        else // If the indexBlock is not found
        {
            key_dir.push_back({ key, indexBlock}); // Add the new key and indexBlock
        }

        // Sort the index
        std::sort(key_dir.begin(), key_dir.end(), [](auto& a, auto& b) { return a.first < b.first; });
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

    Manager(int nBlocks, int cap, int capOverflow) 
        : m_DataArea(cap, nBlocks, capOverflow), m_IndexArea(&m_DataArea) 
        {
            if (!m_DataArea.getBlocks().empty())
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
            std::cout << "Key " << key << " added successfully in " << result << std::endl;
            
            // It only updates the index if it was inserted in a main block
            if (result.rfind("Block", 0) == 0)
            {
                int pos = result.find(" "); // Find the position of the space after "Block"
                int index = std::stoi(result.substr(pos + 1)); // Get the index of the block

                auto blocks = m_DataArea.getBlocks();
                auto& block = blocks[index];

                // Check if the block is not empty and update the index
                if (!block.getRecords().empty())
                {
                    int newKey = block.getRecords()[0].getKey(); // Get the first key of the block to update the index
                    m_IndexArea.UpdateIndex(index, newKey);
                }
            }
        }
        else
        {
            std::cout << "Error adding key " << key << " => (" << result << ")" << std::endl;
        }
    }

    void Search(int key)
    {
        int indexBlock = m_IndexArea.getIndexBlock(key);

        if (indexBlock < 0 || indexBlock >= m_DataArea.getUsedBlocks())
        {
            std::cout << "Record with key " << key << " not found (invalid block)." << std::endl;
            return;
        }

        // Search in the main block
        auto& block = m_DataArea.getBlocks()[indexBlock];

        for (auto& rec : block.getRecords())
        {
            if (rec.getKey() == key)
            {
                std::cout << "Record found (Block " << indexBlock << "): "
                          << "key = " << rec.getKey() << ", value = " << rec.getValue() 
                          << ", dir = " << rec.getDirection() << std::endl;
                return;
            }
        }

        // If the record is not in the main block, search in the overflow area
        auto over = m_DataArea.getOverflow();

        for (auto& rec : over.getRecords())
        {
            if (rec.getKey() == key)
            {
                std::cout << "Record found in the Overflow Area: "
                          << "key = " << rec.getKey() << ", value = " << rec.getValue() 
                          << ", dir = " << rec.getDirection() << std::endl;
                return;
            }
        }

        // If the record is not in the overflow area either
        std::cout << "Record with key " << key << " not found." << std::endl;
    }

    void Show()
    {
        std::cout << "\n--- Index Area ---" << std::endl;

        for (auto& pair : m_IndexArea.getKeyDir())
        {
            std::cout << "Key: " << pair.first << " => Dir: " << (pair.second * N) << std::endl;
        }

        ShowDataArea();
    }

    void ShowDataArea()
    {
        std::cout << "\n--- Data Area ---" << std::endl;

        int i = 0;

        // Show all the blocks in the Main Area

        for (auto& block : m_DataArea.getBlocks())
        {
            std::cout << "Block: " << i++ << std::endl;

            for (auto& rec : block.getRecords())
            {
                std::cout << "\t~ Key: " << rec.getKey() << " => Value: " << rec.getValue() << " => Direction: " << rec.getDirection() << std::endl;
            }
        }

        // Show the Overflow Area

        std::cout << "\n\t[Overflow Area]" << std::endl;

        auto m_Over = m_DataArea.getOverflow();
        for (auto& rec : m_Over.getRecords())
        {
            std::cout << "\t~ Key: " << rec.getKey() << " => Value: " << rec.getValue() << " => Direction: " << rec.getDirection() << std::endl;
        }
    }
};

// -------------------------------------------------------------
// ----------------- Main Function -----------------------------
// -------------------------------------------------------------

int main()
{

    Manager<std::string> m_Archive(BLOCKS, N, OMAX);

    // Block (0) first
    m_Archive.Add(1, "Value 10");
    m_Archive.Add(3, "Value 12");
    m_Archive.Add(4, "Value 16");

    // Block (1) second
    m_Archive.Add(2, "Value 14");
    m_Archive.Add(6, "Value 18");
    m_Archive.Add(8, "Value 20");

    // Block (2) third
    m_Archive.Add(5, "Value 19");
    m_Archive.Add(10, "Value 21");
    m_Archive.Add(9, "Value 22");

    // Overflow Area
    m_Archive.Add(13, "Valor 23");

    // Overflow Area
    m_Archive.Add(14, "Value 24");

    // Search

    std::cout << "\nSearch tests: " << std::endl;

    std::cout << "[~]\t";
    m_Archive.Search(12);
    std::cout << "[~]\t";
    m_Archive.Search(13);
    std::cout << "[~]\t";
    m_Archive.Search(14);

    std::cout << "\nSearch an invalid key: " << std::endl;

    std::cout << "\n[~]\t";
    m_Archive.Search(25);

    std::cout << "\nShowing the Index Area: " << std::endl;
    m_Archive.Show();

    return 0;
}