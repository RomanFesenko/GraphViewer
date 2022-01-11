
#include <iostream>
#include <vector>
#include <numbers>
#include <cmath>


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

void MakePlainWave(CFunctionalMesh&mesh)
{
    using point_t=Eigen::Vector3f;
    using namespace std::numbers;
    CRenderingTraits rt;
    rt.SetMesh(false).SetSpecular(true).SetColored(false).SetSurface(true).SetTwoSideSpecular(true);
    auto make_wave=[](const point_t&k,float omega)
    {
        return plot::cartesian([k,omega](float x,float y,float t)
        {
            return cos(k[0]*x+k[1]*y-omega*t);
        });
    };
    mesh.SetMeshFunctor(make_wave({1,1,0},2*pi_v<float>/2.0))
        .SetResolution(70,70)
        .SetTraits(rt)
        .SetRange({-10,10},{-10,10});
}

void MakeCylindricalWave(CFunctionalMesh&mesh)
{
    using point_t=Eigen::Vector3f;
    using namespace std::numbers;
    CRenderingTraits rt;
    rt.SetMesh(false).SetSpecular(true).SetColored(true).SetSurface(true);
    auto make_wave=[](float omega)
    {
        return plot::cylindrical([omega](float ro,float phi,float t)
        {
            return 3*std::cyl_bessel_j(0, ro)*std::sin(omega*t)-
                   3*std::cyl_neumann(0, ro)*std::cos(omega*t);
        });
    };
    auto color_func=[](float,float,float z)->point_t
    {
        return point_t(0,0,1)+z*point_t(1,1,-1);
    };
    mesh.SetMeshFunctor(make_wave(2*pi_v<float>/2.0))
        .SetColorFunctor(color_func)
        .SetResolution(40,40)
        .SetTraits(rt)
        .SetRange({0.1,25},{0,2*pi_v<float>});
}

void MakeSphericalWave(CFunctionalMesh&mesh)
{
    using point_t=Eigen::Vector3f;
    using namespace std::numbers;
    CRenderingTraits rt;
    rt.SetMesh(true).SetSpecular(true).SetColored(true).SetSurface(true);
    auto make_wave=[](float omega)
    {
        return plot::spherical([omega](float theta,float phi,float t)
        {
            return 2+std::cos(theta)*std::cos(theta)*sin(omega*t);
        });
    };
    mesh.SetMeshFunctor(make_wave(2*pi_v<float>/2.0))
        .SetResolution(20,20)
        .SetTraits(rt)
        .SetRange({0,pi_v<float>},{0,2*pi_v<float>});
}

void MakeRevolveWave(CFunctionalMesh&mesh)
{
    using point_t=Eigen::Vector3f;
    using namespace std::numbers;
    CRenderingTraits rt;
    rt.SetMesh(true).SetSpecular(true).SetColored(false).SetSurface(true).SetTwoSideSpecular(true);
    auto make_wave=[](float omega)
    {
        return plot::revolve([omega](float phi,float ro,float t)
        {
            return 2+std::sin(2*ro)*std::cos(t*omega);
        });
    };
    mesh.SetMeshFunctor(make_wave(2*pi_v<float>/2.0))
        .SetResolution(20,20)
        .SetTraits(rt)
        .SetRange({0,2*pi_v<float>},{-2,2});
}


CScene*gl_scene_ptr=nullptr;

int main()
{
    using point_t=Eigen::Vector3f;
    using namespace std::numbers;
    auto window=MakeWindow();

    CScene glScene;
    gl_scene_ptr=&glScene;

    CFunctionalMesh mesh;
    MakeRevolveWave(mesh);

    glScene.LightSource().SetPosition(0,0,5);
    glScene.AddMesh(mesh);

    glEnable(GL_DEPTH_TEST);
    glCheckError();

    for (;!glfwWindowShouldClose(window);)
    {
        glClearColor(0.0f, 0.0, 0.0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
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





















