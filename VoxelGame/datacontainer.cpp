#include "datacontainer.hpp"
#include <iostream>


void DataContainer::setNamedItem(const std::string& name, DataItemPtr item)
{
    item->setName(name);

    size_t n = entries.size();
    for (int i=0; i<n; i++) {
        if (entries[i]->getName() == name) {
            entries[i] = item;
            return;
        }
    }
    
    entries.push_back(item);
}

DataItemPtr DataContainer::getNamedItem(const std::string& name)
{
    for (auto i=entries.begin(); i!=entries.end(); ++i) {
        // std::cout << "Checking item named " << (*i)->getName() << std::endl;
        if ((*i)->getName() == name) return *i;
    }
    // std::cout << "Could not find " << name << std::endl;
    return 0;
}

void DataContainer::setIndexedItem(uint64_t ix, DataItemPtr item)
{
    item->setIndex(ix);
    
    size_t n = entries.size();
    for (int i=0; i<n; i++) {
        if (entries[i]->hasIndex() && entries[i]->getIndex() == ix) {
            entries[i] = item;
            return;
        }
    }
    
    entries.push_back(item);
}

DataItemPtr DataContainer::getIndexedItem(uint64_t ix)
{
    for (auto i=entries.begin(); i!=entries.end(); ++i) {
        if ((*i)->hasIndex() && (*i)->getIndex() == ix) return *i;
    }
    return 0;
}


static int item_byte_sizes[] = {
    1, // INT8,
    2, // INT16,
    4, // INT32,
    8, // INT64,
    4, // FLOAT,
    8, // DOUBLE,
    0, // CONTAINER,
};

static const char *item_name[] = {
    "INT8",
    "INT16",
    "INT32",
    "INT64",
    "FLOAT",
    "DOUBLE",
    "CONTAINER",    
};

static inline int arraySizeSize(uint64_t size) {
    if (size <= 255) return 1;
    if (size <= 65535) return 2;
    if (size <= 4294967295UL) return 4;
    return 8;
}

static inline int tagSizeSize(unsigned char tag) {
    tag &= DataItem::SIZE_SIZE_MASK;
    switch (tag) {
    case DataItem::SMALL_ARR:
        return 1;
    case DataItem::MED_ARR:
        return 2;
    case DataItem::LARGE_ARR:
        return 4;
    case DataItem::HUGE_ARR:
        return 8;
    default:
        return 0;
    }
    return 0;
}

static inline int arraySizeTag(uint64_t size) {
    if (size == 0) return DataItem::NO_ARR;
    if (size <= 255) return DataItem::SMALL_ARR;
    if (size <= 65535) return DataItem::MED_ARR;
    if (size <= 4294967295UL) return DataItem::LARGE_ARR;
    return DataItem::HUGE_ARR;
}

static inline void push_bytes(std::deque<char>& data, char *start, size_t len)
{
    // XXX Could fix this to work with big endian
    data.insert(std::end(data), start, start+len);
}

void DataItem::pack(std::deque<char>& data)
{
    uint64_t offset = 1;
    char tag_byte = item_type;

    if (name.size()) tag_byte |= NAMED;
    if (indexed) tag_byte |= INDEXED;
    if (array_count) {
        tag_byte |= arraySizeTag(array_count);
    }
    data.push_back(tag_byte);
    
    if (name.size()) {
        data.push_back((char)name.size());
        data.insert(std::end(data), std::begin(name), std::end(name));
    }
    
    if (indexed) {
        push_bytes(data, (char *)&index, 8);
    }
    
    if (item_type == CONTAINER) {
        DataContainerPtr container = getContainer();
        container->pack(data);
    } else {
        if (array_count == 0) {
            int item_size = item_byte_sizes[item_type];
            push_bytes(data, (char *)&data64, item_size);
        } else {
            int item_size = item_byte_sizes[item_type];
            int size_size = arraySizeSize(array_count);
            push_bytes(data, (char *)&array_count, size_size);
            uint64_t byte_count = array_count * item_size;
            push_bytes(data, ptr, byte_count);
        }
    }
}

static inline char next_byte(std::deque<char>& data)
{
    char b = data.front();
    data.pop_front();
    return b;
}

static void pop_string(std::deque<char>& data, uint64_t len, std::string& str)
{
    str.resize(len);
    for (int i=0; i<len; i++) {
        str[i] = next_byte(data);
    }
}

static void pop_bytes(std::deque<char>& data, uint64_t len, char *mem)
{
    std::copy(data.begin(), data.begin() + len, mem);
    data.erase(data.begin(), data.begin() + len);
}

