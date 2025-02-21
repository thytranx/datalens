#include "window.h"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

Window::Window(int screen_width, int screen_height,
               GLFWframebuffersizefun framebuffer_size_callback,
               GLFWcursorposfun mouse_callback,
               GLFWscrollfun scroll_callback,
               GLFWkeyfun toggle_cursor
)

{
	this->_scr_width = screen_height;
	this->_scr_height = screen_height;

	// Init & set major & minor OpenGL version to 3
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(screen_width, screen_height, "Datalens", nullptr, nullptr);

	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window \n";
		glfwTerminate();
	}

	// Associate OpenGL context with specificed window
	glfwMakeContextCurrent(window);

	// Callback everytime window's framebuffer size changes
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Callback whenever mouse cursor moves within the window's client area
    glfwSetCursorPosCallback(window, mouse_callback);

	// Callback whenever mouse scroll is used
    glfwSetScrollCallback(window, scroll_callback);

	// Callback whenever toggle key is pressed
    glfwSetKeyCallback(window, toggle_cursor);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD \n";
	}

	// Renderer info retrieving
    const GLubyte* renderer = glGetString (GL_RENDERER);
    const GLubyte* version = glGetString (GL_VERSION);
    std::cout<<"Renderer: "<<renderer<<std::endl;
    std::cout<<"OpenGL version supported "<<version<<std::endl;

    // Smooth out jagged edges (anti-aliasing)
    glfwWindowHint(GLFW_SAMPLES, 8);
    glEnable(GL_MULTISAMPLE);

    // Vertical synchronization (vsync)
	glfwSwapInterval(1);
}

float Window::get_aspect_ratio() const {
    return float(_scr_width) / float(_scr_height);
}
