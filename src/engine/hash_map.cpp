#include "mapedit/runtime.hpp"

// ZS1 retail HASH_MAP::~HASH_MAP.
HASH_MAP::~HASH_MAP()
{
    units.Release();
    if (map) {
        for (int i=noX*noY-1;i>=0;--i)
            map[i].Release();
        delete[] map;
        map=0;
    }
}


int HASH_MAP::ConvX(float x)
{
    x*=scaleCellX;
    if (x<0.0f)
        return 0;
    if (x>=static_cast<float>(noX))
        return noX-1;
    return static_cast<int>(x);
}

int HASH_MAP::ConvY(float y)
{
    y*=scaleCellY;
    if (y<0.0f)
        return 0;
    if (y>=static_cast<float>(noY))
        return noY-1;
    return static_cast<int>(y);
}

SPRITE* HASH_MAP::FirstUnit(int* index)
{
    return units.BeginIterate(index);
}

SPRITE* HASH_MAP::NextUnit(int* index)
{
    return units.NextIterate(index);
}

SPRITE* HASH_MAP::FirstUnit()
{
    return units.BeginIterate(&curUnit);
}

SPRITE* HASH_MAP::NextUnit()
{
    return units.NextIterate(&curUnit);
}

namespace {
inline int HashCellX(HASH_MAP* hash,float x)
{
    x *= hash->scaleCellX;
    if (x < 0.0f)
        return 0;
    if (x >= (float)hash->noX)
        return hash->noX - 1;
    return (int)x;
}

inline int HashCellY(HASH_MAP* hash,float y)
{
    y *= hash->scaleCellY;
    if (y < 0.0f)
        return 0;
    if (y >= (float)hash->noY)
        return hash->noY - 1;
    return (int)y;
}
}

// ZS1 retail HASH_MAP::FirstInBox.
SPRITE* HASH_MAP::FirstInBox(float left,float top,float right,float bottom)
{
    begx = HashCellX(this,left - 1.0f / scaleCellX);
    begy = HashCellY(this,top - 1.0f / scaleCellY);
    endx = HashCellX(this,right + 1.0f / scaleCellX);
    endy = HashCellY(this,bottom + 1.0f / scaleCellY);
    curx = begx;
    curindex = 0;
    return NextInBox();
}

// ZS1 retail HASH_MAP::NextInBox.
SPRITE* HASH_MAP::NextInBox()
{
    while (begy <= endy) {
        while (curx <= endx) {
            SPRITE_LIST& cell = map[(begy << shiftY) + curx];
            if (curindex < cell.No())
                return *cell[curindex++];
            ++curx;
            curindex = 0;
        }
        ++begy;
        curx = begx;
        curindex = 0;
    }
    return 0;
}


// ZS1 retail HASH_MAP::ChangeCoor.
void HASH_MAP::ChangeCoor(SPRITE* sprite,float x,float y)
{
    const int oldX=HashCellX(this,sprite->X());
    const int oldY=HashCellY(this,sprite->Y());
    const int newX=HashCellX(this,x);
    const int newY=HashCellY(this,y);
    if (oldX==newX && oldY==newY)
        return;

    SPRITE_LIST& oldCell=map[(oldY << shiftY) + oldX];
    if (oldCell.Delete(sprite) != 0)
        return;
    map[(newY << shiftY) + newX].Insert(sprite);
}

// ZS1 retail HASH_MAP::CanPlace.
SPRITE* HASH_MAP::CanPlace(const VID* vid,float x,float y,float z)
{
    if (Map->GetGroundZ(vid,x,y) > z)
        return Mouse;
    if (vid->m_unknown18 == 0)
        return 0;

    SPRITE* sprite=FirstInBox(x-vid->m_snapOffsetX,
                              y-vid->m_snapOffsetY,
                              x+vid->m_snapOffsetX,
                              y+vid->m_snapOffsetY);
    while (sprite) {
        // ZS1 0x00443413..0x00443480 folds SPRITE::IsCross completely into
        // this owner: dying state, X/Y footprint and vertical overlap are
        // tested directly before the collision-mask intersection.
        VID* spriteVid=sprite->m_vid;
        if (sprite->m_ani < 15 &&
            fabsf(sprite->m_x-x) <= spriteVid->m_snapOffsetX+vid->m_snapOffsetX &&
            fabsf(sprite->m_y-y) <= spriteVid->m_snapOffsetY+vid->m_snapOffsetY &&
            sprite->m_z+spriteVid->m_hitVerticalOffset >= z &&
            z+vid->m_hitVerticalOffset >= sprite->m_z &&
            (spriteVid->m_unknown18 & vid->m_unknown18) != 0)
            return sprite;
        sprite=NextInBox();
    }
    return 0;
}

