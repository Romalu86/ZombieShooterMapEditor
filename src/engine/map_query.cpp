#include "mapedit/runtime.hpp"

namespace {
enum SpriteTypeMask {
    U_TERRAIN = 0x001,
    U_OBJECT  = 0x002,
    U_UNIT    = 0x004,
    U_AVIA    = 0x008,
    U_MENU    = 0x010,
    U_RAILWAY = 0x020,
    U_REGION  = 0x040,
    U_CANNON  = 0x200,
    U_SPRITE  = 0x400
};

}

// ZS1 retail MAP::GetSpriteScr owner.
SPRITE* MAP::GetSpriteScr(int type,float screenX,float screenY)
{
    const float radius=300.0f;
    int spriteArmy=type&0xF0000;
    if (!spriteArmy)
        spriteArmy=0xF0000;

    int spriteType;
    int vidIndex=type&0x0FFF;
    const int vidQuery=(vidIndex!=0 && (type&0x1000)==0);
    if (vidQuery) {
        // ZS1 0x004182BE..0x0041835E is the header-visible MAP::Vid source
        // shape: each source use calls ValidateVid and selects m_vids[n] or
        // EmptyVid.  No VID::NoSprites/PropHash callable owner is used here.
        VID* queryVid=ValidateVid(vidIndex) ? m_vids[vidIndex] : EmptyVid;
        if (queryVid->m_entitiesNumber[0]+queryVid->m_entitiesNumber[1]+
            queryVid->m_entitiesNumber[2]+queryVid->m_entitiesNumber[3]==0)
            return 0;

        queryVid=ValidateVid(vidIndex) ? m_vids[vidIndex] : EmptyVid;
        if ((queryVid->m_flag&0x40u)!=0)
            type|=0x8000;

        vidIndex=type&0x0FFF;
        queryVid=ValidateVid(vidIndex) ? m_vids[vidIndex] : EmptyVid;
        spriteType=static_cast<int>(queryVid->m_unknown0C);
    } else {
        spriteType=(type>>20)&0x67F;
        if (!spriteType)
            spriteType=0x67F;
    }

    SPRITE* found=0;

    if (type&0x8000) {
        for (SPRITE* sprite=Hash->FirstInBox(screenX-radius,
                                              screenY-radius,
                                              screenX+radius,
                                              GetGroundZ(screenX,screenY)+screenY+radius);
             sprite;
             sprite=Hash->NextInBox()) {
            VID* spriteVid=sprite->m_vid;
            if (sprite->m_parent ||
                !(spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) ||
                !((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) ||
                ((type&static_cast<int>(0x80000000u)) && (sprite->m_flag&0x7Cu)) ||
                ((type&0x1000) && spriteVid->m_spriteClass!=static_cast<unsigned int>(type&0x7FF)) ||
                ((type&0x0FFF) && spriteVid->m_idx!=(type&0x0FFF)))
                continue;

            // SPRITE::IsInside is folded into all GetSpriteScr iterator paths
            // in retail; preserving it as a call removes hundreds of target
            // x87 instructions and changes the owner boundary.
            const float top=sprite->m_y-sprite->m_z;
            if (screenX<sprite->m_x-spriteVid->m_snapOffsetX ||
                screenX>sprite->m_x+spriteVid->m_snapOffsetX ||
                top-spriteVid->m_hitVerticalOffset-spriteVid->m_snapOffsetY>=screenY ||
                top+spriteVid->m_snapOffsetY<screenY)
                continue;

            if (!found ||
                spriteVid->m_footprintWidth<found->m_vid->m_footprintWidth ||
                spriteVid->m_footprintHeight<found->m_vid->m_footprintHeight)
                found=sprite;
        }
        return found;
    }

    if ((spriteType&(U_UNIT|U_AVIA)) &&
        !(spriteType&(U_TERRAIN|U_OBJECT|U_MENU|U_RAILWAY|U_REGION|U_CANNON|U_SPRITE))) {
        int index=0;
        for (SPRITE* sprite=Hash->units.BeginIterate(&index);sprite;sprite=Hash->units.NextIterate(&index)) {
            VID* spriteVid=sprite->m_vid;
            if (!(spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) ||
                !((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) ||
                ((type&static_cast<int>(0x80000000u)) && (sprite->m_flag&0x7Cu)) ||
                ((type&0x1000) && spriteVid->m_spriteClass!=static_cast<unsigned int>(type&0x7FF)) ||
                ((type&0x0FFF) && spriteVid->m_idx!=(type&0x0FFF)))
                continue;

            const float top=sprite->m_y-sprite->m_z;
            if (screenX<sprite->m_x-spriteVid->m_snapOffsetX ||
                screenX>sprite->m_x+spriteVid->m_snapOffsetX ||
                top-spriteVid->m_hitVerticalOffset-spriteVid->m_snapOffsetY>=screenY ||
                top+spriteVid->m_snapOffsetY<screenY)
                continue;

            if (!found ||
                spriteVid->m_footprintWidth<found->m_vid->m_footprintWidth ||
                spriteVid->m_footprintHeight<found->m_vid->m_footprintHeight)
                found=sprite;
        }
        return found;
    }

    if (vidQuery) {
        // Retail folds MAP::Vid for this classification test and then scans
        // MENU's inherited LIST backwards through m_no/m_data.
        VID* queryVid=(vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex])
            ? m_vids[vidIndex] : EmptyVid;
        if (queryVid->m_spriteClass==10u || queryVid->m_spriteClass==19u) {
            int index=m_menu.m_no-1;
            while (index>=0) {
                SPRITE* sprite=m_menu.m_data[index--];
                if (!sprite)
                    continue;
                VID* spriteVid=sprite->m_vid;
                if (sprite->m_parent ||
                    !(spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) ||
                    !((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) ||
                    ((type&static_cast<int>(0x80000000u)) && (sprite->m_flag&0x7Cu)) ||
                    ((type&0x1000) && spriteVid->m_spriteClass!=static_cast<unsigned int>(type&0x7FF)) ||
                    ((type&0x0FFF) && spriteVid->m_idx!=(type&0x0FFF)))
                    continue;

                const float top=sprite->m_y-sprite->m_z;
                if (screenX<sprite->m_x-spriteVid->m_snapOffsetX ||
                    screenX>sprite->m_x+spriteVid->m_snapOffsetX ||
                    top-spriteVid->m_hitVerticalOffset-spriteVid->m_snapOffsetY>=screenY ||
                    top+spriteVid->m_snapOffsetY<screenY)
                    continue;

                if (!found ||
                    spriteVid->m_footprintWidth<found->m_vid->m_footprintWidth ||
                    spriteVid->m_footprintHeight<found->m_vid->m_footprintHeight)
                    found=sprite;
            }
            return found;
        }
    }

    VID* beginVid=(vidQuery && vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex])
        ? m_vids[vidIndex] : EmptyVid;
    const int beginLayer=vidQuery ? beginVid->m_layer : 0;
    VID* endVid=(vidQuery && vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex])
        ? m_vids[vidIndex] : EmptyVid;
    const int endLayer=vidQuery ? endVid->m_layer+1 : 20;

    if (spriteType&U_REGION) {
        for (int layer=beginLayer;layer<endLayer;++layer) {
            int index;
            for (SPRITE* sprite=FirstSprite(layer,&index);sprite;sprite=NextSprite(layer,&index)) {
                VID* spriteVid=sprite->m_vid;
                if (sprite->m_parent ||
                    !(spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) ||
                    !((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) ||
                    ((type&static_cast<int>(0x80000000u)) && (sprite->m_flag&0x7Cu)) ||
                    ((type&0x1000) && spriteVid->m_spriteClass!=static_cast<unsigned int>(type&0x7FF)) ||
                    ((type&0x0FFF) && spriteVid->m_idx!=(type&0x0FFF)))
                    continue;

                int inside=0;
                if (spriteVid->m_spriteClass==23u) {
                    REGION* region=static_cast<REGION*>(sprite);
                    const float halfWidth=region->sizeX*0.5f;
                    const float halfHeight=region->sizeY*0.5f;
                    const float top=region->m_y-region->m_z;
                    inside=screenX>=region->m_x-halfWidth &&
                           screenX<=region->m_x+halfWidth &&
                           screenY>top-spriteVid->m_hitVerticalOffset-halfHeight &&
                           screenY<=top+halfHeight;
                } else {
                    const float top=sprite->m_y-sprite->m_z;
                    inside=screenX>=sprite->m_x-spriteVid->m_snapOffsetX &&
                           screenX<=sprite->m_x+spriteVid->m_snapOffsetX &&
                           screenY>top-spriteVid->m_hitVerticalOffset-spriteVid->m_snapOffsetY &&
                           screenY<=top+spriteVid->m_snapOffsetY;
                }
                if (inside && (!found ||
                    spriteVid->m_footprintWidth<found->m_vid->m_footprintWidth ||
                    spriteVid->m_footprintHeight<found->m_vid->m_footprintHeight))
                    found=sprite;
            }
        }
    } else {
        for (int layer=beginLayer;layer<endLayer;++layer) {
            int index;
            for (SPRITE* sprite=FirstSprite(layer,&index);sprite;sprite=NextSprite(layer,&index)) {
                VID* spriteVid=sprite->m_vid;
                if (sprite->m_parent ||
                    !(spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) ||
                    !((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) ||
                    ((type&static_cast<int>(0x80000000u)) && (sprite->m_flag&0x7Cu)) ||
                    ((type&0x1000) && spriteVid->m_spriteClass!=static_cast<unsigned int>(type&0x7FF)) ||
                    ((type&0x0FFF) && spriteVid->m_idx!=(type&0x0FFF)))
                    continue;

                const float top=sprite->m_y-sprite->m_z;
                if (screenX<sprite->m_x-spriteVid->m_snapOffsetX ||
                    screenX>sprite->m_x+spriteVid->m_snapOffsetX ||
                    top-spriteVid->m_hitVerticalOffset-spriteVid->m_snapOffsetY>=screenY ||
                    top+spriteVid->m_snapOffsetY<screenY)
                    continue;

                if (!found ||
                    spriteVid->m_footprintWidth<found->m_vid->m_footprintWidth ||
                    spriteVid->m_footprintHeight<found->m_vid->m_footprintHeight)
                    found=sprite;
            }
        }
    }
    return found;
}

// ZS1 retail MAP::FindNearestSprite owner.
SPRITE* MAP::FindNearestSprite(int type,float x,float y,float radius,SPRITE* prev)
{
    float findSize=radius;
    SPRITE* findSprite=0;
    int spriteArmy=type&0xF0000;
    const float prevSize=prev ? NearDistance(x-prev->m_x,y-prev->m_y) : -1.0f;
    if (!spriteArmy)
        spriteArmy=0xF0000;

    const int vidIndex=type&0x0FFF;
    const bool vidQuery=vidIndex!=0 && (type&0x1000)==0;
    int spriteType;
    if (vidQuery) {
        VID* queryVid=(vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex]) ? m_vids[vidIndex] : EmptyVid;
        const int noSprites=queryVid->m_entitiesNumber[0]+queryVid->m_entitiesNumber[1]+
                            queryVid->m_entitiesNumber[2]+queryVid->m_entitiesNumber[3];
        if (!noSprites)
            return 0;

        queryVid=(vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex]) ? m_vids[vidIndex] : EmptyVid;
        if (queryVid->m_flag&0x40u)
            type|=0x8000;

        queryVid=(vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex]) ? m_vids[vidIndex] : EmptyVid;
        spriteType=queryVid->m_unknown0C;
    } else {
        spriteType=(type&0x67F00000)>>20;
        if (!spriteType)
            spriteType=0x67F;
    }

    if (type&0x8000) {
        for (SPRITE* sprite=Hash->FirstInBox(x-radius,y-radius,x+radius,y+radius);
             sprite;
             sprite=Hash->NextInBox()) {
            VID* const spriteVid=sprite->m_vid;
            if (sprite->m_parent ||
                !(spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) ||
                !((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) ||
                ((type&static_cast<int>(0x80000000u)) && (sprite->m_flag&0x7Cu)) ||
                ((type&0x1000) && spriteVid->m_spriteClass!=static_cast<unsigned int>(type&0x7FF)) ||
                (vidIndex && spriteVid->m_idx!=vidIndex))
                continue;
            const float size=NearDistance(x-sprite->m_x,y-sprite->m_y);
            if (size<findSize && size>prevSize) {
                findSprite=sprite;
                findSize=size;
            }
        }
        return findSprite;
    }

    if ((spriteType&0x0C) && !(spriteType&0x673)) {
        int index;
        SPRITE* sprite=0;
        if (Hash->units.m_no) {
            index=Hash->units.m_no-1;
            sprite=Hash->units.m_data[index];
        }
        while (sprite) {
            VID* const spriteVid=sprite->m_vid;
            if (!sprite->m_parent &&
                (spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) &&
                ((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) &&
                (!(type&static_cast<int>(0x80000000u)) || !(sprite->m_flag&0x7Cu)) &&
                (!(type&0x1000) || spriteVid->m_spriteClass==static_cast<unsigned int>(type&0x7FF)) &&
                (!vidIndex || spriteVid->m_idx==vidIndex)) {
                float dx=fabsf(x-sprite->m_x);
                float dy=fabsf(y-sprite->m_y);
                const float size=dx>=dy ? dx+dy*0.5f : dy+dx*0.5f;
                if (size<findSize && size>prevSize) {
                    findSprite=sprite;
                    findSize=size;
                }
            }
            sprite=Hash->units.NextIterate(&index);
        }
        return findSprite;
    }

    if (vidQuery) {
        VID* queryVid=(vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex]) ? m_vids[vidIndex] : EmptyVid;
        if (queryVid->m_spriteClass==10u ||
            ((queryVid=(vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex]) ? m_vids[vidIndex] : EmptyVid),queryVid->m_spriteClass==19u)) {
            int index;
            SPRITE* sprite=0;
            if (m_menu.m_no) {
                index=m_menu.m_no-1;
                sprite=m_menu.m_data[index];
            }
            while (sprite) {
                VID* const spriteVid=sprite->m_vid;
                if (!sprite->m_parent &&
                    (spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) &&
                    ((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) &&
                    (!(type&static_cast<int>(0x80000000u)) || !(sprite->m_flag&0x7Cu)) &&
                    (!(type&0x1000) || spriteVid->m_spriteClass==static_cast<unsigned int>(type&0x7FF)) &&
                    (!vidIndex || spriteVid->m_idx==vidIndex)) {
                    float dx=fabsf(x-sprite->m_x);
                    float dy=fabsf(y-sprite->m_y);
                    const float size=dx>=dy ? dx+dy*0.5f : dy+dx*0.5f;
                    if (size<findSize && size>prevSize) {
                        findSprite=sprite;
                        findSize=size;
                    }
                }
                --index;
                sprite=index>=0 ? m_menu.m_data[index] : 0;
            }
            return findSprite;
        }
    }

    int beginLayer=0;
    int endLayer=20;
    if (vidQuery) {
        VID* queryVid=(vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex]) ? m_vids[vidIndex] : EmptyVid;
        beginLayer=queryVid->m_layer;
        queryVid=(vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex]) ? m_vids[vidIndex] : EmptyVid;
        endLayer=queryVid->m_layer+1;
    }

    for (int layer=beginLayer;layer<endLayer;++layer) {
        int index=m_layers[layer].m_no;
        for (SPRITE* sprite=NextSprite(layer,&index);sprite;sprite=NextSprite(layer,&index)) {
            VID* const spriteVid=sprite->m_vid;
            if (sprite->m_parent ||
                !(spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) ||
                !((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) ||
                ((type&static_cast<int>(0x80000000u)) && (sprite->m_flag&0x7Cu)) ||
                ((type&0x1000) && spriteVid->m_spriteClass!=static_cast<unsigned int>(type&0x7FF)) ||
                (vidIndex && spriteVid->m_idx!=vidIndex))
                continue;
            float dx=fabsf(x-sprite->m_x);
            float dy=fabsf(y-sprite->m_y);
            const float size=dx>=dy ? dx+dy*0.5f : dy+dx*0.5f;
            if (size<findSize && size>prevSize) {
                findSprite=sprite;
                findSize=size;
            }
        }
    }
    return findSprite;
}

