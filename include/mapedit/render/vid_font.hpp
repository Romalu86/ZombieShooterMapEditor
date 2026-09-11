#pragma once
// VID_FONT owner. Included in ABI order by mapedit/runtime.hpp.
class VID_FONT : public VID {
public:
    VID_FONT();
    explicit VID_FONT(VID_FONT* source);
    virtual ~VID_FONT();
    virtual VID* CreateMirror();
    virtual void Draw(const SPRITE* sprite);
    virtual void Load(RESOURCE* res);
    virtual void SetLayer();
    void RestoreDeviceObjects();
    void InvalidateDeviceObjects();

    CD3DFont* m_font; // +0x490
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif

// MapEdit ZS1: PTR_SPRITE is a 4-byte reference-counted SPRITE holder.
