// Compatibility forwarding header.
// The bundled GLFW package stores headers directly in include/, while upstream
// consumers (including Dear ImGui's official backend) use <GLFW/glfw3.h>.
#pragma once
#include "../glfw3.h"
