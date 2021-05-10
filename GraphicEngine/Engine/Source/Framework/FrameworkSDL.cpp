#include "Framework.h"

#if defined(GAP311_ENABLE_SDL)
#include <SDL.h>
#include <SDL_vulkan.h>

namespace GAP311
{

class FrameworkSDL : public IFramework
{
public:
    bool Initialize(int32_t windowWidth, int32_t windowHeight) final override;
    void Shutdown() final override;
    void PumpEvents() final override;

    vk::SurfaceKHR CreateWindowSurface(vk::Instance instance) final override;

    void Log(const char* pMessage) final override;

    void RequestQuit() final override { m_quitRequested = true; }
    bool IsQuitRequested() const final override { return m_quitRequested; }

    float GetFrameTime() const final override { return m_frameTime; }

    vk::Offset2D GetMousePosition() const final override
    {
        return m_mousePosition;
    }

    vk::Offset2D GetMouseDelta() const final override
    {
        return m_mouseDelta;
    }

    bool IsKeyDown(KeyCode keyCode) const final override
    {
        return m_keyboardState[ConvertKeyCode(keyCode)] != 0;
    }

private:
    static uint32_t ConvertKeyCode(KeyCode code);

    bool m_quitRequested = false;
    SDL_Window* m_pWindow = nullptr;
    std::vector<Uint8> m_keyboardState;
    Uint64 m_ticksLast = 0;
    Uint64 m_ticksFreq = 1;
    float m_frameTime = 0;
    vk::Offset2D m_mousePosition;
    vk::Offset2D m_mouseDelta;
};

IFramework* FrameworkCreateSDL()
{
    return new FrameworkSDL();
}

bool FrameworkSDL::Initialize(int32_t windowWidth, int32_t windowHeight)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("FrameworkSDL",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight, SDL_WINDOW_VULKAN);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }

    m_ticksFreq = SDL_GetPerformanceFrequency();
    m_ticksLast = SDL_GetPerformanceCounter();

    m_pWindow = window;
    return true;
}

void FrameworkSDL::Shutdown()
{
    if (m_pWindow)
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
    }

    SDL_Quit();
}

void FrameworkSDL::PumpEvents()
{
    SDL_PumpEvents();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            m_quitRequested = true;
            break;
        }
    }

    int totalKeys = 0;
    const Uint8* keys = SDL_GetKeyboardState(&totalKeys);
    m_keyboardState.assign(keys, keys + totalKeys);

    vk::Offset2D lastPosition = m_mousePosition;
    SDL_GetMouseState(&m_mousePosition.x, &m_mousePosition.y);
    m_mouseDelta.x = m_mousePosition.x - lastPosition.x;
    m_mouseDelta.y = m_mousePosition.y - lastPosition.y;

    Uint64 ticksNow = SDL_GetPerformanceCounter();
    m_frameTime = static_cast<float>(static_cast<double>(ticksNow - m_ticksLast) / m_ticksFreq);
    m_ticksLast = ticksNow;
}

vk::SurfaceKHR FrameworkSDL::CreateWindowSurface(vk::Instance instance)
{
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(m_pWindow, instance, &surface))
    {
        SDL_Log("SDL failed to create Vulkan surface: %s\n", SDL_GetError());
    }
    return surface;
}

void FrameworkSDL::Log(const char* pMessage)
{
    SDL_Log("%s", pMessage);
}

uint32_t FrameworkSDL::ConvertKeyCode(KeyCode code)
{
    switch (code)
    {
    case KeyCode::A: return SDL_SCANCODE_A;
    case KeyCode::B: return SDL_SCANCODE_B;
    case KeyCode::C: return SDL_SCANCODE_C;
    case KeyCode::D: return SDL_SCANCODE_D;
    case KeyCode::E: return SDL_SCANCODE_E;
    case KeyCode::F: return SDL_SCANCODE_F;
    case KeyCode::G: return SDL_SCANCODE_G;
    case KeyCode::H: return SDL_SCANCODE_H;
    case KeyCode::I: return SDL_SCANCODE_I;
    case KeyCode::J: return SDL_SCANCODE_J;
    case KeyCode::K: return SDL_SCANCODE_K;
    case KeyCode::L: return SDL_SCANCODE_L;
    case KeyCode::M: return SDL_SCANCODE_M;
    case KeyCode::N: return SDL_SCANCODE_N;
    case KeyCode::O: return SDL_SCANCODE_O;
    case KeyCode::P: return SDL_SCANCODE_P;
    case KeyCode::Q: return SDL_SCANCODE_Q;
    case KeyCode::R: return SDL_SCANCODE_R;
    case KeyCode::S: return SDL_SCANCODE_S;
    case KeyCode::T: return SDL_SCANCODE_T;
    case KeyCode::U: return SDL_SCANCODE_U;
    case KeyCode::V: return SDL_SCANCODE_V;
    case KeyCode::W: return SDL_SCANCODE_W;
    case KeyCode::X: return SDL_SCANCODE_X;
    case KeyCode::Y: return SDL_SCANCODE_Y;
    case KeyCode::Z: return SDL_SCANCODE_Z;
    case KeyCode::Num0: return SDL_SCANCODE_0;
    case KeyCode::Num1: return SDL_SCANCODE_1;
    case KeyCode::Num2: return SDL_SCANCODE_2;
    case KeyCode::Num3: return SDL_SCANCODE_3;
    case KeyCode::Num4: return SDL_SCANCODE_4;
    case KeyCode::Num5: return SDL_SCANCODE_5;
    case KeyCode::Num6: return SDL_SCANCODE_6;
    case KeyCode::Num7: return SDL_SCANCODE_7;
    case KeyCode::Num8: return SDL_SCANCODE_8;
    case KeyCode::Num9: return SDL_SCANCODE_9;
    case KeyCode::Left: return SDL_SCANCODE_LEFT;
    case KeyCode::Right: return SDL_SCANCODE_RIGHT;
    case KeyCode::Up: return SDL_SCANCODE_UP;
    case KeyCode::Down: return SDL_SCANCODE_DOWN;
    case KeyCode::Space: return SDL_SCANCODE_SPACE;
    case KeyCode::Enter: return SDL_SCANCODE_RETURN;
    }

    return SDL_SCANCODE_UNKNOWN;
}

}

#endif // GAP311_ENABLE_SDL