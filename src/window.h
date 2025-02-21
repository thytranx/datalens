#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct Window
{
	GLFWwindow* window;
	unsigned int _scr_width;
	unsigned int _scr_height;

	Window(int screen_width, int screen_height,
           GLFWframebuffersizefun framebuffer_size_callback,
           GLFWcursorposfun mouse_callback,
           GLFWscrollfun scroll_callback,
           GLFWkeyfun toggle_cursor
           );

    float get_aspect_ratio() const;
};
