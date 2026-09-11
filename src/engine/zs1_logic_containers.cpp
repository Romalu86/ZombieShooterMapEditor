#include "mapedit/runtime.hpp"

namespace {
void LogicContainerOutOfMemory(int count)
{
    MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",count);
}
}

// destroys both STRING-bearing members and follows the retail delete-cookie flags.
template<> NAMED_LIST_STRUCT<LOGICVAR>::~NAMED_LIST_STRUCT() {}
// of each 8-byte named STRING entry and follows the VC6 array-cookie/delete flags.
// STRING objects without the vector-delete wrapper path.
template<> NAMED_LIST_STRUCT<STRING>::~NAMED_LIST_STRUCT() {}

// ZS1 active LOGIC concrete template owners.  The original A53 logic.cpp used
// to carry these specializations; PORT5 intentionally removed that TU from the
// project when the ZS1 VM replaced it.  Keep the owners in a dedicated TU so
// MSVC emits the exact LIST/NAMED_LIST symbols required by zs1_logic_bridge.obj
// without reintroducing the old A53 parser/runtime implementation.

template<> LIST<LOGICSTACK>::LIST() : m_no(0),m_max(0),m_data(0) {}

template<> void LIST<LOGICSTACK>::Release()
{
    m_max=0;
    m_no=0;
    if (m_data)
        delete[] m_data;
    m_data=0;
}

// Compiler-generated scalar deleting destructor wraps the body below.
template<> LIST<LOGICSTACK>::~LIST()
{
    // Retail destructor intentionally leaves m_max unchanged.
    if (m_data)
        delete[] m_data;
    m_data=0;
    m_no=0;
}

// LIST<LOGICSTACK>::Expand is header-inline in script/logic.hpp.
// Retail still emits its standalone COMDAT when large callers decline to inline it.

template<> void LIST<LOGICSTACK>::Insert(LOGICSTACK item)
{
    ExpandForInsert();
    m_data[m_no++]=item;
}


// Base storage owner used by the retail NAMED_LIST_LOGICVAR family.
template<> LIST<NAMED_LIST_STRUCT<LOGICVAR> >::LIST() : m_no(0),m_max(0),m_data(0) {}

template<> void LIST<NAMED_LIST_STRUCT<LOGICVAR> >::Release()
{
    if (m_data)
        delete[] m_data;
    m_data=0;
    m_no=0;
    m_max=0;
}

template<> LIST<NAMED_LIST_STRUCT<LOGICVAR> >::~LIST()
{
    // Retail base destructor intentionally leaves m_max unchanged.
    if (m_data)
        delete[] m_data;
    m_data=0;
    m_no=0;
}
template<> int LIST<NAMED_LIST_STRUCT<LOGICVAR> >::No() { return m_no; }
template<> NAMED_LIST_STRUCT<LOGICVAR>* LIST<NAMED_LIST_STRUCT<LOGICVAR> >::operator[](int index) { return m_data+index; }
template<> const NAMED_LIST_STRUCT<LOGICVAR>* LIST<NAMED_LIST_STRUCT<LOGICVAR> >::operator[](int index) const { return m_data+index; }

// copies name/value entries, destroys old storage and updates the capacity field.
template<> void LIST<NAMED_LIST_STRUCT<LOGICVAR> >::Expand(int newAllocation)
{
    if (newAllocation<=m_max)
        return;
    typedef NAMED_LIST_STRUCT<LOGICVAR> ITEM;
    ITEM* fresh=new ITEM[newAllocation];
    if (!fresh) {
        LogicContainerOutOfMemory(newAllocation);
        return;
    }
    if (m_data) {
        for (int i=0;i<m_max;++i) {
            fresh[i].name=m_data[i].name;
            fresh[i].val=m_data[i].val;
        }
        delete[] m_data;
    }
    m_data=fresh;
    m_max=newAllocation;
}

template<> void LIST<NAMED_LIST_STRUCT<LOGICVAR> >::ExpandForInsert()
{
    if (m_no>=m_max)
        Expand(m_max*2+4);
}

template<> void LIST<NAMED_LIST_STRUCT<LOGICVAR> >::DeleteFrom(int n)
{
    if (n<=0) {
        Release();
        return;
    }
    if (n<m_no)
        m_no=n;
}


// Base storage owner used by the retail NAMED_LIST_STRING family.
template<> LIST<NAMED_LIST_STRUCT<STRING> >::LIST() : m_no(0),m_max(0),m_data(0) {}

