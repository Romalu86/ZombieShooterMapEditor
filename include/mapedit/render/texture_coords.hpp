#pragma once
// VID_TEXCOOR owner. Included in ABI order by mapedit/runtime.hpp.
struct VID_TEXCOOR {
    int shadow_shift;      // +0x00
    int nsurf;             // +0x04
    int begx;              // +0x08
    int begy;              // +0x0C
    int sizex;             // +0x10
    int sizey;             // +0x14
    int shiftx;            // +0x18
    int shifty;            // +0x1C
    int next_fragment;     // +0x20
    void SetCoor(int x0,int y0,int x1,int y1,int next) {
        nsurf=0;
        next_fragment=next;
        shiftx=x0;
        const int width=x1-x0;
        sizex=width>256?256:width;
        shifty=y0;
        const int height=y1-y0;
        sizey=height>256?256:height;
    }
    int Intersection(int shift_x,int shift_y,int size_x,int size_y);
    void Read(STREAM* res,int new_version);
};

int Distance(int d1,int d2);
float Distance(float d1,float d2);
int Distance(int d1,int d2,int d3);
float Distance(float d1,float d2,float d3);
int Square(int val);
int Sqrt(int val);

