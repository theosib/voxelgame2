#ifndef INCLUDED_DATA_CONTAINER_HPP
#define INCLUDED_DATA_CONTAINER_HPP

#include <string.h>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <memory>

class DataContainer;
class DataItem;

typedef std::shared_ptr<DataContainer> DataContainerPtr;
typedef std::shared_ptr<DataItem> DataItemPtr;

class DataItem {
public:
    enum DataItemType {
        INT8,
        INT16,
        INT32,
        INT64,
        FLOAT,
        DOUBLE,
        CONTAINER,
        CONTAINER_END,
        TYPE_MASK = 7,
        
        INDEXED = 8,
        NAMED = 16,
        
        SIZE_SIZE_MASK = 7<<5,
        NO_ARR =    0<<5,
        SMALL_ARR = 1<<5,
        MED_ARR =   2<<5,
        LARGE_ARR = 3<<5,
        HUGE_ARR =  4<<5
    };
    
private:
    union {
        int8_t data8;
        int16_t data16;
        int32_t data32;
        int64_t data64;
        float fdata;
        double ddata;
        char *ptr;
    };
    uint64_t array_count;
    uint64_t index;
    std::string name;
    DataContainerPtr container;
    unsigned char item_type;
    bool indexed;
    
protected:
    DataItem() {
        array_count = 0;
        item_type = 0;
        indexed = 0;
        index = 0;
    }
    
public:    
    ~DataItem() {
        if (array_count) {
            delete[] ptr;
        }
    }
    
    static DataItemPtr makeInt8  (int8_t   i) { DataItemPtr di(new DataItem); di->item_type = INT8;   di->data8 =  i; return di; }
    static DataItemPtr makeInt16 (int16_t  i) { DataItemPtr di(new DataItem); di->item_type = INT16;  di->data16 = i; return di; }
    static DataItemPtr makeInt32 (int32_t  i) { DataItemPtr di(new DataItem); di->item_type = INT32;  di->data32 = i; return di; }
    static DataItemPtr makeInt64 (int64_t  i) { DataItemPtr di(new DataItem); di->item_type = INT64;  di->data64 = i; return di; }
    static DataItemPtr makeFloat (float    f) { DataItemPtr di(new DataItem); di->item_type = FLOAT;  di->fdata =  f; return di; }
    static DataItemPtr makeDouble(double   d) { DataItemPtr di(new DataItem); di->item_type = DOUBLE; di->ddata =  d; return di; }
    
    static DataItemPtr makeArray(const char *data, int type, size_t byte_count, size_t item_count) {
        DataItemPtr di(new DataItem);
        di->item_type = type;
        di->array_count = item_count;
        di->ptr = new char[byte_count];
        if (data) {
            memcpy(di->ptr, data, byte_count);
        } else {
            memset(di->ptr, 0, byte_count);
        }
        return di;
    }
    
    static DataItemPtr makeInt8Array  (size_t cnt, const int8_t   *p=0) { return makeArray((const char *)p, INT8  , cnt*1, cnt); }
    static DataItemPtr makeInt16Array (size_t cnt, const int16_t  *p=0) { return makeArray((const char *)p, INT16 , cnt*2, cnt); }
    static DataItemPtr makeInt32Array (size_t cnt, const int32_t  *p=0) { return makeArray((const char *)p, INT32 , cnt*4, cnt); }
    static DataItemPtr makeInt64Array (size_t cnt, const int64_t  *p=0) { return makeArray((const char *)p, INT64 , cnt*8, cnt); }
    static DataItemPtr makeFloatArray (size_t cnt, const float    *p=0) { return makeArray((const char *)p, FLOAT , cnt*4, cnt); }
    static DataItemPtr makeDoubleArray(size_t cnt, const double   *p=0) { return makeArray((const char *)p, DOUBLE, cnt*8, cnt); }
    
    static DataItemPtr makeString(const std::string& s) { return makeInt8Array(s.length(), (const int8_t *)s.c_str()); }
    
    static DataItemPtr wrapContainer(DataContainerPtr p) {
        DataItemPtr di(new DataItem);
        di->item_type = CONTAINER;
        di->container = p;
        return di;
    }
    
    void setName(const std::string& n) { name = n; }
    const std::string& getName() { return name; }
    void setIndex(uint64_t ix) { index = ix; indexed = true; }
    uint64_t getIndex() { return index; }
    bool hasIndex() { return indexed; }
    
    int8_t   getInt8()   { return data8; }
    int16_t  getInt16()  { return data16; }
    int32_t  getInt32()  { return data32; }
    int64_t  getInt64()  { return data64; }
    float    getFloat()  { return fdata; }
    double   getDouble() { return ddata; }
    
    uint64_t getArrayCount() { return array_count; }
    int getDataType()   { return item_type; }
    
    int8_t   *getInt8Array()   { return (int8_t*)ptr; }
    int16_t  *getInt16Array()  { return (int16_t*)ptr; }
    int32_t  *getInt32Array()  { return (int32_t*)ptr; }
    int64_t  *getInt64Array()  { return (int64_t*)ptr; }
    float    *getFloatArray()  { return (float*)ptr; }
    double   *getDoubleArray() { return (double*)ptr; }
    
    std::string_view getString() { return std::string_view((char*)ptr, array_count); }
    
    DataContainerPtr getContainer() { return container; }
    
    static DataItemPtr unpack(std::deque<char>& data);
    void pack(std::deque<char>& data);
    
    void debug(int level);
};

class DataContainer {
private:
    std::vector<DataItemPtr> entries;
    // int64_t packed_length;
    
protected:
    DataContainer() {}
public:
    ~DataContainer() {}
    
    void addInt8 (int8_t  i)   { entries.push_back(DataItem::makeInt8(i)); }
    void addInt16(int16_t i)   { entries.push_back(DataItem::makeInt16(i)); }
    void addInt32(int32_t i)   { entries.push_back(DataItem::makeInt32(i)); }
    void addInt64(int64_t i)   { entries.push_back(DataItem::makeInt64(i)); }
    void addFloat(float f)     { entries.push_back(DataItem::makeFloat(f)); }
    void addDouble(double d)   { entries.push_back(DataItem::makeDouble(d)); }
    void addString(const std::string& s) { entries.push_back(DataItem::makeString(s)); }
    void addContainer(DataContainerPtr dc) {
        entries.push_back(DataItem::wrapContainer(dc));
    }
    
    size_t numItems() { return entries.size(); }
    
    void addItem(DataItemPtr i)  { entries.push_back(i); }
    void setItem(int container_index, DataItemPtr i)  { entries[container_index] = i; }
    void setNamedItem(const std::string& name, DataItemPtr i);
    void setIndexedItem(uint64_t index, DataItemPtr i);
    
    DataItemPtr getItem(int container_index) { return entries[container_index]; }
    DataItemPtr getNamedItem(const std::string& name);
    DataItemPtr getIndexedItem(uint64_t index);
    
    static DataContainerPtr unpack(std::deque<char>& data);
    void pack(std::deque<char>& data);
    
    void debug(int level=0);
    
    static DataContainerPtr makeContainer() {
        return std::shared_ptr<DataContainer>(new DataContainer);
    }
};

#endif
