
#ifndef  _opengl_iface_
#define  _opengl_iface_

#include <GL/gl.h>

#include <initializer_list>
#include <array>
#include <string>
#include <iostream>

#include <Eigen/Core>


const bool gl_debug=true;


struct CDrawGuard
{
    static bool is_draw;

    explicit CDrawGuard(GLenum type)
    {
        assert(!is_draw);
        is_draw=true;
        glBegin(type);
    }
    ~CDrawGuard(){glEnd();is_draw=false;}
};
inline bool CDrawGuard::is_draw=false;


inline void _glCheckError_(const char *file, int line)
{
    if constexpr(gl_debug){
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    }
}
#define glCheckError() _glCheckError_(__FILE__, __LINE__)


#endif

