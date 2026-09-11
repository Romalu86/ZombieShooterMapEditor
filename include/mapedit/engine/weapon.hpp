#pragma once
// WEAPON owner. Included in ABI order by mapedit/runtime.hpp.
class WEAPON {
public:
    uint32_t m_attackMask;       // +0x00 SpriteType; target VID type-mask
    uint32_t m_property;         // +0x04 Property; PropInTurn / PropSelfDirecting flags
    float m_length;              // +0x08 Length
    float m_weight;              // +0x0C Weight
    float m_power;               // +0x10 Power; train command-all gate
    float m_detectRange;         // +0x14 DetectRange
    float m_battleRange;         // +0x18 BattleRange
    float m_targetRadius;        // +0x1C WeaponAim; random target radius in CreateChild
    int m_reloadTime;           // +0x20 ReloadTime
    int m_buildTime;            // +0x24 BuildTime
    int m_maxAmmo;              // +0x28 MaxAmmo
    int m_army;                 // +0x2C DefaultArmy
    int m_defaultBehave;        // +0x30 DefaultBehave
    int m_icon;                 // +0x34 Icon
    int m_enemyRating;          // +0x38 EnemyRating
    float m_deadZone;           // +0x3C DeadZone
    unsigned long m_period;     // +0x40 Period
    // ZS1 runtime WEAPON record is 0x23C bytes. The resource record is 0x280
    // and MAP::LoadWeapon compacts/converts the tail in-place into this runtime
    // representation. Proven code uses raw offsets through +0x238, so retain
    // the remaining bytes rather than pretending the AS1 0x264 layout survives.
    unsigned char m_runtimeTail[0x23C-0x44];
    int PropInTurn() const;
    int PropSelfDirecting() const;
    int PropFrontEye() const;
    int PropRandomTarget() const;
    int PropAttackAnyArmy() const;
    int PropAttackNearOnly() const;
    int PropAnyDirFire() const;
    int PropMoved() const;
    float Interpolate(float* data,float coeff)
    {
        const int maxColumn=*reinterpret_cast<const int*>(
            reinterpret_cast<const unsigned char*>(this)+0x238);
        const int curColumn=static_cast<int>(coeff);
        if (curColumn>=maxColumn)
            return data[maxColumn];
        return data[curColumn]+(data[curColumn+1]-data[curColumn])*(coeff-static_cast<float>(curColumn));
    }
    int Interpolate(int* data,float coeff)
    {
        const int maxColumn=*reinterpret_cast<const int*>(
            reinterpret_cast<const unsigned char*>(this)+0x238);
        const int curColumn=static_cast<int>(coeff);
        if (curColumn>=maxColumn)
            return data[maxColumn];
        return static_cast<int>(static_cast<float>(data[curColumn]) +
                                static_cast<float>(data[curColumn+1]-data[curColumn]) *
                                (coeff-static_cast<float>(curColumn)));
    }
};