// Legacy MapEdit owner: 0x0041BEA5..0x0041C266.
void MAP::FindSpritesInsidePolygon(int type,const POLYGON* polygon,SPRITE_LIST* list)
{
    int spriteArmy=type&0xF0000;
    if (!spriteArmy)
        spriteArmy=0xF0000;

    int spriteType;
    int vidIndex=type&0x0FFF;
    const int vidQuery=(vidIndex!=0 && (type&0x1000)==0);
    if (vidQuery) {
        VID* queryVid=ValidateVid(vidIndex) ? m_vids[vidIndex] : EmptyVid;
        if ((queryVid->m_flag&0x40u)!=0)
            type|=0x8000;

        vidIndex=type&0x0FFF;
        queryVid=ValidateVid(vidIndex) ? m_vids[vidIndex] : EmptyVid;
        spriteType=static_cast<int>(queryVid->m_unknown0C);
    } else {
        spriteType=(type>>20)&0x67F;
        if (!spriteType)
            spriteType=0x67F;
    }

    POLYGON* mutablePolygon=const_cast<POLYGON*>(polygon);
    if (type&0x8000) {
        for (SPRITE* sprite=Hash->FirstInBox(-100.0f,-100.0f,m_w+100.0f,m_h+100.0f);
             sprite;
             sprite=Hash->NextInBox()) {
            VID* spriteVid=sprite->m_vid;
            if (sprite->m_parent ||
                !(spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) ||
                !((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) ||
                ((type&static_cast<int>(0x80000000u)) && (sprite->m_flag&0x7Cu)) ||
                ((type&0x1000) && spriteVid->m_spriteClass!=static_cast<unsigned int>(type&0x7FF)) ||
                (vidIndex && spriteVid->m_idx!=vidIndex))
                continue;

            if (mutablePolygon->AskInside(sprite->m_x,sprite->m_y-sprite->m_z))
                list->InsertUnique(sprite);
        }
        return;
    }

    if ((spriteType&(U_UNIT|U_AVIA)) &&
        !(spriteType&(U_TERRAIN|U_OBJECT|U_MENU|U_RAILWAY|U_REGION|U_CANNON|U_SPRITE))) {
        int index=Hash->units.m_no-1;
        SPRITE* sprite=index>=0 ? Hash->units.m_data[index] : 0;
        while (sprite) {
            VID* spriteVid=sprite->m_vid;
            if (!sprite->m_parent &&
                (spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) &&
                ((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) &&
                (!(type&static_cast<int>(0x80000000u)) || !(sprite->m_flag&0x7Cu)) &&
                (!(type&0x1000) || spriteVid->m_spriteClass==static_cast<unsigned int>(type&0x7FF)) &&
                (!vidIndex || spriteVid->m_idx==vidIndex) &&
                mutablePolygon->AskInside(sprite->m_x,sprite->m_y-sprite->m_z))
                list->InsertUnique(sprite);

            sprite=Hash->units.NextIterate(&index);
        }
        return;
    }

    int beginLayer=0;
    int endLayer=20;
    if (vidQuery) {
        VID* beginVid=(vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex]) ? m_vids[vidIndex] : EmptyVid;
        beginLayer=beginVid->m_layer;
        VID* endVid=(vidIndex>=0 && vidIndex<m_noVid && m_vids[vidIndex]) ? m_vids[vidIndex] : EmptyVid;
        endLayer=endVid->m_layer+1;
    }

    for (int layer=beginLayer;layer<endLayer;++layer) {
        SPRITE_LIST* sprites=&m_layers[layer];
        int index=sprites->m_no-1;
        while (index>=0) {
            SPRITE* sprite=0;
            while (index>=0 && !sprite) {
                sprite=sprites->m_data[index];
                if (!sprite)
                    --index;
            }
            if (!sprite)
                break;

            VID* spriteVid=sprite->m_vid;
            if (!sprite->m_parent &&
                (spriteVid->m_unknown0C&static_cast<unsigned int>(spriteType)) &&
                ((0x10000u<<((sprite->m_flag>>12)&3u))&static_cast<unsigned int>(spriteArmy)) &&
                (!(type&static_cast<int>(0x80000000u)) || !(sprite->m_flag&0x7Cu)) &&
                (!(type&0x1000) || spriteVid->m_spriteClass==static_cast<unsigned int>(type&0x7FF)) &&
                (!vidIndex || spriteVid->m_idx==vidIndex) &&
                mutablePolygon->AskInside(sprite->m_x,sprite->m_y-sprite->m_z))
                list->InsertUnique(sprite);

            --index;
        }
    }
}


STRING MAP::GetMouseTipsString()
{
    STRING result("");
    if (!IsPaused())
        result=m_player[m_curArmy]->GetMouseTipsString();

    if (result.operator==("") && m_menu.SpriteUnderCursor()) {
        STRING key="MenuVid"+Int2Str(m_menu.NVidUnderCursor());
        STRING section("MouseTips");
        STRING defaultString("");
        STRING allDir=key+"AllDir";
        result=Profile->GetString(&section,&allDir,&defaultString);
        if (result.operator==("")) {
            STRING dirKey=key+"Dir"+Int2Str(m_menu.NDirUnderCursor());
            result=Profile->GetString(&section,&dirKey,&defaultString);
        }
    }
    return result;
}


