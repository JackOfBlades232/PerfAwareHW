#include "input.hpp"
#include <cstdio>

namespace input
{

bool g_enabled = false;

// @FEAT: Echo should be disabled. However, I don't want to
//        write platform-specific code in this project, so no luck.

void enable()
{
    g_enabled = true;
}

void disable()
{
    g_enabled = false;
}

bool interactivity_enabled()
{
    return g_enabled;
}

void wait_for_lf()
{
    if (!interactivity_enabled())
        return;

    char c;
    while ((c = getchar()) != EOF) {
        if (c == '\n')
            return;
    }
}

}
