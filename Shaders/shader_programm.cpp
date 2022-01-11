#include <assert.h>
#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>

#include "shader_programm.h"

////////////////////////////////////////////////
//                  CShader
////////////////////////////////////////////////


CShader& CShader::operator=(CShader&&other)
{
    if(this==&other) return *this;
    if(m_handler!=0)
    {
        glDeleteShader(m_handler);
        glCheckError();
    }
    m_handler=other.m_handler;
    other.m_handler=0;
    return *this;
}

CShader CShader::Compile(type_t t,const char*str,CShaderBuildError&error)
{
    GLenum shader_type;
    switch(t)
    {
        case compute:        shader_type=GL_COMPUTE_SHADER;        break;
        case vertex:         shader_type=GL_VERTEX_SHADER;         break;
        case tess_control:   shader_type=GL_TESS_CONTROL_SHADER;   break;
        case tess_evaluation:shader_type=GL_TESS_EVALUATION_SHADER;break;
        case geometry:       shader_type=GL_GEOMETRY_SHADER;       break;
        case fragment:       shader_type=GL_FRAGMENT_SHADER;       break;

        default:assert(false);
    }
    int handler = glCreateShader(shader_type);
    glCheckError();
    glShaderSource(handler,1,&str,NULL);
    glCheckError();
    glCompileShader(handler);
    glCheckError();

    GLint valid;
    glGetShaderiv(handler, GL_COMPILE_STATUS, &valid);
    glCheckError();
    if(valid!=GL_TRUE)
    {
        GLint log_size;
        glGetShaderiv(handler,GL_INFO_LOG_LENGTH,&log_size);
        glCheckError();
        std::string log;log.resize(log_size);
        glGetShaderInfoLog(handler,log.size(),NULL,log.data());
        glCheckError();
        error.m_status=CShaderBuildError::compile_error;
        error.m_log=std::move(log);
        handler=0;
    }
    else
    {
        error.m_SetNoError();
    }
    return CShader(handler);
}

CShader CShader::Compile(type_t t,const char*str)
{
    CShaderBuildError error;
    auto result=Compile(t,str,error);
    if(error) throw CShaderBuildException(error);
    return result;
}

CShader::~CShader()
{
    if(m_handler!=0)
    {
        glDeleteShader(m_handler);
        glCheckError();
    }
}

////////////////////////////////////////////////
//                  CShaderProgramm
////////////////////////////////////////////////

const CShaderProgramm* CShaderProgramm::m_current=nullptr;

GLuint CShaderProgramm::m_glCreateProgram()
{
    auto h=glCreateProgram();
    glCheckError();
    return h;
}

void CShaderProgramm::m_glAttachShader(GLuint shader_handler)
{
  glAttachShader(m_handler,shader_handler);
  glCheckError();
}

void CShaderProgramm::m_glDetachShader(GLuint shader_handler)
{
  glDetachShader(m_handler,shader_handler);
  glCheckError();
}

void CShaderProgramm::m_glLinkProgram( )
{
  glLinkProgram(m_handler);
  glCheckError();
}

GLint CShaderProgramm::m_glGetUniformLocation(const GLchar*str)const
{
  auto handler=glGetUniformLocation(m_handler,str);
  glCheckError();
  return handler;
}


CShaderProgramm& CShaderProgramm::operator=(CShaderProgramm&&other)
{
    if(this==&other) return *this;
    if(m_handler!=0)
    {
        glDeleteProgram(m_handler);
        glCheckError();
    }
    m_handler=other.m_handler;
    other.m_handler=0;
    return *this;
}


void CShaderProgramm::m_CheckError(CShaderBuildError&error)
{
    GLint valid;
    glGetProgramiv(m_handler,GL_LINK_STATUS,&valid);
    glCheckError();
    if(valid!=GL_TRUE)
    {
        GLint log_size;
        glGetProgramiv(m_handler,GL_INFO_LOG_LENGTH,&log_size);
        glCheckError();
        std::string log;log.resize(log_size);
        glGetProgramInfoLog(m_handler,log.size(),NULL,log.data());
        glCheckError();
        error.m_status=CShaderBuildError::link_error;
        error.m_log=std::move(log);
        m_handler=0;
    }
    else
    {
        error.m_SetNoError();
    }
}

void CShaderProgramm::Use()const
{
    if(m_handler)
    {
        glUseProgram(m_handler);
        glCheckError();
        m_current=this;
    }
}

CShaderProgramm::~CShaderProgramm()
{
    if(m_handler)
    {
        glDeleteProgram(m_handler);
        glCheckError();
    }
}
