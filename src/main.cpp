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

#include <glm/gtc/type_ptr.hpp>

// 4. Standard C++ Headers
#include <iostream>
#include <string>
#include <vector>

// Window size constraints
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// Camera Variables 
Camera camera(glm::vec3(0.0f, 1.0f, 2.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Mouse tracking variables
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;

// GUI & Viewport Interaction globals 
bool showGUI = true;
int selectedIndex = -1;
ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;

// --- Helper: Basic Ray-Bounding Box Intersection ---
bool RayIntersectsObject(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& objPos, float radius = 0.5f) {
    glm::vec3 oc = rayOrigin - objPos;
    float b = glm::dot(oc, rayDirection);
    float c = glm::dot(oc, oc) - radius * radius;
    if (b > 0.0f && c > 0.0f) return false;
    float discriminant = b * b - c;
    return discriminant >= 0.0f;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

// Input functions
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    static bool f1PressedLastFrame = false;
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
        if (!f1PressedLastFrame) {
            if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED)) {
                glfwRestoreWindow(window);
                glfwSetWindowSize(window, 800, 600);
            }
            else {
                glfwMaximizeWindow(window);
            }
            f1PressedLastFrame = true;
        }
    }
    else {
        f1PressedLastFrame = false;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) currentOperation = ImGuizmo::TRANSLATE;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) currentOperation = ImGuizmo::ROTATE;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) currentOperation = ImGuizmo::SCALE;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(1, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(2, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(3, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(4, deltaTime);
    }
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (ImGuizmo::IsOver()) return;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        float x = (2.0f * (float)xpos) / SCR_WIDTH - 1.0f;
        float y = 1.0f - (2.0f * (float)ypos) / SCR_HEIGHT;

        glm::mat4 projection = glm::perspective(glm::radians(static_cast<float>(camera.Zoom)), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 invProj = glm::inverse(projection);
        glm::mat4 invView = glm::inverse(view);

        glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
        glm::vec4 rayEye = invProj * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

        glm::vec3 rayWorld = glm::normalize(glm::vec3(invView * rayEye));
    }
}

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

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

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

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiIO& io = ImGui::GetIO();
    std::string fontPath = "C:\\Windows\\Fonts\\arial.ttf";
    ImFont* mainFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 32.0f);

    glEnable(GL_DEPTH_TEST);
    Triangle myTriangle;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        auto& objects = myTriangle.GetSceneObjects();

        // --- INLINE HOTKEY 'F' FOCUS HANDLER ---
        static bool fPressedLastFrame = false;
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !ImGui::GetIO().WantTextInput) {
            if (!fPressedLastFrame) {
                if (selectedIndex < 0 || selectedIndex >= (int)objects.size()) {
                    camera.Position = glm::vec3(0.0f, 0.0f, 3.0f);
                    camera.Yaw = -90.0f;
                    camera.Pitch = 0.0f;
                    camera.updateCameraVectors();
                }
                else {
                    auto& selectedObj = objects[selectedIndex];
                    glm::vec3 targetPos = selectedObj.position;

                    camera.Position = targetPos + glm::vec3(0.0f, 0.0f, 4.0f);

                    glm::vec3 direction = glm::normalize(targetPos - camera.Position);
                    camera.Pitch = glm::degrees(asin(direction.y));
                    camera.Yaw = glm::degrees(atan2(direction.z, direction.x));
                    camera.updateCameraVectors();
                }
                fPressedLastFrame = true;
            }
        }
        else {
            fPressedLastFrame = false;
        }

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

        // 1. SELECT OBJECT VIA RAY CAST DETECTOR
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

        // 2. IMGUI MENU PANEL
        if (showGUI) {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);

            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_AlwaysAutoResize
                | ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoScrollbar;

            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.85f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));

            if (ImGui::Begin("##PersistentMenu", nullptr, windowFlags))
            {
                if (ImGui::TreeNodeEx("MENU", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
                {
                    ImGui::Separator();

                    if (ImGui::TreeNodeEx("Controls", ImGuiTreeNodeFlags_SpanAvailWidth))
                    {
                        ImGui::Indent();
                        ImGui::TextDisabled("Viewport Controls Active:");
                        ImGui::BulletText("Left-Click an object to select.");
                        ImGui::BulletText("Press W: Translate | E: Rotate | R: Scale");
                        ImGui::BulletText("Hold Right-Click: Look around with mouse");
                        ImGui::BulletText("Scroll Wheel: Zoom Camera In & Out");
                        ImGui::BulletText("ESC: Exit Window | F1: Crop Window Layout");
                        ImGui::Unindent();

                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }
                ImGui::End();
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        // 3. OVERLAY DIRECT VIEWPORT MANIPULATION GIZMO
        if (selectedIndex >= 0 && selectedIndex < (int)objects.size()) {
            auto& obj = objects[selectedIndex];

            int currentWidth, currentHeight;
            glfwGetWindowSize(window, &currentWidth, &currentHeight);

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetRect(0, 0, (float)currentWidth, (float)currentHeight);

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