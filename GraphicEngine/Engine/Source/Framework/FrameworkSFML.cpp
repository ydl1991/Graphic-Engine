#include "Framework.h"

#if defined(GAP311_ENABLE_SFML)
#include <SFML/Window.hpp>
#include <vulkan/vulkan.hpp>

namespace GAP311
{


class FrameworkSFML : public IFramework
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
        return { m_mousePosition.x, m_mousePosition.y };
    }

    vk::Offset2D GetMouseDelta() const final override
    {
        return { m_mouseDelta.x, m_mouseDelta.y };
    }

    bool IsKeyDown(KeyCode keyCode) const final override
    { 
        return sf::Keyboard::isKeyPressed(ConvertKeyCode(keyCode));
    }

private:
    static sf::Keyboard::Key ConvertKeyCode(KeyCode code);

    bool m_quitRequested = false;
    sf::Window m_window;
    sf::Clock m_clock;
    float m_frameTime = 0;
    sf::Vector2i m_mousePosition;
    sf::Vector2i m_mouseDelta;
};

IFramework* FrameworkCreateSFML()
{
    return new FrameworkSFML();
}

bool FrameworkSFML::Initialize(int32_t windowWidth, int32_t windowHeight)
{
    printf(
        "NOTE: SFML support is janky at the moment as it does not have official "
        "support for Vulkan in its last release.  SFML still creates an OpenGL "
        "context which Vulkan will complain about but still operate with. "
        "However, SFML may crash on shutdown when cleaning up the OpenGL context.\n"
        "You may want to consider using a different framework until SFML gets "
        "official Vulkan support.\n"
    );

    m_window.create(sf::VideoMode(windowWidth, windowHeight), "FrameworkSFML");
    if (!m_window.isOpen())
    {
        printf("Failed to create SFML window!\n");
        return false;
    }

    m_clock.restart();
    return true;
}

void FrameworkSFML::Shutdown()
{
    if (m_window.isOpen())
        m_window.close();
}

void FrameworkSFML::PumpEvents()
{
    sf::Event event;
    while (m_window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
        {
            m_quitRequested = true;
        }
    }

    auto lastPosition = m_mousePosition;
    m_mousePosition = sf::Mouse::getPosition();
    m_mouseDelta = m_mousePosition - lastPosition;

    m_frameTime = m_clock.getElapsedTime().asSeconds();
    m_clock.restart();
}

vk::SurfaceKHR FrameworkSFML::CreateWindowSurface(vk::Instance instance)
{
    vk::Win32SurfaceCreateInfoKHR surfaceInfo;
    surfaceInfo.hinstance = ::GetModuleHandle(NULL);
    surfaceInfo.hwnd = reinterpret_cast<HWND>(m_window.getSystemHandle());

    vk::SurfaceKHR surface = instance.createWin32SurfaceKHR(surfaceInfo);
    if (!surface)
    {
        printf("SFML failed to create Vulkan surface.\n");
    }
    return surface;
}

void FrameworkSFML::Log(const char* pMessage)
{
    printf("%s\n", pMessage);
}

sf::Keyboard::Key FrameworkSFML::ConvertKeyCode(KeyCode code)
{
    switch (code)
    {
    case KeyCode::A: return sf::Keyboard::A;
    case KeyCode::B: return sf::Keyboard::B;
    case KeyCode::C: return sf::Keyboard::C;
    case KeyCode::D: return sf::Keyboard::D;
    case KeyCode::E: return sf::Keyboard::E;
    case KeyCode::F: return sf::Keyboard::F;
    case KeyCode::G: return sf::Keyboard::G;
    case KeyCode::H: return sf::Keyboard::H;
    case KeyCode::I: return sf::Keyboard::I;
    case KeyCode::J: return sf::Keyboard::J;
    case KeyCode::K: return sf::Keyboard::K;
    case KeyCode::L: return sf::Keyboard::L;
    case KeyCode::M: return sf::Keyboard::M;
    case KeyCode::N: return sf::Keyboard::N;
    case KeyCode::O: return sf::Keyboard::O;
    case KeyCode::P: return sf::Keyboard::P;
    case KeyCode::Q: return sf::Keyboard::Q;
    case KeyCode::R: return sf::Keyboard::R;
    case KeyCode::S: return sf::Keyboard::S;
    case KeyCode::T: return sf::Keyboard::T;
    case KeyCode::U: return sf::Keyboard::U;
    case KeyCode::V: return sf::Keyboard::V;
    case KeyCode::W: return sf::Keyboard::W;
    case KeyCode::X: return sf::Keyboard::X;
    case KeyCode::Y: return sf::Keyboard::Y;
    case KeyCode::Z: return sf::Keyboard::Z;
    case KeyCode::Num0: return sf::Keyboard::Num0;
    case KeyCode::Num1: return sf::Keyboard::Num1;
    case KeyCode::Num2: return sf::Keyboard::Num2;
    case KeyCode::Num3: return sf::Keyboard::Num3;
    case KeyCode::Num4: return sf::Keyboard::Num4;
    case KeyCode::Num5: return sf::Keyboard::Num5;
    case KeyCode::Num6: return sf::Keyboard::Num6;
    case KeyCode::Num7: return sf::Keyboard::Num7;
    case KeyCode::Num8: return sf::Keyboard::Num8;
    case KeyCode::Num9: return sf::Keyboard::Num9;
    case KeyCode::Left: return sf::Keyboard::Left;
    case KeyCode::Right: return sf::Keyboard::Right;
    case KeyCode::Up: return sf::Keyboard::Up;
    case KeyCode::Down: return sf::Keyboard::Down;
    case KeyCode::Space: return sf::Keyboard::Space;
    case KeyCode::Enter: return sf::Keyboard::Enter;
    }

    return sf::Keyboard::Unknown;
}

}

#endif // GAP311_ENABLE_SFML