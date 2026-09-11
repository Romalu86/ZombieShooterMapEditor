#pragma once
// VID_LIGHT owner. Included in ABI order by mapedit/runtime.hpp.
class VID_LIGHT : public VID {
public:
    VID_LIGHT();
    explicit VID_LIGHT(VID_LIGHT* source);
    virtual ~VID_LIGHT();
    virtual VID* CreateMirror();
    virtual void Draw(const SPRITE* sprite);
    virtual void Load(RESOURCE* res);
    virtual void SetLayer();

    int m_cadrSize;   // +0x490
    COLOR* m_cadrs;   // +0x494
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif

