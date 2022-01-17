GraphViewer. 05/25/21.

Application for building and viewing 3D and 2D graphs, with support for animation and various display methods, with Qt GUI.

Example for use base render library:

```C++
#include <iostream>
#include <vector>
#include <numbers>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include "functional_mesh.h"
#include "view_ruling.h"
#include "scene.h"

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
    auto window=MakeWindow();// make window,create OpenGL context,set callbacks. 
    CScene glScene;
    gl_scene_ptr=&glScene;

    CRenderingTraits rt;
    CFunctionalMesh mesh[4];

    rt.SetMesh(false).SetSpecular(true).SetSurface(true).SetTwoSideSpecular(true);
    
    // Build klein bottle surface from patches.
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
        glScene.Render(glfwGetTime());// Rendering surface
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
```
Result:

![klein_bottle](https://user-images.githubusercontent.com/72913560/149735165-2d1a4d1c-e839-412c-afbb-35d9a269a233.png)












