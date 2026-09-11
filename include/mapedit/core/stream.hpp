#pragma once
// STREAM/FSTREAM owners. Included in ABI order by mapedit/runtime.hpp.
class STREAM {
public:
    STREAM();
    virtual ~STREAM();
    virtual int Read(void* data,unsigned int size)=0;
    virtual int Write(const void* data,unsigned int size)=0;
};

class STRING_STREAM : public STREAM {
public:
    STRING string;
    unsigned int position;
    STRING_STREAM() : string(), position(0) {}
    virtual ~STRING_STREAM() {}
    int Read(void* data,unsigned int size) override;
    int Write(const void* data,unsigned int size) override;
};

class FSTREAM : public STREAM {
public:
    FILE* file;
    FSTREAM(const STRING* name,const char* mode);
    virtual ~FSTREAM();
    int IsOpen();
    int IsEnd();
    virtual int Read(void* data,unsigned int size);
    virtual int Write(const void* data,unsigned int size);
    int Length();
};

