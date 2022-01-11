#include <assert.h>
#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>

#include "vao_managment.h"

////////////////////////////////////////////////
//                  CBufferFormat
////////////////////////////////////////////////

bool CBufferFormat::operator==(const CBufferFormat&other)const
{
    return m_normalize==other.m_normalize&&
           m_start_ofset==other.m_normalize&&
           m_block_size==other.m_block_size&&
           m_step==other.m_step;
}

CBufferFormat&CBufferFormat::SetStartOffset(size_t offset)
{
    m_start_ofset=offset;
    return *this;
}

CBufferFormat&CBufferFormat::SetBlockSize(size_t bsize)
{
    m_block_size=bsize;
    return *this;
}

CBufferFormat&CBufferFormat::SetStep(size_t step)
{
    m_step=step;
    return *this;
}

CBufferFormat&CBufferFormat::SetNormalize(size_t norm)
{
    m_normalize=norm;
    return *this;
}

////////////////////////////////////////////////
//                  CBaseBufferImpl
////////////////////////////////////////////////

CBaseBufferImpl::CBaseBufferImpl(GLenum t,size_t st):
m_buffer_type(t),m_sizeof_type(st),m_num_elements(0)
{
    glGenBuffers(1,&m_handler);
    glCheckError();
}

CBaseBufferImpl&CBaseBufferImpl::operator=(CBaseBufferImpl&&other)
{
    if(this==&other) return *this;
    if(m_sizeof_type!=0) glDeleteBuffers(1, &m_handler);
    m_CopyData(other);
    other.m_sizeof_type=0;
    return *this;
}

void CBaseBufferImpl::m_CopyData(const CBaseBufferImpl&other)
{
    m_type_enum=other.m_type_enum;
    m_sizeof_type=other.m_sizeof_type;
    m_num_elements=other.m_num_elements;
    m_handler=other.m_handler;
}

void CBaseBufferImpl::m_Write(size_t size_of_el,size_t size_of_stor,const void*ptr,usage_t usage)
{
    assert(size_of_el>0);
    glBindBuffer(m_buffer_type, m_handler);
    glCheckError();
    if(m_sizeof_type*m_num_elements<size_of_el*size_of_stor)
    {
        glBufferData(m_buffer_type,size_of_el*size_of_stor,ptr,usage);
        glCheckError();
    }
    else
    {
        glBufferSubData(m_buffer_type,0,size_of_el*size_of_stor,ptr);
        glCheckError();
    }

    m_sizeof_type=size_of_el;
    m_num_elements=size_of_stor;
    glBindBuffer(m_buffer_type, 0);
    glCheckError();
}

std::size_t CBaseBufferImpl::m_Read(void*ptr,size_t bytes)const
{
    size_t byte_for_read=std::min(bytes,m_sizeof_type*m_num_elements);
    if(!byte_for_read) return 0;
    glGetBufferSubData(m_buffer_type,0,byte_for_read,ptr);
    glCheckError();
    return byte_for_read;
}

void CBaseBufferImpl::m_Swap(CBaseBufferImpl&other)
{
    assert(m_buffer_type==other.m_buffer_type);
    std::swap(m_sizeof_type,other.m_sizeof_type);
    std::swap(m_num_elements,other.m_num_elements);
    std::swap(m_handler,other.m_handler);
    std::swap(m_type_enum,other.m_type_enum);
}

CBaseBufferImpl::~CBaseBufferImpl()
{
    if(m_sizeof_type!=0)
    {
        glDeleteBuffers(1, &m_handler);
        glCheckError();
    }
}


////////////////////////////////////////////////
//                  CVao
////////////////////////////////////////////////


CVao::CVao()
{
    glGenVertexArrays(1, &m_handler);
    glCheckError();
}

CVao& CVao::operator=(CVao&&other)
{
    if(this==&other) return *this;
    if(!m_moved)
    {
        glDeleteVertexArrays(1, &m_handler);
        glCheckError();
    }
    m_handler=other.m_handler;
    other.m_moved=true;
    return *this;
}

const CVao& CVao::EnableLayout(GLuint layout_index,CBuffer&buff,const CBufferFormat&format)const
{
    assert(buff.Valid());
    glBindVertexArray(m_handler);
    glCheckError();
    glBindBuffer(GL_ARRAY_BUFFER, buff.Handler());
    glCheckError();
    glVertexAttribPointer(layout_index,
                          format.BlockSize(),
                          buff.TypeEnum(),
                          format.Normalize(),
                          format.Step()*buff.SizeOfType(),
                          (void*)(format.StartOfset()*buff.SizeOfType()));
    glCheckError();
    glEnableVertexAttribArray(layout_index);
    glCheckError();

    glBindBuffer(GL_ARRAY_BUFFER,0);
    glCheckError();
    glBindVertexArray(0);
    glCheckError();
    return *this;
}


void CVao::DrawArraw(mode_t mode,size_t beg,size_t count)const
{
    glBindVertexArray(m_handler);
    glCheckError();
    glDrawArrays(mode,beg,count);
    glCheckError();
    glBindVertexArray(0);
    glCheckError();
}

void CVao::DrawElement(const CIndexBuffer&buff,mode_t mode,size_t count)const
{
    assert(buff.Valid());

    glBindVertexArray(m_handler);
    glCheckError();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buff.Handler());
    glCheckError();

    glDrawElements(mode,count,buff.TypeEnum(),0);
    glCheckError();

    glBindVertexArray(0);
    glCheckError();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glCheckError();
}

void CVao::DrawElement(const CIndexBuffer&buff,mode_t mode)const
{
    DrawElement(buff,mode,buff.Size());
    glCheckError();
}

void CVao::Swap(CVao&other)
{
    std::swap(m_moved,other.m_moved);
    std::swap(m_handler,other.m_handler);
}

CVao::~CVao()
{
    if(!m_moved)
    {
        glDeleteVertexArrays(1, &m_handler);
        glCheckError();
    }
}







