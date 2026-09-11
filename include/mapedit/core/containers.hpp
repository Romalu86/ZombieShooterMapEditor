#pragma once
// ACT/LIST/NAMED_LIST/UNDO owners. Included in ABI order by mapedit/runtime.hpp.
struct ACT {
    int act;
    int var1;
    int var2;
    int var3;
    ACT();
    ACT(const ACT* r);
    ACT(int a,int v1,int v2,int v3);
    int operator==(const ACT* r);
    int operator!=(const ACT* r);
};

template<class T> class LIST {
public:
    LIST();
    virtual ~LIST();
    int m_no;        // +0x04
    int m_max;       // +0x08
    T*  m_data;      // +0x0C
    int No();
    T* First();
    T* operator[](int index);
    const T* operator[](int index) const;
    int Location(T const* value);
    int ExistIn(T const* value);
    int InsertUnique(T const* value);
    int Delete(T const* value);
    T* Item(int index);
    void Release();
    void Insert(T item);
    void Push(const T* item);
    T* Pop();
    T* Last();
    void SetNo(int newNo);
    void InsertFirst(T item);
    void InsertBefore(int n,T item);
    int DeleteNumber(int index);
    void DeleteNumberS(int index);
    void ShiftUp(int index);
    void ShiftDown(int index);
    void ExpandForInsert();
    void GetReleased(LIST<T>* source);
    void Expand(int newAllocation);
    void DeleteFrom(int n);
    T BeginIterate(int* index);
    T NextIterate(int* index);
    void Write(STREAM* file);
    void Read(STREAM* file);
    void OldRead(STREAM* file);
    const LIST<T>* operator=(const LIST<T>* r);
};

// ZS1 mylib.h template source-shape: LIST<int> growth/insert is header-visible.
// SetTextItems at target 0x004558E0 expands this path in-place under VC6 /Ob1.
template<> inline void LIST<int>::Expand(int newAllocation)
{
    if (newAllocation <= m_max)
        return;
    int* oldData=m_data;
    int* newData=static_cast<int*>(::operator new(static_cast<unsigned int>(newAllocation)*sizeof(int)));
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

template<> inline void LIST<int>::ExpandForInsert()
{
    if (m_no >= m_max)
        Expand(m_max*2 + 4);
}

template<> inline void LIST<int>::Insert(int item)
{
    ExpandForInsert();
    m_data[m_no++] = item;
}

struct INIVAR {
    uint16_t type;
    uint16_t noelem;
    INIVAR();
    INIVAR(int typ,int no);
};

struct CONSTVAR {
    STRING varstr;
    int var;
    CONSTVAR();
    CONSTVAR(const STRING* str);
    ~CONSTVAR();
    CONSTVAR* operator=(const CONSTVAR* that);
};

template<class T> struct NAMED_LIST_STRUCT {
    STRING name;
    T val;
    ~NAMED_LIST_STRUCT();
};

template<class T> class NAMED_LIST : public LIST<NAMED_LIST_STRUCT<T> > {
public:
    NAMED_LIST();
    virtual ~NAMED_LIST();
    STRING* Name(int index);
    T* operator[](int index);
    T* Last();
    int Location(const STRING* name);
    void Insert(STRING name,T item);
    void Write(STREAM* file);
    void Read(STREAM* file);
};

class SPRITE_LIST : public LIST<SPRITE*> {
public:
    SPRITE_LIST();
    ~SPRITE_LIST();
    void Insert(SPRITE* spr);
    void InsertFirst(SPRITE* spr);
    void InsertBefore(int n,SPRITE* spr);
    int InsertUnique(SPRITE* spr);
    int Delete(SPRITE* spr);
    int DeleteSpriteNumber(int index);
    void DeleteAll();
    void Release();
    int IsEqual(const SPRITE_LIST* r);
};

class UNDO {
public:
    struct UNDO_DATA {
        enum TYPE_CREATE { INSERTED=0, DELETED=1 };
        TYPE_CREATE type;
        SPRITE* sprite;
        UNDO_DATA();
        UNDO_DATA(TYPE_CREATE createType, SPRITE* spr);
        void Undo();
        void DeleteSprite();
    };

    int noUndo;
    LIST<UNDO_DATA> sprites[5];

    UNDO();
    ~UNDO();
    void ReleaseUndo(int index);
    void Begin();
    void End();
    void AddInsert(SPRITE* sprite);
    void AddRemove(SPRITE* sprite);
    void DeleteLast();
    void Reset();
    int IsUndo();
    int IsRedo();
    void Undo();
    void Redo();
};

