#pragma once
extern float FSin[256];
extern float FCos[256];

// ANGLE owner. Included in ABI order by mapedit/runtime.hpp.
class ANGLE {
public:
    uint8_t value;

    ANGLE();
    ANGLE(uint8_t direction) : value(direction) {}
    ANGLE(float x,float y);
    ANGLE(float x,float y,int* radius);
    ANGLE(const ANGLE* other);

    int operator==(const ANGLE* other);
    // ZS1 DrawSnow 0x00422670 performs the one-byte comparison inline.
    int operator!=(const ANGLE* other) { return value != other->value; }
    int operator<(const ANGLE* other);
    int operator<=(const ANGLE* other);
    int operator>(const ANGLE* other);
    ANGLE Difference(const ANGLE* other);
    ANGLE GetInversed();
    void Inverse();
    // ZS1 DrawSnow folds the historical self-check + one-byte assignment into the caller.
    const ANGLE* operator=(const ANGLE* other) { if (this != other) value=other->value; return this; }
    const ANGLE operator+(const ANGLE* other) { return ANGLE(static_cast<uint8_t>(static_cast<unsigned int>(value)+other->value)); }
    const ANGLE operator-(const ANGLE* other);
    int Int();
    float Sin() { return FSin[value]; }
    float Cos() { return FCos[value]; }
    float SinY();
    float CosY();
    float RotateX(float x,float y) { return x*Cos()-y*Sin(); }
    float RotateY(float x,float y) { return x*Sin()+y*Cos(); }
    void Write(STREAM* res);
    void Read(STREAM* res);
    static void Init();
};

ANGLE Decart2Polar(int x,int y,int* radius);
ANGLE Decart2Polar(float x,float y);
ANGLE Decart2Polar(float x,float y,float* radius);