template<> void LIST<NAMED_LIST_STRUCT<STRING> >::Release()
{
    if (m_data)
        delete[] m_data;
    m_data=0;
    m_no=0;
    m_max=0;
}

template<> LIST<NAMED_LIST_STRUCT<STRING> >::~LIST()
{
    // Retail base destructor intentionally leaves m_max unchanged.
    if (m_data)
        delete[] m_data;
    m_data=0;
    m_no=0;
}
template<> int LIST<NAMED_LIST_STRUCT<STRING> >::No() { return m_no; }
template<> NAMED_LIST_STRUCT<STRING>* LIST<NAMED_LIST_STRUCT<STRING> >::operator[](int index) { return m_data+index; }
template<> const NAMED_LIST_STRUCT<STRING>* LIST<NAMED_LIST_STRUCT<STRING> >::operator[](int index) const { return m_data+index; }

template<> void LIST<NAMED_LIST_STRUCT<STRING> >::Expand(int newAllocation)
{
    if (newAllocation<=m_max)
        return;
    typedef NAMED_LIST_STRUCT<STRING> ITEM;
    ITEM* fresh=new ITEM[newAllocation];
    if (!fresh) {
        LogicContainerOutOfMemory(newAllocation);
        return;
    }
    if (m_data) {
        for (int i=0;i<m_max;++i) {
            fresh[i].name=m_data[i].name;
            fresh[i].val=m_data[i].val;
        }
        delete[] m_data;
    }
    m_data=fresh;
    m_max=newAllocation;
}

template<> void LIST<NAMED_LIST_STRUCT<STRING> >::ExpandForInsert()
{
    if (m_no>=m_max)
        Expand(m_max*2+4);
}

template<> void LIST<NAMED_LIST_STRUCT<STRING> >::DeleteNumberS(int n)
{
    if (n>=0 && n<m_no) {
        --m_no;
        while (n<m_no) {
            m_data[n]=m_data[n+1];
            ++n;
        }
    }
    if (m_no==0)
        Release();
}


template<> NAMED_LIST<LOGICVAR>::NAMED_LIST() {}
// Compiler-generated family deleting destructor; derived body is empty and the
// directly proven 0x0041C360 base destructor performs storage teardown.
template<> NAMED_LIST<LOGICVAR>::~NAMED_LIST() {}
template<> STRING* NAMED_LIST<LOGICVAR>::Name(int index) { return &this->m_data[index].name; }
template<> LOGICVAR* NAMED_LIST<LOGICVAR>::Last() { return &this->m_data[this->m_no-1].val; }
template<> LOGICVAR* NAMED_LIST<LOGICVAR>::operator[](int index) { return &this->m_data[index].val; }

template<> void NAMED_LIST<LOGICVAR>::Insert(STRING name,LOGICVAR item)
{
    this->ExpandForInsert();
    this->m_data[this->m_no].name=name;
    this->m_data[this->m_no].val=item;
    ++this->m_no;
}

template<> void NAMED_LIST<LOGICVAR>::Write(STREAM* file)
{
    if (!file)
        return;
    file->Write(&this->m_no,4u);
    for (int i=0;i<this->m_no;++i) {
        this->m_data[i].name.Write(file);
        file->Write(&this->m_data[i].val,0x14u);
    }
}

template<> void NAMED_LIST<LOGICVAR>::Read(STREAM* file)
{
    if (!file)
        return;
    file->Read(&this->m_no,4u);
    this->Expand(this->m_no);
    for (int i=0;i<this->m_no;++i) {
        this->m_data[i].name.Read(file);
        file->Read(&this->m_data[i].val,0x14u);
    }
}


template<> NAMED_LIST<STRING>::NAMED_LIST() {}
// Compiler-generated family deleting destructor; derived body is empty and the
// directly proven 0x0041C3D0 base destructor performs storage teardown.
template<> NAMED_LIST<STRING>::~NAMED_LIST() {}
template<> STRING* NAMED_LIST<STRING>::Name(int index) { return &this->m_data[index].name; }
template<> STRING* NAMED_LIST<STRING>::operator[](int index) { return &this->m_data[index].val; }

template<> void NAMED_LIST<STRING>::Insert(STRING name,STRING item)
{
    this->ExpandForInsert();
    this->m_data[this->m_no].name=name;
    this->m_data[this->m_no].val=item;
    ++this->m_no;
}