DataItemPtr DataItem::unpack(std::deque<char>& data)
{
    unsigned char tag_byte = next_byte(data);
    // std::cout << "tag byte: " << (int)tag_byte << std::endl;
    unsigned char item_type = tag_byte & TYPE_MASK;
    int size_size = tagSizeSize(tag_byte);
    
    DataItemPtr item(new DataItem);
    item->item_type = item_type;

    // std::cout << "item_type:" << (int)item_type << " named=" << (!!(tag_byte&NAMED)) << " indexed=" << (!!(tag_byte&INDEXED)) << std::endl;
    
    if (tag_byte & NAMED) {
        int str_size = next_byte(data);
        pop_string(data, str_size, item->name);
        // std::cout << "name=" << item->name << std::endl;
    }
    
    if (tag_byte & INDEXED) {
        pop_bytes(data, sizeof(uint64_t), (char *)&item->index);
        item->indexed = true;
    }
    
    if (item_type == CONTAINER) {
        // std::cout << "Going to unpack contaner\n";
        item->container = DataContainer::unpack(data);
    } else {
        if (size_size) {            
            uint64_t array_count = 0;
            pop_bytes(data, size_size, (char *)&array_count);
            item->array_count = array_count;
            int item_size = item_byte_sizes[item_type];
            uint64_t byte_count = array_count * item_size;
            // std::cout << "byte count " << byte_count << std::endl;
            item->ptr = new char[byte_count];
            pop_bytes(data, byte_count, item->ptr);
        } else {
            int item_size = item_byte_sizes[item_type];
            item->data64 = 0;
            pop_bytes(data, item_size, (char *)&(item->data64));
        }
    }
    
    return item;
}


DataContainerPtr DataContainer::unpack(std::deque<char>& data)
{
    DataContainerPtr dc(new DataContainer);
    
    while (data.size()>0) {
        char tag = data.front();
        if (tag == DataItem::CONTAINER_END) {
            data.pop_front();
            // std::cout << "Returning container with " << dc->numItems() << " at " << ((void*)dc.get()) << std::endl;
            return dc;
        }
        
        // std::cout << "Container unpacking item " << dc->numItems() << std::endl;
        DataItemPtr di = DataItem::unpack(data);
        // std::cout << "Added item named " << di->getName() << std::endl;
        dc->entries.push_back(di);
    }
    
    // std::cout << "Returning container with " << dc->numItems() << std::endl;
    return dc;
}

void DataContainer::pack(std::deque<char>& data)
{
    for (auto i=entries.begin(); i!=entries.end(); ++i) {
        DataItemPtr di = *i;
        di->pack(data);
    }
    data.push_back(DataItem::CONTAINER_END);
}


void DataItem::debug(int level)
{
    std::string indent(level, ' ');
    if (array_count) {
        std::cout << indent << item_name[item_type] << '[' << array_count << ']';
        int item_size = item_byte_sizes[item_type];
        uint64_t num_bytes = array_count * item_size;
        for (uint64_t i=0; i<num_bytes; i++) printf(" %02x", 255 & ptr[i]);
    } else {
        std::cout << indent << item_name[item_type];
        if (item_type != CONTAINER) {
            printf(" %016llx", data64);
        }
    }
    
    
    
    if (name.size()) {
        std::cout << " name=" << name;
    }
    if (indexed) {
        std::cout << " index=" << index;
    }
    std::cout << std::endl;
    if (item_type == CONTAINER) {
        container->debug(level+1);
    }
}

void DataContainer::debug(int level)
{
    std::string indent(level, ' ');
    std::cout << indent << "Container has " << entries.size() << " items\n";
    for (auto i=entries.begin(); i!=entries.end(); ++i) {
        DataItemPtr di = *i;
        di->debug(level);
    }
}

#if 0
int main()
{
    DataContainerPtr dc = DataContainer::makeContainer();
    dc->addInt8(8);
    dc->addString("String");
    DataContainerPtr dc2 = DataContainer::makeContainer();
    dc->addContainer(dc2);
    dc2->addInt64(0xdeadbeef);
    DataContainerPtr dc3 = DataContainer::makeContainer();
    dc2->addContainer(dc3);
    dc->addInt16(80);
    dc3->addInt32(0xbad5);
    
    dc->debug();
    
    std::deque<char> serial;
    dc->pack(serial);
    for (int i=0; i<serial.size(); i++) printf("%02x ", 255 & serial[i]);
    printf("\n");
    
    DataContainerPtr dc4 = DataContainer::unpack(serial);
    dc4->debug();
    
    return 0;
}
#endif