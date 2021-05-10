#include "Framework.h"

#if defined(GAP311_ENABLE_GLFW)
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.hpp>

namespace GAP311
{

class FrameworkGLFW : public IFramework
{
public:
    bool Initialize(int32_t windowWidth, int32_t windowHeight) final override;
    void Shutdown() final override;
    void PumpEvents() final override;

    vk::SurfaceKHR CreateWindowSurface(vk::Instance instance) final override;

    void Log(const char* pMessage) final override;

    void RequestQuit() final override { m_quitRequested = true; }
    bool IsQuitRequested() const final override { return m_quitRequested; }

    float GetFrameTime() const { return m_frameTime; }

    vk::Offset2D GetMousePosition() const final override
    {
        return { static_cast<int32_t>(m_mouseX), static_cast<int32_t>(m_mouseY) };
    }

    vk::Offset2D GetMouseDelta() const final override
    {
        return { static_cast<int32_t>(m_mouseDeltaX), static_cast<int32_t>(m_mouseDeltaY) };
    }

    bool IsKeyDown(KeyCode keyCode) const final override
    {
        return glfwGetKey(m_pWindow, ConvertKeyCode(keyCode)) == GLFW_PRESS;
    }

private:
    static uint32_t ConvertKeyCode(KeyCode code);

    bool m_quitRequested = false;
    GLFWwindow* m_pWindow = nullptr;
    uint64_t m_ticksLast = 0;
    uint64_t m_ticksFreq = 1;
    float m_frameTime = 0;

    double m_mouseX = 0, m_mouseY = 0;
    double m_mouseDeltaX = 0, m_mouseDeltaY = 0;
};

IFramework* FrameworkCreateGLFW()
{
    return new FrameworkGLFW();
}

bool FrameworkGLFW::Initialize(int32_t windowWidth, int32_t windowHeight)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // GLFW might default to initializing opengl, prevent it

    m_pWindow = glfwCreateWindow(windowWidth, windowHeight, "FrameworkGLFW", nullptr, nullptr);
    if (!m_pWindow)
    {
        printf("Failed to create GLFW window!\n");
        return false;
    }

    glfwSetWindowUserPointer(m_pWindow, this);

    m_ticksFreq = glfwGetTimerFrequency();
    m_ticksLast = glfwGetTimerValue();

    return true;
}

void FrameworkGLFW::Shutdown()
{
    if (m_pWindow)
    {
        glfwDestroyWindow(m_pWindow);
        m_pWindow = nullptr;
    }

    glfwTerminate();
}

void FrameworkGLFW::PumpEvents()
{
    glfwPollEvents();

    if (glfwWindowShouldClose(m_pWindow))
        m_quitRequested = true;

    double lastMouseX = m_mouseX;
    double lastMouseY = m_mouseY;
    glfwGetCursorPos(m_pWindow, &m_mouseX, &m_mouseY);
    m_mouseDeltaX = m_mouseX - lastMouseX;
    m_mouseDeltaY = m_mouseY - lastMouseY;

    uint64_t ticksNow = glfwGetTimerValue();
    m_frameTime = static_cast<float>(static_cast<double>(ticksNow - m_ticksLast) / m_ticksFreq);
    m_ticksLast = ticksNow;
}

vk::SurfaceKHR FrameworkGLFW::CreateWindowSurface(vk::Instance instance)
{
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(instance, m_pWindow, nullptr, &surface);
    if (surface == VK_NULL_HANDLE)
    {
        printf("GLFW failed to create Vulkan surface.\n");
    }
    return surface;
}

void FrameworkGLFW::Log(const char* pMessage)
{
    printf("%s\n", pMessage);
}

uint32_t FrameworkGLFW::ConvertKeyCode(KeyCode code)
{
    switch (code)
    {
    case KeyCode::A: return GLFW_KEY_A;
    case KeyCode::B: return GLFW_KEY_B;
    case KeyCode::C: return GLFW_KEY_C;
    case KeyCode::D: return GLFW_KEY_D;
    case KeyCode::E: return GLFW_KEY_E;
    case KeyCode::F: return GLFW_KEY_F;
    case KeyCode::G: return GLFW_KEY_G;
    case KeyCode::H: return GLFW_KEY_H;
    case KeyCode::I: return GLFW_KEY_I;
    case KeyCode::J: return GLFW_KEY_J;
    case KeyCode::K: return GLFW_KEY_K;
    case KeyCode::L: return GLFW_KEY_L;
    case KeyCode::M: return GLFW_KEY_M;
    case KeyCode::N: return GLFW_KEY_N;
    case KeyCode::O: return GLFW_KEY_O;
    case KeyCode::P: return GLFW_KEY_P;
    case KeyCode::Q: return GLFW_KEY_Q;
    case KeyCode::R: return GLFW_KEY_R;
    case KeyCode::S: return GLFW_KEY_S;
    case KeyCode::T: return GLFW_KEY_T;
    case KeyCode::U: return GLFW_KEY_U;
    case KeyCode::V: return GLFW_KEY_V;
    case KeyCode::W: return GLFW_KEY_W;
    case KeyCode::X: return GLFW_KEY_X;
    case KeyCode::Y: return GLFW_KEY_Y;
    case KeyCode::Z: return GLFW_KEY_Z;
    case KeyCode::Num0: return GLFW_KEY_0;
    case KeyCode::Num1: return GLFW_KEY_1;
    case KeyCode::Num2: return GLFW_KEY_2;
    case KeyCode::Num3: return GLFW_KEY_3;
    case KeyCode::Num4: return GLFW_KEY_4;
    case KeyCode::Num5: return GLFW_KEY_5;
    case KeyCode::Num6: return GLFW_KEY_6;
    case KeyCode::Num7: return GLFW_KEY_7;
    case KeyCode::Num8: return GLFW_KEY_8;
    case KeyCode::Num9: return GLFW_KEY_9;
    case KeyCode::Left: return GLFW_KEY_LEFT;
    case KeyCode::Right: return GLFW_KEY_RIGHT;
    case KeyCode::Up: return GLFW_KEY_UP;
    case KeyCode::Down: return GLFW_KEY_DOWN;
    case KeyCode::Space: return GLFW_KEY_SPACE;
    case KeyCode::Enter: return GLFW_KEY_ENTER;
    }

    return GLFW_KEY_UNKNOWN;
}

}

#endif // GAP311_ENABLE_GLFW