#include "../src/uiwarp.h"
#include <iostream>
#include <fstream>
#include <glfw/glfw3.h>
#include <unordered_map>
using namespace std;
using namespace UIWarp;

void draw(Vec4i rect) // By qiaozhanrong
{
    glBegin(GL_QUADS);
    glVertex2i(rect.x, rect.y);
    glVertex2i(rect.x, rect.y + rect.h);
    glVertex2i(rect.x + rect.w, rect.y + rect.h);
    glVertex2i(rect.x + rect.w, rect.y);
    glEnd();
}
inline double max(double num1, double num2)
{
    return num1 > num2 ? num1 : num2;
}
void print(Layout& layout)
{
    for (auto& child : layout)
    {
        if (child->is_control())
        {
            auto control = dynamic_pointer_cast<Control>(child);
            cout << "Control " << control->id << ": " << control->type
                 << " pos(" << control->rect.x << "," << control->rect.y << ") "
                 << "size(" << control->rect.w << "," << control->rect.h << ")" << endl;
        }
        else
        {
            auto layout = dynamic_pointer_cast<Layout>(child);
            auto rect = layout->get_rect();

            cout << "Layout "<<layout->get_property<std::string>("id")<<": " << layout->get_type()
                 << " pos(" << rect.x << "," << rect.y << ") "
                 << "size(" << rect.w << "," << rect.h << ")" << endl;
        }
    }
}
struct Color { double r, g, b; };
constexpr int win_w = 800, win_h = 600;
Layout* layout_global_ref = nullptr;
int main(int argc,char* argv[])
{
    // Try to read layout from file
    pugi::xml_document xml;
    xml.load_file(argv[1]);
    Layout layout(xml, { 0, 0, win_w, win_h });
    print(layout);
    layout_global_ref=&layout;
    GLFWwindow* window;

    if (!glfwInit()) return -1;
    window = glfwCreateWindow(win_w, win_h, "Preview", nullptr, nullptr);

    glfwMakeContextCurrent(window);
    glPushMatrix();
    glViewport(0, 0, win_w, win_h);
    glOrtho(0.0, win_w, 0.0, win_h, -10, 10);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glfwSetWindowSizeCallback(window, [](GLFWwindow*, int w, int h) {
        glLoadIdentity();
        glViewport(0, 0, w, h);
        glOrtho(0.0, w, 0.0, h, -10, 10);
        layout_global_ref->set_rect({ 0,0,w,h });
    });
    unordered_map<string, Color> color_map;
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        if (glfwGetKey(window, GLFW_KEY_R))
        {
            xml.load_file(argv[1]);
            layout.load(xml);
            print(layout);
        }
        for (auto& child : layout)
        {
            if (!child->is_control()) continue;
            auto control = dynamic_pointer_cast<Control>(child);
            if (color_map.find(control->id) == color_map.end())
                color_map[control->id] =
            {
                max(rand() / static_cast<double>(RAND_MAX), 0.1),
                max(rand() / static_cast<double>(RAND_MAX), 0.1),
                max(rand() / static_cast<double>(RAND_MAX), 0.1)
            };
            auto color = color_map[control->id];
            glColor3d(color.r, color.g, color.b);
            draw(control->rect);
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
}
