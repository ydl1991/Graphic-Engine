#include "Application.h"

int main()
{
    Application app;

    if (!app.Initialize(1920, 1280, GAP311::FrameworkType::eGLFW))
    {
        printf("Failed to initialize application!\n");
        app.Shutdown();
        return -1;
    }

    app.Run();
    app.Shutdown();

    return 0;
}
