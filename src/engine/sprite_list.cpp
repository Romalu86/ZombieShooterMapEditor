#include "mapedit/runtime.hpp"

// Retail LIST_SPRITE constructor: non-owning 0x10-byte pointer list, vptr family 0x004923B4.
template<> LIST<SPRITE*>::LIST() : m_no(0), m_max(0), m_data(0) {}

// Compiler-generated scalar deleting destructor around the directly proven body below.
// Retail LIST_SPRITE destructor: frees pointer storage only; stored SPRITE references are not released.
template<> LIST<SPRITE*>::~LIST()
{
    if (m_data)
        operator delete(m_data);
    m_data = 0;
    m_no = 0;
}

// Retail owning layer-list constructor uses the distinct SPRITE_LIST vptr family 0x004923B0.
SPRITE_LIST::SPRITE_LIST() : LIST<SPRITE*>() {}

// VC6 vector deleting destructor owner.  flags&2 handles an array cookie and
// destroys each 0x10-byte SPRITE_LIST; scalar path destroys one object and
// flags&1 controls operator delete.  The semantic destructor itself remains empty.
SPRITE_LIST::~SPRITE_LIST() {}

// Explicit LIST<SPRITE*> specializations must precede every specialization that calls them.
template<> void LIST<SPRITE*>::ExpandForInsert()
{
    if (m_no >= m_max)
        Expand(m_max * 2 + 4);
}

template<> int LIST<SPRITE*>::DeleteNumber(int index)
{
    if (index < 0 || index >= m_no)
        return 1;
    --m_no;
    m_data[index] = m_data[m_no];
    return 0;
}

template<> void LIST<SPRITE*>::Insert(SPRITE* item)
{
    ExpandForInsert();
    m_data[m_no] = item;
    ++m_no;
}

template<> void LIST<SPRITE*>::InsertFirst(SPRITE* item)
{
    ExpandForInsert();
    int i=m_no;
    ++m_no;
    while (i!=0) {
        m_data[i]=m_data[i-1];
        --i;
    }
    m_data[0]=item;
}

template<> void LIST<SPRITE*>::InsertBefore(int n,SPRITE* item)
{
    ExpandForInsert();
    int i=m_no;
    ++m_no;
    while (i>n) {
        m_data[i]=m_data[i-1];
        --i;
    }
    m_data[n]=item;
}

template<> int LIST<SPRITE*>::InsertUnique(SPRITE* const* item)
{
    if (Location(item) < 0) {
        Insert(*item);
        return 0;
    }
    return 1;
}

template<> int LIST<SPRITE*>::Delete(SPRITE* const* item)
{
    return DeleteNumber(Location(item));
}

// Reference-counted sprite-list wrappers recovered from sprite_list.obj.
// The list owns one SPRITE reference for every stored entry, including duplicates.

void SPRITE_LIST::InsertFirst(SPRITE* sprite)
{
    if (!sprite)
        return;
    sprite->AddRef();
    LIST<SPRITE*>::InsertFirst(sprite);
}

void SPRITE_LIST::InsertBefore(int n,SPRITE* sprite)
{
    if (!sprite)
        return;
    sprite->AddRef();
    LIST<SPRITE*>::InsertBefore(n,sprite);
}

// Retail 0x0044C440..0x0044C4CA: AddRef and LIST growth/copy are folded
// into this owner; there is no LIST::Insert/ExpandForInsert helper boundary.
void SPRITE_LIST::Insert(SPRITE* sprite)
{
    if (!sprite)
        return;

    ++sprite->m_noRef;
    if (m_no >= m_max) {
        const int newAllocation=m_max*2+4;
        if (newAllocation>m_max) {
            SPRITE** oldData=m_data;
            SPRITE** newData=static_cast<SPRITE**>(::operator new(static_cast<unsigned int>(newAllocation)*sizeof(SPRITE*)));
            m_data=newData;
            if (!newData)
                MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",newAllocation);
            if (oldData) {
                for (int i=0;i<m_max;++i)
                    newData[i]=oldData[i];
                ::operator delete(oldData);
            }
            m_max=newAllocation;
        }
    }
    m_data[m_no++]=sprite;
}

// Retail 0x0044C4D0..0x0044C59F folds AddRef, reverse duplicate search,
// growth and insertion.  The reference increment happens before the duplicate
// test and is intentionally retained when the function returns 1.
int SPRITE_LIST::InsertUnique(SPRITE* sprite)
{
    if (!sprite)
        return 1;

    ++sprite->m_noRef;
    int index=m_no;
    while (index!=0) {
        --index;
        if (m_data[index]==sprite)
            return 1;
    }

    if (m_no >= m_max) {
        const int newAllocation=m_max*2+4;
        if (newAllocation>m_max) {
            SPRITE** oldData=m_data;
            SPRITE** newData=static_cast<SPRITE**>(::operator new(static_cast<unsigned int>(newAllocation)*sizeof(SPRITE*)));
            m_data=newData;
            if (!newData)
                MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",newAllocation);
            if (oldData) {
                for (int i=0;i<m_max;++i)
                    newData[i]=oldData[i];
                ::operator delete(oldData);
            }
            m_max=newAllocation;
        }
    }

    m_data[m_no++]=sprite;
    return 0;
}

