#pragma once
// POINTLIST/POLYGON/CRC32 owners. Included in ABI order by mapedit/runtime.hpp.
class POINTLIST {
public:
    float x;
    float y;
    POINTLIST* next;
};

class POLYGON {
public:
    POINTLIST* head;
    int closed;
    int noPoint;
    POLYGON();
    ~POLYGON();
    int NoPoint();
    int AskInside(float x,float y);
    POINTLIST* CreatePoint(float x,float y);
    void AddLinedPoint(float x,float y);
    void Closed();
    void CreateBox(float x0,float y0,float x1,float y1);
    void Draw(COLOR color);
    void Release();
};

class CRC32 {
public:
    CRC32();
    CRC32(void* buf,unsigned int count);
    operator unsigned int();
    unsigned int Add(void* buf,unsigned int count);
private:
    unsigned int crc;
    static unsigned int crc_table[256];
};