// ZS1 retail HASH_MAP::AskLine; integer conversion is truncation toward zero.
int HASH_MAP::AskLine(const VID* vid,float x,float y,float z,
                      float* endx,float* endy,float* endz)
{
    if (!vid || vid->m_unknown18 == 0)
        return 0;

    int x2=static_cast<int>(x);
    int y2=static_cast<int>(y);
    int z2=static_cast<int>(z);
    unsigned int dx=static_cast<unsigned int>(
        abs(static_cast<int>(*endx)-static_cast<int>(x)));
    unsigned int dy=static_cast<unsigned int>(
        abs(static_cast<int>(*endy)-static_cast<int>(y)));
    unsigned int steep=0;
    int sx=(*endx>x) ? 1 : -1;
    int sy=(*endy>y) ? 1 : -1;

    if (dy>dx) {
        steep=1;
        int t=x2; x2=y2; y2=t;
        unsigned int ut=dx; dx=dy; dy=ut;
        t=sx; sx=sy; sy=t;
    }

    int e=static_cast<int>(2u*dy-dx);
    int sz=0;
    if (dx!=0) {
        sz=((static_cast<int>(*endz)-static_cast<int>(z))<<4) /
           static_cast<int>(dx);
    }

    for (unsigned int i=0;i<dx;++i) {
        if ((i%16u)==0u && i>0u) {
            z2+=sz;
            SPRITE* hit;
            if (steep) {
                hit=CanPlace(vid,static_cast<float>(y2),static_cast<float>(x2),
                             static_cast<float>(z2));
                if (hit) {
                    *endx=static_cast<float>(y2);
                    *endy=static_cast<float>(x2);
                    *endz=static_cast<float>(z2);
                    return 1;
                }
            } else {
                hit=CanPlace(vid,static_cast<float>(x2),static_cast<float>(y2),
                             static_cast<float>(z2));
                if (hit) {
                    *endx=static_cast<float>(x2);
                    *endy=static_cast<float>(y2);
                    *endz=static_cast<float>(z2);
                    return 1;
                }
            }
        }

        while (e>=0) {
            y2+=sy;
            e-=static_cast<int>(2u*dx);
        }
        x2+=sx;
        e+=static_cast<int>(2u*dy);
    }
    return 0;
}

// ZS1 retail HASH_MAP::Insert. Duplicate InsertUnique return values are intentionally ignored.
void HASH_MAP::Insert(SPRITE* sprite)
{
    // ZS1 0x00443000 reads both VID flags directly.  Cell coordinates are
    // clamped in floating point first; the float-to-int conversion is only
    // emitted for the in-range branch.
    VID* vid=sprite->m_vid;
    if ((vid->m_flag & 0x40u)!=0) {
        const float fx=sprite->m_x*scaleCellX;
        int x;
        if (fx<0.0f)
            x=0;
        else if (fx>=static_cast<float>(noX))
            x=noX-1;
        else
            x=static_cast<int>(fx);

        const float fy=sprite->m_y*scaleCellY;
        int y;
        if (fy<0.0f)
            y=0;
        else if (fy>=static_cast<float>(noY))
            y=noY-1;
        else
            y=static_cast<int>(fy);

        map[x+(y<<shiftY)].InsertUnique(sprite);
    }
    if ((vid->m_unknown0C & 0x0Cu)!=0)
        units.InsertUnique(sprite);
}

// ZS1 retail HASH_MAP::Delete.
int HASH_MAP::Delete(SPRITE* sprite)
{
    int result=0;
    if (map && sprite->Vid()->PropHash()) {
        int x=static_cast<int>(sprite->X()*scaleCellX);
        if (x<0) x=0;
        else if (x>=noX) x=noX-1;
        int y=static_cast<int>(sprite->Y()*scaleCellY);
        if (y<0) y=0;
        else if (y>=noY) y=noY-1;

        if (x==curx && y==begy && curindex>0 &&
            curindex<map[x+(y<<shiftY)].No() &&
            *map[x+(y<<shiftY)][curindex-1]==sprite)
            --curindex;

        result=map[x+(y<<shiftY)].Delete(sprite);
    }
    if (sprite->IsSpriteType(0x0Cu))
        result|=2*units.Delete(sprite);

    if (result && sprite!=Mouse && (!Mouse || sprite!=Mouse->m_child))
        sprite->Error(10,"hash can't delete",static_cast<unsigned long>(result));
    return result;
}

// ZS1 retail HASH_MAP constructor.
HASH_MAP::HASH_MAP(float size_x,float size_y,VID** vids,int no_vid)
{
    // Retail 0x00450BF0 initializes only these iterator fields before the
    // sizing pass; the remaining box iterator fields are left untouched until
    // their respective query owners assign them.
    curUnit=0;
    curindex=0;

    float max_x=0.0f;
    float max_y=0.0f;
    for (int i=0;i<no_vid;++i) {
        VID* vid=vids[i];
        // Canonical ASM @ 0x00450C54 calls VID::PropHash @ 0x0041F5F0.
        // The pre-A11 reconstruction incorrectly used PropGround here.
        if (!vid || !vid->PropHash())
            continue;
        if (vid->m_footprintWidth>max_x)
            max_x=vid->m_footprintWidth;
        if (vid->m_footprintHeight>max_y)
            max_y=vid->m_footprintHeight;
    }


    // Retail divides both maxima by the 2.0f constant at 0x004B50BC.
    max_x/=2.0f;
    max_y/=2.0f;
    int shift_x=0;
    int shift_y_cell=0;
    while (static_cast<float>(1<<shift_x)<max_x)
        ++shift_x;
    while (static_cast<float>(1<<shift_y_cell)<max_y)
        ++shift_y_cell;

    scaleCellX=1.0f/static_cast<float>(1<<shift_x);
    scaleCellY=1.0f/static_cast<float>(1<<shift_y_cell);
    noY=static_cast<int>((size_y-1.0f)*scaleCellY+3.0f);

    shiftY=0;
    const float need_x=scaleCellX*(size_x-1.0f)+1.0f;
    while (static_cast<float>(1<<shiftY)<need_x)
        ++shiftY;
    noX=1<<shiftY;


    map=new SPRITE_LIST[noX*noY];
    if (!map)
        MYERROR::LogExit(Error,"!!!ERROR!!!HASH_MAP: Enough memory %i,%i",noX,noY);
}