// Retail 0x0044C5B0..0x0044C64B folds reverse Location, swap-delete and
// SPRITE::Release.  Keep the negative-ref error and virtual deleting-destructor
// path local to this owner.
int SPRITE_LIST::Delete(SPRITE* sprite)
{
    if (!sprite)
        return 1;

    int index=m_no;
    while (index!=0) {
        --index;
        if (m_data[index]==sprite)
            break;
    }
    if (index<0 || index>=m_no || m_data[index]!=sprite)
        return 1;

    --m_no;
    m_data[index]=m_data[m_no];

    --sprite->m_noRef;
    if (sprite->m_noRef>0)
        return 0;
    if (sprite->m_noRef<0) {
        const int vidIndex=sprite->m_vid ? sprite->m_vid->m_idx : -1;
        MYERROR::Error(::Error,"SPRITE %i",4,"noRef at Release",
                       static_cast<unsigned long>(sprite->m_noRef),vidIndex);
        return 0;
    }
    sprite->ScalarDeletingDestructor(1u);
    return 0;
}

int MENU::Delete(SPRITE* sprite)
{
    if (this->sprite==sprite)
        this->sprite=0;
    return SPRITE_LIST::Delete(sprite);
}

// Retail 0x0044C650..0x0044C6DD folds bounds-check, swap-delete and the
// reference decrement.  Its historical semantics deliberately invoke the
// virtual deleting destructor for every non-negative post-decrement count.
int SPRITE_LIST::DeleteSpriteNumber(int index)
{
    if (index<0 || index>=m_no)
        return 1;

    SPRITE* sprite=m_data[index];
    --m_no;
    m_data[index]=m_data[m_no];

    --sprite->m_noRef;
    if (sprite->m_noRef<0) {
        const int vidIndex=sprite->m_vid ? sprite->m_vid->m_idx : -1;
        MYERROR::Error(::Error,"SPRITE %i",4,"noRef at Release",
                       static_cast<unsigned long>(sprite->m_noRef),vidIndex);
        return 0;
    }
    if (sprite)
        sprite->ScalarDeletingDestructor(1u);
    return 0;
}

// Retail duplicate-collapse is locally expanded; the final distinct-entry
// destruction intentionally remains a call to the adjacent DeleteSpriteNumber
// owner, exactly as at 0x0044C78D.
void SPRITE_LIST::DeleteAll()
{
    for (int first=0;first<m_no;++first) {
        for (int scan=m_no-1;scan>first;--scan) {
            SPRITE* sprite=m_data[first];
            if (!sprite || sprite!=m_data[scan])
                continue;

            --sprite->m_noRef;
            if (sprite->m_noRef<0) {
                const int vidIndex=sprite->m_vid ? sprite->m_vid->m_idx : -1;
                MYERROR::Error(::Error,"SPRITE %i",4,"noRef at Release",
                               static_cast<unsigned long>(sprite->m_noRef),vidIndex);
            } else if (sprite->m_noRef==0) {
                sprite->ScalarDeletingDestructor(1u);
            }

            if (scan>=0 && scan<m_no) {
                --m_no;
                m_data[scan]=m_data[m_no];
            }
        }
    }

    for (int index=m_no-1;index>=0;--index) {
        if (m_data[index])
            DeleteSpriteNumber(index);
    }
}

// ZS1 retail release pass: remove list slot only while the sprite remains referenced.
void SPRITE_LIST::Release()
{
    // ZS1 0x0044C7A0..0x0044C819 folds LIST::No/operator[],
    // SPRITE::Release and LIST::DeleteNumber into one reverse owner.
    for (int index=m_no-1;index>=0;--index) {
        SPRITE* sprite=m_data[index];
        if (!sprite)
            continue;

        --sprite->m_noRef;
        if (sprite->m_noRef>0) {
            if (index>=0 && index<m_no) {
                --m_no;
                m_data[index]=m_data[m_no];
            }
            continue;
        }

        if (sprite->m_noRef<0) {
            const int vidIndex=sprite->m_vid ? sprite->m_vid->m_idx : -1;
            MYERROR::Error(::Error,"SPRITE %i",4,"noRef at Release",
                           static_cast<unsigned long>(sprite->m_noRef),vidIndex);
            continue;
        }

        sprite->ScalarDeletingDestructor(1u);
    }
}

// ZS1 retail SameMembers/IsEqual semantics: same count and membership ignoring order.
int SPRITE_LIST::IsEqual(const SPRITE_LIST* compare)
{
    SPRITE_LIST* mutableCompare = const_cast<SPRITE_LIST*>(compare);
    if (!mutableCompare || mutableCompare->No() != No())
        return 0;

    for (int mine=No()-1; mine>=0; --mine) {
        int other=mutableCompare->No()-1;
        for (; other>=0; --other) {
            if (*(*this)[mine] == *(*mutableCompare)[other])
                break;
        }
        if (other < 0)
            return 0;
    }
    return 1;
}
