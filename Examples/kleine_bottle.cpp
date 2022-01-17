
#include <iostream>
#include <fstream>
#include <vector>
#include <numbers>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>



#include "../functional_mesh.h"

#include "../view_ruling.h"
#include "../scene.h"

unsigned int glSceenWidth = 800;
unsigned int glScreenHeight = 800;;


void ResizeCallback(GLFWwindow* window, int width, int height);
void CharCallback ( GLFWwindow * win, unsigned int ch );
void MoouseMoveCallback ( GLFWwindow * win, double x, double y );
GLFWwindow* MakeWindow();


class CKleinBottle
{
    using point_t=Eigen::Vector3f;

    int   m_stage;
    float m_a = 2.5f;
    float m_b = 1.5f;

    public:
    CKleinBottle(int s,float a,float b):m_stage(s),m_a(a),m_b(b){}

    point_t operator()(float u,float v)const
    {
        float x,y,z;
        switch(m_stage)
        {
            case 0:
            {
                float cos_u=std::cos(u);
                x = (m_a + m_b*cos_u)*std::cos(v);
                y = (m_a + m_b*cos_u)*std::sin(v);
                z = -m_a*std::sin(u);
            }
            break;
            case 1:
            {
                float cos_u=std::cos(u);
                x = (m_a + m_b*cos_u)*std::cos(v);
                y = (m_a + m_b*cos_u)*std::sin(v);
                z = 3*u;
            }
            break;
            case 2:
            {
                x = 2-2*std::cos(u)+std::sin(v);
                y = std::cos(v);
                z = 3*u;;
            }
            break;
            case 3:
            {
                x = 2 + (2 + std::cos(v))*std::cos(u);
                y = std::sin(v);
                z = 3*std::numbers::pi_v<float> + (2 + std::cos(v))*std::sin(u);
            }
        }
        return point_t(x,y,z);
    }
};


CScene*gl_scene_ptr=nullptr;

int main()
{
    const float fpi=std::numbers::pi_v<float>;
    auto window=MakeWindow();
    CScene glScene;
    gl_scene_ptr=&glScene;

    CRenderingTraits rt;
    CFunctionalMesh mesh[4];

    rt.SetMesh(false).SetSpecular(true).SetSurface(true).SetTwoSideSpecular(true);
    for(int i=0;i<4;++i)
    {
        mesh[i].SetMeshFunctor(CKleinBottle(i,2.5f,1.5f))
               .SetResolution(60,60)
               .SetTransparency(0.35)
               .SetTraits(rt)
               .SetUniformColor(0.5,0.5,1)
               .SetRange({0,fpi},{0,2*fpi});
        glScene.AddMesh(mesh[i]);

    }
    glScene.LightSource().SetPosition(8,8,30);


    glClearColor(0.0f, 0.0, 0.0, 1.0f);
    while(!glfwWindowShouldClose(window))
    {
        glScene.Render(glfwGetTime());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

//Callbacks

void ResizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    glSceenWidth=width;
    glScreenHeight=height;
}

void CharCallback ( GLFWwindow * win, unsigned int ch )
{
    const float speed=0.2;
    auto&cam=gl_scene_ptr->Camera();
    switch(ch)
    {
        case 'w':
        case 'W':
        cam.IncreasePosition(cam.GetViewOrt()*speed);
        break;

        case 's':
        case 'S':
        cam.IncreasePosition(-cam.GetViewOrt()*speed);
        break;

        case 'a':
        case 'A':
        cam.IncreasePosition(-cam.GetRightOrt()*speed);
        break;

        case 'd':
        case 'D':
        cam.IncreasePosition(cam.GetRightOrt()*speed);
        break;

        case ' ':
        glfwSetWindowShouldClose(win,1);
        break;

        default:break;
    }
}

void MoouseMoveCallback( GLFWwindow * win, double x, double y )
{
    const double center_x=glSceenWidth/2;
    const double center_y=glScreenHeight/2;
    double dx=x-center_x;
    double dy=center_y-y;
    double mod=sqrt(dx*dx+dy*dy);
    if(mod==0) return;
    dx/=mod*30;
    dy/=mod*30;
    gl_scene_ptr->Camera().IncreaseView(-dy,-dx);
    glfwSetCursorPos(win,center_x,center_y);
}


GLFWwindow* MakeWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(glSceenWidth,glScreenHeight,"DCEL mesh",NULL,NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window,ResizeCallback);
    glfwSetCharCallback(window,CharCallback);
    glfwSetCursorPosCallback(window,MoouseMoveCallback);

    //glewExperimental = GL_TRUE;
    glewInit ();
    glGetError ();
    glCheckError();
    return window;
}





















