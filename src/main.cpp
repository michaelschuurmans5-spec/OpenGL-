// 1. ALWAYS FIRST: Graphics API function pointers
#include <glad/glad.h>  
#include <GLFW/glfw3.h>

// 2. Project Headers
#include "Triangle.h"
#include "Camera.h"

// 3. UI Frameworks
#include <imgui.h>
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/backends/ImGuizmo.h"

#include <glm/glm/gtc/type_ptr.hpp>

// 4. Standard C++ Headers
#include <iostream>

// Window size constraints
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera Variables 
Camera camera(glm::vec3(0.0f, 1.0f, 2.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Mouse tracking variables
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;

// GUI & Viewport Interaction globals 
bool showGUI = true; // Kept true so your simple menu renders
int selectedIndex = -1;
ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;

// --- Helper: Basic Ray-Bounding Box Intersection ---
// Simplest way to check if user clicked close to an object center
bool RayIntersectsObject(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& objPos, float radius = 0.5f) {
    glm::vec3 oc = rayOrigin - objPos;
    float b = glm::dot(oc, rayDirection);
    float c = glm::dot(oc, oc) - radius * radius;
    if (b > 0.0f && c > 0.0f) return false;
    float discriminant = b * b - c;
    return discriminant >= 0.0f;
}

// FUNCTIONS:
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Input functions
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Direct Viewport Shortcut Controls for Gizmo Modes
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) currentOperation = ImGuizmo::TRANSLATE;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) currentOperation = ImGuizmo::ROTATE;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) currentOperation = ImGuizmo::SCALE;

    // Standard Camera controls (Hold Right-Click to move camera around freely)
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(1, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(2, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(3, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(4, deltaTime);
    }
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true; // Prevent snaps when re-engaging right-click
    }
}

// Mouse click callback for Selecting objects via Ray Casting
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return; // If clicking inside your menu box, ignore selection logic

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // If the user is actively clicking and dragging a Gizmo axis handle, do not re-select underlying objects
        if (ImGuizmo::IsOver()) return;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // 1. Normalized Device Coordinates (NDC)
        float x = (2.0f * (float)xpos) / SCR_WIDTH - 1.0f;
        float y = 1.0f - (2.0f * (float)ypos) / SCR_HEIGHT;

        // 2. Perspective Projection Inverse
        glm::mat4 projection = glm::perspective(glm::radians(static_cast<float>(camera.Zoom)), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 invProj = glm::inverse(projection);
        glm::mat4 invView = glm::inverse(view);

        glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
        glm::vec4 rayEye = invProj * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

        glm::vec3 rayWorld = glm::normalize(glm::vec3(invView * rayEye));
        glm::vec3 rayOrigin = camera.Position;

        // 3. Loop through scene list to detect close hits
        extern Triangle myTriangle; // Scoped access to scene objects
    }
}

// Mouse cursor position callback
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    // Only process camera pan updates if holding down the right mouse button
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}


// APP 
int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Window Renderer", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Custom addition: Capture mouse click actions directly
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // --- ImGui setup ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glEnable(GL_DEPTH_TEST);
    Triangle myTriangle;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // --- Start ImGui frame ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(static_cast<float>(camera.Zoom)), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
        myTriangle.Draw(view, projection, lightPos, camera.Position);

        auto& objects = myTriangle.GetSceneObjects();

        // 1. SELECT OBJECT VIA RAY CAST DETECTOR
        // This acts as your background process handler for clicks
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !ImGuizmo::IsOver() && !ImGui::GetIO().WantCaptureMouse) {
            float x = (2.0f * (float)xpos) / SCR_WIDTH - 1.0f;
            float y = 1.0f - (2.0f * (float)ypos) / SCR_HEIGHT;
            glm::mat4 invProj = glm::inverse(projection);
            glm::mat4 invView = glm::inverse(view);
            glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
            glm::vec4 rayEye = invProj * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
            glm::vec3 rayWorld = glm::normalize(glm::vec3(invView * rayEye));

            for (int i = 0; i < (int)objects.size(); i++) {
                if (RayIntersectsObject(camera.Position, rayWorld, objects[i].position)) {
                    selectedIndex = i;
                    break;
                }
            }
        }

        // 2. MINIMAL IMGUI MENU PANEL
        if (showGUI) {
            ImGui::Begin("MENU", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Viewport Controls Active:");
            ImGui::BulletText("Left-Click an object to select.");
            ImGui::BulletText("Press W: Translate | E: Rotate | R: Scale");
            ImGui::BulletText("Hold Right-Click: Look around with mouse");
            ImGui::End();
        }

        // 3. OVERLAY DIRECT VIEWPORT MANIPULATION GIZMO
        if (selectedIndex >= 0 && selectedIndex < (int)objects.size()) {
            auto& obj = objects[selectedIndex];
           
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetRect(0, 0, SCR_WIDTH, SCR_HEIGHT);

            ImGuizmo::Manipulate(
                glm::value_ptr(view),
                glm::value_ptr(projection),
                currentOperation,
                ImGuizmo::LOCAL,
                glm::value_ptr(obj.transformMatrix)
            );

            if (ImGuizmo::IsUsing()) {
                float matrixTranslation[3], matrixRotation[3], matrixScale[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(obj.transformMatrix), matrixTranslation, matrixRotation, matrixScale);

                obj.position = glm::vec3(matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]);
                obj.rotation = glm::vec3(matrixRotation[0], matrixRotation[1], matrixRotation[2]);
                obj.scale = glm::vec3(matrixScale[0], matrixScale[1], matrixScale[2]);
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
             