

#include <type_traits>
#include <assert.h>

#include <GL/glew.h>

#include "uniform_value.h"

#define define_uniform(short_type,full_type)\
void uniform_value_base::m_glUniform1##short_type (full_type value)\
{\
    glUniform1##short_type(m_handler,value);\
}\
void uniform_value_base::m_glUniform2##short_type (full_type _1,full_type _2)\
{\
    glUniform2##short_type (m_handler,_1,_2);\
}\
void uniform_value_base::m_glUniform3##short_type (full_type _1,full_type _2,full_type _3)\
{\
    glUniform3##short_type (m_handler,_1,_2,_3);\
}\
void uniform_value_base::m_glUniform4##short_type (full_type _1,full_type _2,full_type _3,full_type _4)\
{\
    glUniform4##short_type(m_handler,_1,_2,_3,_4);\
}\
void  uniform_value_base::m_glUniform2##short_type##v (GLsizei count,const full_type* data)\
{\
    glUniform2##short_type##v(m_handler,count,data);\
}\
void  uniform_value_base::m_glUniform3##short_type##v (GLsizei count,const full_type* data)\
{\
    glUniform3##short_type##v(m_handler,count,data);\
}\
void  uniform_value_base::m_glUniform4##short_type##v (GLsizei count,const full_type* data)\
{\
    glUniform4##short_type##v(m_handler,count,data);\
}

define_uniform(ui,GLuint)
define_uniform(i,GLint)
define_uniform(f,GLfloat)
define_uniform(d,GLdouble)

#undef define_uniform


void uniform_value_base::m_glUniformMatrix2fv(const GLfloat *data)
{
    glUniformMatrix2fv(m_handler,1,GL_FALSE,data);
}

void uniform_value_base::m_glUniformMatrix3fv(const GLfloat *data)
{
    glUniformMatrix3fv(m_handler,1,GL_FALSE,data);
}

void uniform_value_base::m_glUniformMatrix4fv(const GLfloat *data)
{
    glUniformMatrix4fv(m_handler,1,GL_FALSE,data);
}








