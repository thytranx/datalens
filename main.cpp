#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "src/behavior_inspector.h"

#include "src/frame_counter.h"
#include "src/shader.h"
#include "src/window.h"
#include "src/camera.h"
#include "src/drawable_mesh.h"
#include "src/drawable_model.h"

float crosshair_size;
constexpr  float crosshair_size_max = 0.01f;
float crosshair_size_target = crosshair_size_max;
FrameCounter frame_counter;
bool fps_mode;
Camera camera;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos_arg, double ypos_arg);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void toggle_cursor(GLFWwindow* window, int key, int scancode, int action, int mods);
void input_processing(GLFWwindow* window);

Window window_object(1024, 768, framebuffer_size_callback, mouse_callback, scroll_callback, toggle_cursor);


int main()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window_object.window, true);
    ImGui_ImplOpenGL3_Init();

    Shader basic_shader("shaders/basic.vert", "shaders/basic.frag");
    Shader flat_shader("shaders/tex.vert", "shaders/tex_flat.frag");
    Shader blinn_shader("shaders/tex.vert", "shaders/tex_blinn.frag");
    Shader tex_shader("shaders/tex.vert", "shaders/tex.frag");
    Shader grad_shader("shaders/tex.vert", "shaders/tex_grad.frag");
    Shader dither_shader("shaders/tex.vert", "shaders/tex_dither.frag");

    std::vector<Shader*> shaders = {&flat_shader, &blinn_shader, &tex_shader, &grad_shader, &dither_shader};

    // Default place holder
    objl::Loader loader;
    loader.LoadFile("resources/ball.obj");
    DrawableMesh defaultObject(GL_STATIC_DRAW, loader.LoadedMeshes[0]);

    DrawableModel oneq8i(GL_STATIC_DRAW,
                   "resources/_1q8i/1q8i.obj", "resources/_1q8i/textures/");

    std::vector<DrawableModel*> models_list = {&oneq8i};

    glEnable(GL_DEPTH_TEST);

    ModelBehaviorInspector model_behavior_inspector;
    ModelBehaviorInspector chat_window;

    while (!glfwWindowShouldClose(window_object.window))
    {
        // Init a new frame for ImGui library for OpenGL and GLFW
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Handles user input from keyboard and mouse events
        input_processing(window_object.window);

        // Display information related to the objects via UI elements
        model_behavior_inspector.render(window_object, camera, models_list);

        // Update camera object
        camera.Update(frame_counter.deltaTime);

        // Tracking frame timing information
        frame_counter.update(false);

        // Retrieves background color from property_inspector
        auto color = model_behavior_inspector.background_color;

        // Set the clear color for the OpenGL context and clears the specified buffers
        glClearColor(color[0], color[1], color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Linear interpolation between current value of the size of crosshair and its target value
        crosshair_size = glm::mix(crosshair_size, crosshair_size_target, 0.1f);

        // Create 4x4 identity matrix named matrix_model
        glm::mat4 matrix_model = glm::mat4(1.0f);

        // Retrieves the position of the object from property_inspector
        auto position = model_behavior_inspector.position;

        // Translation operation
        matrix_model = glm::translate(matrix_model, glm::vec3(position[0], position[1], position[2]));

        // Retrieves the rotation of the object from property_inspector
        auto rotation = model_behavior_inspector.rotation;

        // Rotation operation
        matrix_model = glm::rotate(matrix_model, glm::radians(rotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));

        // Checks the value of rotaable property
        if (model_behavior_inspector.rotatable)
            rotation[1] += frame_counter.deltaTime * 10.0f;

        // Keeps the y-axis rotation within 0 - 360 degree range
        rotation[1] = fmodf(rotation[1], 360.0f);

        // Apply rotations around the y-axis and the z-axis
        // glm::radians converts angles from degress to radians
        matrix_model = glm::rotate(matrix_model, glm::radians(rotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
        matrix_model = glm::rotate(matrix_model, glm::radians(rotation[2]), glm::vec3(0.0f, 0.0f, 1.0f));

        // Retrieves the scalling factors from a structure likely used for inspecting and modifying model properties
        auto scale = model_behavior_inspector.scale;

        // Applies scaling to the model using retrieved scale factors
        matrix_model = glm::scale(matrix_model, glm::vec3(scale[0], scale[1], scale[2]));

        // Camera Projection Setup - creates a perspective projection matrix
        glm::mat4 projection = glm::perspective(
                glm::radians(camera.Zoom), window_object.get_aspect_ratio(), 0.1f, 1000.0f);

        // Shader setup and Model rendering
        Shader this_shader = *shaders[model_behavior_inspector.current_shader]; // Choose the current shader
        this_shader.use(); // Activates the selected shader

        // Sets shader uniforms: time, model, view, projection, fps_mode, camPos
        grad_shader.setFloat("time", glfwGetTime()); //
        this_shader.setMat4("model", matrix_model);
        this_shader.setMat4("view", camera.GetViewMatrix(!fps_mode));
        this_shader.setMat4("projection", projection);
        const glm::vec3 camPos = fps_mode ? camera.Position : camera.OrbitPosition;
        this_shader.setVec3("camPos", camPos);

        // Renders the current model
        models_list[model_behavior_inspector.current_model]->Draw();

        basic_shader.use(); // Activates the basic shader

        // Set view and projection matrices for the basic shader
        basic_shader.setMat4("view", camera.GetViewMatrix(!fps_mode));
        basic_shader.setMat4("projection", projection);
        matrix_model = glm::mat4(1.0f); // Resets matrix_model to an identify matrix

        // Translates the crosshair to camera.TargetSmooth
        matrix_model = glm::translate(matrix_model, camera.TargetSmooth);
        float factor = model_behavior_inspector.hide_crosshair ? 0 : 1;
        matrix_model = glm::scale(matrix_model, glm::vec3(crosshair_size * factor));
        basic_shader.setVec4("color", 1, 0, 0, 1.0f);
        basic_shader.setMat4("model", matrix_model);
        defaultObject.Draw();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_object.window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

bool first_mouse = true;

void focus(GLFWwindow* window)
{
    crosshair_size_target = 0;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void unfocus(GLFWwindow* window)
{
    crosshair_size_target = crosshair_size_max;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    first_mouse = true;
}

void toggle_cursor(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS) {
        fps_mode = !fps_mode;
        if (fps_mode)
            focus(window);
        else
            unfocus(window);
    }
}

void input_processing(GLFWwindow* window)
{
    // Check if the Escape key is pressed, if it is, sets the windows close flag to true, terminate application
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);


    ImGuiIO& io = ImGui::GetIO();
    if (!fps_mode && !io.WantCaptureMouse) // Only if not in FPS mode and ImGui isn't using the mouse
    {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
            focus(window); // Capture mouse
        else if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
            unfocus(window); // Release mouse
    }

    if (fps_mode)
    {
        float dt = frame_counter.deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            dt += 5.0f; // Increase speed if Shift key is held

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, dt);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, dt);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, dt);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, dt);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.ProcessKeyboard(DOWN, dt);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.ProcessKeyboard(UP, dt);
    }
}

float last_X, last_Y;

void mouse_callback(GLFWwindow* window, double xpos_arg, double ypos_arg)
{
    float x_pos = static_cast<float>(xpos_arg);
    float y_pos = static_cast<float>(ypos_arg);

    if (first_mouse)
    {
        last_X = x_pos;
        last_Y = y_pos;
        first_mouse = false;
    }

    float xoffset = x_pos - last_X;
    float yoffset = last_Y - y_pos; // reversed since y-coordinates go from bottom to top

    last_X = x_pos;
    last_Y = y_pos;

    auto pan = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2);

    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        camera.ProcessMouseMovement(xoffset, yoffset, !fps_mode, pan);
}

void scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
    camera.ProcessMouseScroll(y_offset, !fps_mode);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
};
