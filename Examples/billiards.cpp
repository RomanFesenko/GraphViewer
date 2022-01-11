
#include <iostream>
#include <vector>
#include <random>

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

class CAnimator
{
    using point_t=Eigen::Vector3f;
    float m_prev_time;
    float m_radius;
    std::pair<float,float> m_delta_x;
    std::pair<float,float> m_delta_y;
    float m_x_min;
    float m_x_max;
    float m_y_min;
    float m_y_max;

    std::vector<point_t> m_balls_speeds;
    std::vector<CFunctionalMesh*> m_balls;
    public:
    CAnimator(float rad,const std::pair<float,float>& dx,const std::pair<float,float>& dy)
    {
        m_radius=rad;
        m_delta_x=dx;
        m_delta_y=dy;

        m_x_min=m_delta_x.first+m_radius;
        m_x_max=m_delta_x.second-m_radius;
        m_y_min=m_delta_y.first+m_radius;
        m_y_max=m_delta_y.second-m_radius;
    }
    void operator()(float t)
    {
        for(int i=0;i<m_balls.size();++i)
        {
            auto m=m_balls[i];
            auto org=m->GetOrg();
            point_t&speed=m_balls_speeds[i];
            if((org[0]<=m_x_min&&speed[0]<0)||(org[0]>=m_x_max&&speed[0]>0))
            {
                speed[0]=-speed[0];
            }
            if((org[1]<=m_y_min&&speed[1]<0)||(org[1]>=m_y_max&&speed[1]>0))
            {
                speed[1]=-speed[1];
            }
        }
        for(int i=0;i<m_balls.size();++i)
        {
            auto&i_speed=m_balls_speeds[i];
            for(int j=0;j<m_balls.size();++j)
            {
                if(i==j) continue;
                auto&j_speed=m_balls_speeds[j];
                point_t delta=(m_balls[j]->GetOrg()-m_balls[i]->GetOrg());
                if(delta.norm()<=2*m_radius&&(i_speed-j_speed).dot(delta)>=0)
                {
                    std::swap(i_speed,j_speed);
                }
            }
        }
        float passed=t-m_prev_time;
        m_prev_time=t;
        for(int i=0;i<m_balls.size();++i)
        {
            auto&speed=m_balls_speeds[i];
            m_balls[i]->DeltaOrg(speed*passed);
        }
    }
    CAnimator& AddBall(CFunctionalMesh&mesh,float x,float y)
    {
        m_balls.push_back(&mesh);
        mesh.SetOrg(x,y,m_radius);
        return *this;
    }
    void Init(float time)
    {
        m_prev_time=time;
        std::default_random_engine dre;
        std::uniform_real_distribution<float> urd(m_x_min-m_x_max,m_x_max-m_x_min);
        m_balls_speeds.clear();
        for(int i=0;i<m_balls.size();++i)
        {
            m_balls_speeds.push_back({urd(dre),urd(dre),0});
        }
    }
};



CScene*gl_scene_ptr=nullptr;

int main()
{
    using point_t=Eigen::Vector3f;
    auto window=MakeWindow();

    CScene glScene;
    gl_scene_ptr=&glScene;

    const float rad=1;
    CFunctionalMesh fmesh_sphere[4],fmesh_plane;
    CAnimator anim(rad,{-5,5},{-5,5});
    CRenderingTraits rt;
    rt.SetMesh(false).SetSurface(true).SetSpecular(true).SetColored(true).SetBox(false);
    fmesh_plane.SetPlane(10,10).SetTraits(rt).SetUniformColor({0,1,1});

    for(auto&ball:fmesh_sphere)
    {
        ball.SetSphere(rad,30,30).SetTraits(rt);
        glScene.AddMesh(ball);
    }


    anim.AddBall(fmesh_sphere[0],-1,-1)
        .AddBall(fmesh_sphere[1],1,-1)
        .AddBall(fmesh_sphere[2],-1,1)
        .AddBall(fmesh_sphere[3],1,1);


    glScene.AddMesh(fmesh_plane);
    glScene.LightSource().SetPosition({0,0,4});


    anim.Init(glfwGetTime());
    glEnable(GL_DEPTH_TEST);
    for (;!glfwWindowShouldClose(window);)
    {
        glClearColor(0.0f, 0.0, 0.0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glScene.Render(glfwGetTime());
        anim(glfwGetTime());
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





















