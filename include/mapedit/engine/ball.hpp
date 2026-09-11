#pragma once
// ZS1-only BALL owner (spriteClass 11), recovered from retail MapEditZS1.exe.
// The class derives directly from SPRITE and overrides only the deleting
// destructor and MoveTact vtable slots.
class BALL : public SPRITE {
public:
    BALL(VID*,float,float,float,ANGLE,SPRITE*);
    ~BALL();
    virtual void* ScalarDeletingDestructor(unsigned int flags) override;
    virtual void MoveTact() override;

    int state;                  // +0x70
    float lastX;                // +0x74
    float lastY;                // +0x78
    unsigned long underTime;    // +0x7C
    int impactCount;            // +0x80
};
