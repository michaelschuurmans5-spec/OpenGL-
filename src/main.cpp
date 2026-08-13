// 1. ALWAYS FIRST: Graphics API function pointers
#include <glad/glad.h>  
#include <GLFW/glfw3.h>

// 2. Project Headers
#include "Triangle.h"
#include "Camera.h"
#include "TerrainParams.h"

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
#include <cstring>
#include <cstdio>

// Window size constraints
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// ---------------------------------------------------------
// Objects > Shapes drag-and-drop / properties panel state
// (Left untouched: Objects > Models is reserved for imported meshes.)
// ---------------------------------------------------------
struct PendingShapeConfig {
    ObjectType type = ObjectType::Cube;
    char nameBuffer[64] = "Cube";
    glm::vec3 baseColor = glm::vec3(1.0f, 0.5f, 0.31f);
    int textureSlotIndex = 0; // 0=None, 1=Container, 2=Grass
};

static PendingShapeConfig g_pendingShape;
static int g_expandedShapeIndex = -1; // which shape's properties panel is open, -1 = none
static bool g_isDraggingShape = false; // true while a shape item is being dragged toward the viewport

// ---------------------------------------------------------
// Viewport Manager (right-hand outliner) state
// ---------------------------------------------------------
static bool g_outlinerOpen = true;      // arrow drop-down: collapse/expand the panel
static int  g_renamingIndex = -1;       // index of the object currently being renamed
static char g_renameBuffer[64] = "";    // scratch buffer for the rename input
static bool g_focusRequest = false;     // set by the outliner "Focus" button (same as pressing F)
static int  g_pendingDeleteId = -1;  // Delete
static bool g_openDeletePopup = false;

// ---------------------------------------------------------
// Level Designer > Terrain Generator panel state
// ---------------------------------------------------------
static TerrainParams g_terrainParams;
static bool g_terrainDirty = true;            // true when sliders changed since last preview regen
static bool g_openTerrainConfirmPopup = false;
static bool g_terrainPanelWasOpen = false;    // regen the preview the first time the panel is (re)opened

static void SetBuffer(char* buf, size_t bufSize, const std::string& value) {
    strncpy(buf, value.c_str(), bufSize - 1);
    buf[bufSize - 1] = '\0';
}

static TextureSlot ToTextureSlot(int index) {
    switch (index) {
    case 1: return TextureSlot::Container;
    case 2: return TextureSlot::Grass;
    default: return TextureSlot::None;
    }
}

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

// --- Helper: frame the camera on a world position (shared by the F hotkey
// and the Viewport Manager's focus button) ---
void FocusCameraOnPoint(const glm::vec3& targetPos) {
    camera.Position = targetPos + glm::vec3(0.0f, 0.0f, 4.0f);

    glm::vec3 direction = glm::normalize(targetPos - camera.Position);
    camera.Pitch = glm::degrees(asin(direction.y));
    camera.Yaw = glm::degrees(atan2(direction.z, direction.x));
    camera.updateCameraVectors();
}

// --- Helper: index of the object that currently holds the selection lock,
// or -1 when nothing is locked. While an object is locked it is the only
// object that can be picked or manipulated in the viewport. ---
int FindLockedIndex(const std::vector<GameObject>& objects) {
    for (int i = 0; i < (int)objects.size(); i++) {
        if (objects[i].locked) return i;
    }
    return -1;
}

// --- Helper: Basic Ray-Bounding Box Intersection ---
bool RayIntersectsObject(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& objPos, float radius = 0.5f) {
    glm::vec3 oc = rayOrigin - objPos;
    float b = glm::dot(oc, rayDirection);
    float c = glm::dot(oc, oc) - radius * radius;
    if (b > 0.0f && c > 0.0f) return false;
    float discriminant = b * b - c;
    return discriminant >= 0.0f;
}

// Casts a ray from the camera through a screen-space point and intersects it
// with the y = 0 ground plane, so a shape dropped into the viewport lands
// roughly where the cursor is pointing. Falls back to a spot in front of the
// camera if the ray never reaches the ground (e.g. looking straight up).
glm::vec3 ScreenPosToGroundPoint(double mouseX, double mouseY, const Camera& cam,
    const glm::mat4& view, const glm::mat4& projection, unsigned int screenWidth, unsigned int screenHeight) {

    float x = (2.0f * (float)mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * (float)mouseY) / screenHeight;

    glm::mat4 invProj = glm::inverse(projection);
    glm::mat4 invView = glm::inverse(view);

    glm::vec4 rayClip(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = invProj * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(invView * rayEye));

    glm::vec3 rayOrigin = cam.Position;

    if (fabs(rayWorld.y) > 0.0001f) {
        float t = -rayOrigin.y / rayWorld.y;
        if (t > 0.0f) {
            return rayOrigin + rayWorld * t;
        }
    }

    // Fallback: drop it a fixed distance in front of the camera
    return rayOrigin + cam.Front * 3.0f;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

// Input functions
// Input functions
void processInput(GLFWwindow* window, const glm::vec3* selectedTarget)
{
    // ---------------------------------------------------------
    // ESCAPE - Close application
    // ---------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }


    // ---------------------------------------------------------
    // F1 - Toggle/maximise window
    // ---------------------------------------------------------
    static bool f1PressedLastFrame = false;

    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
    {
        if (!f1PressedLastFrame)
        {
            if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
            {
                glfwRestoreWindow(window);
                glfwSetWindowSize(window, 800, 600);
            }
            else
            {
                glfwMaximizeWindow(window);
            }

            f1PressedLastFrame = true;
        }
    }
    else
    {
        f1PressedLastFrame = false;
    }


    // ---------------------------------------------------------
    // ImGuizmo operation shortcuts
    // ---------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        currentOperation = ImGuizmo::TRANSLATE;

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        currentOperation = ImGuizmo::ROTATE;

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        currentOperation = ImGuizmo::SCALE;


    // ---------------------------------------------------------
    // RIGHT MOUSE BUTTON - Normal camera movement
    // ---------------------------------------------------------
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Move forward
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            camera.ProcessKeyboard(1, deltaTime);
        }

        // Move backward
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera.ProcessKeyboard(2, deltaTime);
        }

        // A/D - normal camera strafing
        // Only when Left Mouse is NOT being held.
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
        {
            // Move left
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(3, deltaTime);
            }

            // Move right
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            {
                camera.ProcessKeyboard(4, deltaTime);
            }
        }
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
    }


    // ---------------------------------------------------------
    // LEFT MOUSE + A/D - Orbit around selected object
    // ---------------------------------------------------------
    if (selectedTarget != nullptr &&
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        constexpr float orbitSpeed = glm::radians(90.0f);

        // Orbit left
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera.OrbitAroundTarget(
                *selectedTarget,
                orbitSpeed * deltaTime
            );
        }

        // Orbit right
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera.OrbitAroundTarget(
                *selectedTarget,
                -orbitSpeed * deltaTime
            );
        }
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

        auto& objects = myTriangle.GetSceneObjects();

        glm::vec3* selectedTarget = nullptr;

        if (selectedIndex >= 0 &&
            selectedIndex < static_cast<int>(objects.size()))
        {
            selectedTarget = &objects[selectedIndex].position;
        }

        processInput(window, selectedTarget);


        // --- INLINE HOTKEY 'F' FOCUS HANDLER ---
        // A locked object owns the selection: force it and ignore everything else.
        int lockedIndex = FindLockedIndex(objects);
        if (lockedIndex >= 0) {
            selectedIndex = lockedIndex;
        }

        static bool fPressedLastFrame = false;
        bool fKeyDown = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) && !ImGui::GetIO().WantTextInput;

        if ((fKeyDown && !fPressedLastFrame) || g_focusRequest) {
            if (selectedIndex < 0 || selectedIndex >= (int)objects.size()) {
                camera.Position = glm::vec3(0.0f, 0.0f, 3.0f);
                camera.Yaw = -90.0f;
                camera.Pitch = 0.0f;
                camera.updateCameraVectors();
            }
            else {
                FocusCameraOnPoint(objects[selectedIndex].position);
            }
            g_focusRequest = false;
        }
        fPressedLastFrame = fKeyDown;

        // --- INLINE HOTKEY 'X' DELETE HANDLER ---
        static bool xPressedLastFrame = false;
        bool xKeyDown = (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) && !ImGui::GetIO().WantTextInput;
        if (xKeyDown && !xPressedLastFrame &&
            selectedIndex >= 0 && selectedIndex < (int)objects.size())
        {
            g_pendingDeleteId = objects[selectedIndex].id;
            g_openDeletePopup = true;
        }
        xPressedLastFrame = xKeyDown;


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
                // Locked: only the locked object may be picked.
                if (lockedIndex >= 0 && i != lockedIndex) continue;
                // Hidden objects are not pickable.
                if (!objects[i].visible) continue;

                if (RayIntersectsObject(camera.Position, rayWorld, objects[i].position)) {
                    selectedIndex = i;
                    break;
                }
            }
        }

        // 1b. VIEWPORT DRAG-DROP TARGET (drop a shape from Objects > Shapes here)
        // Only exists while an actual drag is in progress - otherwise this
        // full-screen window would sit over the whole viewport every frame
        // and steal the clicks that selection/gizmo/orbit rely on.
        if (g_isDraggingShape)
        {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2((float)SCR_WIDTH, (float)SCR_HEIGHT), ImGuiCond_Always);

            ImGuiWindowFlags dropZoneFlags =
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                ImGuiWindowFlags_NoDecoration;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            if (ImGui::Begin("##ViewportDropZone", nullptr, dropZoneFlags)) {
                ImGui::InvisibleButton("##ViewportDropZoneArea", ImGui::GetWindowSize());

                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SHAPE_TYPE")) {
                        ObjectType droppedType = *(const ObjectType*)payload->Data;
                        ImVec2 dropMousePos = ImGui::GetIO().MousePos;

                        glm::vec3 dropPos = ScreenPosToGroundPoint(
                            dropMousePos.x, dropMousePos.y,
                            camera, view, projection, SCR_WIDTH, SCR_HEIGHT);

                        myTriangle.SpawnShape(
                            droppedType,
                            dropPos,
                            "", // auto-named
                            glm::vec3(1.0f, 0.5f, 0.31f),
                            TextureSlot::None);
                    }
                    ImGui::EndDragDropTarget();
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        // 2. IMGUI MENU PANEL
        g_isDraggingShape = false; // re-armed below only while a shape item is actively being dragged
        if (showGUI)
        {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);

            ImGuiWindowFlags windowFlags =
                ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_AlwaysAutoResize
                | ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoScrollbar;

            ImGui::PushStyleColor(
                ImGuiCol_WindowBg,
                ImVec4(0.1f, 0.1f, 0.1f, 0.85f)
            );

            ImGui::PushStyleVar(
                ImGuiStyleVar_WindowPadding,
                ImVec2(8.0f, 6.0f)
            );

            if (ImGui::Begin("##PersistentMenu", nullptr, windowFlags))
            {
                // ---------------------------------------------------------
                // ROOT MENU
                // ---------------------------------------------------------
                if (ImGui::TreeNodeEx(
                    "MENU",
                    ImGuiTreeNodeFlags_DefaultOpen |
                    ImGuiTreeNodeFlags_SpanAvailWidth))
                {
                    ImGui::Separator();

                    // =====================================================
                    // CONTROLS
                    // =====================================================
                    if (ImGui::TreeNodeEx(
                        "Controls",
                        ImGuiTreeNodeFlags_SpanAvailWidth))
                    {
                        ImGui::Indent();

                        ImGui::TextDisabled("Viewport Controls Active:");
                        ImGui::BulletText("Left-Click an object to select.");
                        ImGui::BulletText("Press W: Translate | E: Rotate | R: Scale");
                        ImGui::BulletText("Hold Right-Click: Look around with mouse");
                        ImGui::BulletText("Orbit: Hold Down left mouse A & D");
                        ImGui::BulletText("Scroll Wheel: Zoom Camera In & Out");
                        ImGui::BulletText("ESC: Exit Window | F1: Crop Window Layout");

                        ImGui::BulletText("FOCUS:F");

                        ImGui::BulletText("FOCUS: F");

                        ImGui::Unindent();

                        ImGui::TreePop();
                    }


                    // =====================================================
                    // OBJECTS
                    // =====================================================
                    if (ImGui::TreeNodeEx(
                        "Objects",
                        ImGuiTreeNodeFlags_SpanAvailWidth))
                    {
                        ImGui::Indent();

                        // -------------------------------------------------
                        // SHAPES
                        // -------------------------------------------------
                        if (ImGui::TreeNodeEx(
                            "Shapes",
                            ImGuiTreeNodeFlags_SpanAvailWidth))
                        {
                            ImGui::Indent();

                            static const char* shapeNames[] = { "Cube", "Sphere", "Plane", "Cylinder", "Prism" };
                            static const ObjectType shapeTypes[] = {
                                ObjectType::Cube, ObjectType::Sphere, ObjectType::Plane,
                                ObjectType::Cylinder, ObjectType::Prism
                            };
                            const int shapeCount = IM_ARRAYSIZE(shapeNames);

                            bool draggingShapeThisFrame = false;

                            for (int i = 0; i < shapeCount; i++)
                            {
                                ImGui::PushID(i);

                                bool isOpen = (g_expandedShapeIndex == i);
                                if (ImGui::Selectable(shapeNames[i], isOpen))
                                {
                                    if (isOpen)
                                    {
                                        g_expandedShapeIndex = -1;
                                    }
                                    else
                                    {
                                        g_expandedShapeIndex = i;
                                        g_pendingShape.type = shapeTypes[i];
                                        SetBuffer(g_pendingShape.nameBuffer, sizeof(g_pendingShape.nameBuffer), shapeNames[i]);
                                        g_pendingShape.baseColor = glm::vec3(1.0f, 0.5f, 0.31f);
                                        g_pendingShape.textureSlotIndex = 0;
                                    }
                                }

                                // DRAG SOURCE: lets you drag this shape straight into the viewport
                                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                                {
                                    draggingShapeThisFrame = true;
                                    ImGui::SetDragDropPayload("SHAPE_TYPE", &shapeTypes[i], sizeof(ObjectType));
                                    ImGui::Text("Place %s", shapeNames[i]);
                                    ImGui::EndDragDropSource();
                                }

                                // DETAILS DROPDOWN: name, mesh type, base color, texture slot, Create
                                if (g_expandedShapeIndex == i)
                                {
                                    ImGui::Indent();
                                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.2f));
                                    ImGui::BeginChild(
                                        "##ShapeDetails",
                                        ImVec2(320.0f, 220.0f),
                                        true,
                                        ImGuiWindowFlags_HorizontalScrollbar);

                                    ImGui::TextDisabled("Mesh Type: Basic Shape (%s)", shapeNames[i]);
                                    ImGui::Spacing();

                                    ImGui::SetNextItemWidth(160.0f);
                                    ImGui::InputText("Name", g_pendingShape.nameBuffer, IM_ARRAYSIZE(g_pendingShape.nameBuffer));

                                    ImGui::ColorEdit3("Base Color", &g_pendingShape.baseColor.x);

                                    const char* textureOptions[] = { "None", "Container", "Grass" };
                                    ImGui::SetNextItemWidth(160.0f);
                                    ImGui::Combo("Texture Slot", &g_pendingShape.textureSlotIndex, textureOptions, IM_ARRAYSIZE(textureOptions));

                                    ImGui::Spacing();
                                    // More configuration options (roughness, material presets, etc.) can be added here later.

                                    if (ImGui::Button("Create", ImVec2(120.0f, 0.0f)))
                                    {
                                        glm::vec3 spawnPos = camera.Position + camera.Front * 3.0f;
                                        std::string spawnName = g_pendingShape.nameBuffer;

                                        myTriangle.SpawnShape(
                                            g_pendingShape.type,
                                            spawnPos,
                                            spawnName,
                                            g_pendingShape.baseColor,
                                            ToTextureSlot(g_pendingShape.textureSlotIndex));
                                    }

                                    ImGui::EndChild();
                                    ImGui::PopStyleColor();
                                    ImGui::Unindent();
                                }

                                ImGui::PopID();
                            }

                            g_isDraggingShape = draggingShapeThisFrame;

                            ImGui::Unindent();
                            ImGui::TreePop();
                        }


                        // -------------------------------------------------
                        // MODELS
                        // -------------------------------------------------
                        if (ImGui::TreeNodeEx(
                            "Models",
                            ImGuiTreeNodeFlags_SpanAvailWidth))
                        {
                            ImGui::Indent();

                            ImGui::BulletText("Load Model");
                            ImGui::BulletText("Imported Models");

                            ImGui::Unindent();
                            ImGui::TreePop();
                        }

                        ImGui::Unindent();
                        ImGui::TreePop();
                    }

                    // LIGHTING & ATMOSPHERE (NEW)
                    if (ImGui::TreeNodeEx(
                        "Lighting & Atmosphere",
                        ImGuiTreeNodeFlags_SpanAvailWidth))
                    {
                        ImGui::Indent();

                        ImGui::TextDisabled("Sun Direction");
                        ImGui::SliderFloat("Azimuth", &myTriangle.lightSettings.sunAzimuth, 0.0f, 360.0f, "%.1f deg");
                        ImGui::SliderFloat("Elevation", &myTriangle.lightSettings.sunElevation, 2.0f, 89.0f, "%.1f deg");

                        ImGui::Spacing();
                        ImGui::TextDisabled("Light Properties");
                        ImGui::ColorEdit3("Sun Color", &myTriangle.lightSettings.sunColor.x);
                        ImGui::SliderFloat("Intensity", &myTriangle.lightSettings.sunIntensity, 0.0f, 3.0f, "%.2f");
                        ImGui::SliderFloat("Ambient Light", &myTriangle.lightSettings.ambientIntensity, 0.0f, 1.0f, "%.2f");

                        ImGui::Spacing();
                        if (ImGui::TreeNode("Shadow Fine-Tuning"))
                        {
                            ImGui::SliderFloat("Min Bias", &myTriangle.lightSettings.shadowBiasMin, 0.0001f, 0.005f, "%.4f");
                            ImGui::SliderFloat("Max Bias", &myTriangle.lightSettings.shadowBiasMax, 0.001f, 0.02f, "%.4f");
                            ImGui::Checkbox("Debug Cascade Split Colors", &myTriangle.lightSettings.debugCascades);
                            ImGui::TreePop();
                        }

                        ImGui::Unindent();
                        ImGui::TreePop();
                    }

                    // =====================================================
                    // LEVEL DESIGNER
                    // =====================================================
                    if (ImGui::TreeNodeEx(
                        "Level Designer",
                        ImGuiTreeNodeFlags_SpanAvailWidth))
                    {
                        ImGui::Indent();

                        // -------------------------------------------------
                        // TERRAIN GENERATOR
                        // -------------------------------------------------
                        bool terrainNodeOpen = ImGui::TreeNodeEx(
                            "Terrain Generator",
                            ImGuiTreeNodeFlags_SpanAvailWidth);

                        if (terrainNodeOpen)
                        {
                            ImGui::Indent();

                            // Regenerate the preview whenever the panel is (re)opened
                            // so there's always something in the viewport reacting to
                            // the sliders, even before the first slider touch.
                            if (!g_terrainPanelWasOpen) {
                                g_terrainDirty = true;
                            }

                            ImGui::TextDisabled(myTriangle.IsTerrainCommitted()
                                ? "Editing the terrain already in the scene."
                                : "Adjust sliders to preview, then Generate Terrain.");
                            ImGui::Spacing();

                            ImGui::TextDisabled("Shape");
                            g_terrainDirty |= ImGui::SliderFloat("Terrain Size", &g_terrainParams.size, 5.0f, 100.0f);
                            g_terrainDirty |= ImGui::SliderInt("Resolution", &g_terrainParams.resolution, 8, 200);

                            ImGui::Spacing();
                            ImGui::TextDisabled("Features (0 = off)");
                            g_terrainDirty |= ImGui::SliderFloat("Hills", &g_terrainParams.hillScale, 0.0f, 5.0f);
                            g_terrainDirty |= ImGui::SliderFloat("Mountains", &g_terrainParams.mountainScale, 0.0f, 5.0f);
                            g_terrainDirty |= ImGui::SliderFloat("Valleys", &g_terrainParams.valleyScale, 0.0f, 5.0f);
                            g_terrainDirty |= ImGui::SliderFloat("Holes", &g_terrainParams.holeScale, 0.0f, 5.0f);
                            g_terrainDirty |= ImGui::SliderFloat("Rocks", &g_terrainParams.rockScale, 0.0f, 5.0f);

                            ImGui::Spacing();
                            g_terrainDirty |= ImGui::SliderInt("Seed", &g_terrainParams.seed, 0, 9999);

                            // Live preview: regenerate the mesh the instant any
                            // slider above moved, so the viewport updates as you drag.
                            if (g_terrainDirty) {
                                myTriangle.PreviewTerrain(g_terrainParams);
                                g_terrainDirty = false;
                            }

                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::Spacing();

                            if (ImGui::Button("Generate Terrain", ImVec2(160.0f, 0.0f))) {
                                g_openTerrainConfirmPopup = true;
                            }

                            if (myTriangle.IsTerrainCommitted()) {
                                ImGui::SameLine();
                                if (ImGui::Button("Delete Terrain", ImVec2(140.0f, 0.0f))) {
                                    myTriangle.DeleteTerrain();
                                    g_terrainParams = TerrainParams(); // reset sliders to defaults
                                    g_terrainDirty = true;
                                }
                            }

                            ImGui::Unindent();
                            ImGui::TreePop();
                        }
                        g_terrainPanelWasOpen = terrainNodeOpen;

                        ImGui::Unindent();
                        ImGui::TreePop();
                    }


                    // Close MENU
                    ImGui::TreePop();
                }

                ImGui::End();
            }

            if (g_openDeletePopup) { ImGui::OpenPopup("Delete Object?"); g_openDeletePopup = false; }

            if (ImGui::BeginPopupModal("Delete Object?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                int deleteIdx = -1;
                for (int i = 0; i < (int)objects.size(); i++)
                    if (objects[i].id == g_pendingDeleteId) { deleteIdx = i; break; }

                if (deleteIdx < 0) { g_pendingDeleteId = -1; ImGui::CloseCurrentPopup(); }
                else {
                    ImGui::Text("Are you sure you want to delete \"%s\" (#%d)?",
                        objects[deleteIdx].name.c_str(), objects[deleteIdx].id);
                    ImGui::Spacing();
                    if (ImGui::Button("Yes", ImVec2(110.0f, 0.0f))) {
                        myTriangle.OnObjectDeleted(g_pendingDeleteId); // keep terrain tracking in sync
                        objects.erase(objects.begin() + deleteIdx);
                        if (selectedIndex == deleteIdx)      selectedIndex = -1;
                        else if (selectedIndex > deleteIdx)  selectedIndex--;
                        if (g_renamingIndex == deleteIdx)      g_renamingIndex = -1;
                        else if (g_renamingIndex > deleteIdx)  g_renamingIndex--;
                        lockedIndex = FindLockedIndex(objects);
                        g_pendingDeleteId = -1;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("No", ImVec2(110.0f, 0.0f))) {
                        g_pendingDeleteId = -1; ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }

            if (g_openTerrainConfirmPopup) { ImGui::OpenPopup("Generate Terrain?"); g_openTerrainConfirmPopup = false; }

            if (ImGui::BeginPopupModal("Generate Terrain?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text(myTriangle.IsTerrainCommitted()
                    ? "Apply these changes to the terrain in the scene?"
                    : "Apply this terrain to the viewport scene?");
                ImGui::Spacing();
                if (ImGui::Button("Yes", ImVec2(110.0f, 0.0f))) {
                    myTriangle.CommitTerrain();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("No", ImVec2(110.0f, 0.0f))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }


            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }


        // 2b. VIEWPORT MANAGER (right-hand outliner)
        // Lists every object living in the viewport with its unique ID, an
        // editable name, a visibility (eye) toggle and a selection lock.
        {
            const float panelWidth = 340.0f;
            ImGui::SetNextWindowPos(ImVec2((float)SCR_WIDTH - panelWidth, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(
                ImVec2(panelWidth, g_outlinerOpen ? (float)SCR_HEIGHT : 0.0f),
                ImGuiCond_Always);

            ImGuiWindowFlags outlinerFlags =
                ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoCollapse;

            if (!g_outlinerOpen)
                outlinerFlags |= ImGuiWindowFlags_AlwaysAutoResize;

            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.09f, 0.92f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));

            if (ImGui::Begin("##ViewportManager", nullptr, outlinerFlags))
            {
                // ---- Header with the arrow drop-down ----
                if (ImGui::ArrowButton("##OutlinerToggle", g_outlinerOpen ? ImGuiDir_Right : ImGuiDir_Down))
                    g_outlinerOpen = !g_outlinerOpen;

                ImGui::SameLine();
                ImGui::Text("Viewport Manager");

                if (g_outlinerOpen)
                {
                    ImGui::Separator();

                    int lockedNow = FindLockedIndex(objects);
                    if (lockedNow >= 0) {
                        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
                            "LOCKED: %s (only this object is selectable)",
                            objects[lockedNow].name.c_str());
                        ImGui::Separator();
                    }

                    ImGui::TextDisabled("Select an item, then press F to focus it.");
                    ImGui::Spacing();

                    ImGui::BeginChild("##OutlinerList", ImVec2(0.0f, 0.0f), false);

                    for (int i = 0; i < (int)objects.size(); i++)
                    {
                        GameObject& obj = objects[i];
                        ImGui::PushID(obj.id);

                        bool interactable = (lockedNow < 0) || (i == lockedNow);

                        // ---- Eye icon: hide / unhide ----
                        if (ImGui::SmallButton(obj.visible ? "O" : "-")) {
                            obj.visible = !obj.visible;
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip(obj.visible ? "Visible (click to hide)" : "Hidden (click to show)");

                        // ---- Lock icon: restrict selection to this object ----
                        ImGui::SameLine();
                        if (ImGui::SmallButton(obj.locked ? "[L]" : "[ ]")) {
                            if (obj.locked) {
                                obj.locked = false;
                            }
                            else {
                                for (auto& other : objects) other.locked = false; // only one lock at a time
                                obj.locked = true;
                                selectedIndex = i;
                            }
                            lockedNow = FindLockedIndex(objects);
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip(obj.locked
                                ? "Selection locked to this object (click to unlock)"
                                : "Lock selection to this object");

                        ImGui::SameLine();
                        if (ImGui::SmallButton("Delete (X)")) {
                            g_pendingDeleteId = obj.id;
                            g_openDeletePopup = true;
                        }

                        // ---- Name row (double-click or the Rename button to edit) ----
                        if (g_renamingIndex == i)
                        {
                            ImGui::SetNextItemWidth(180.0f);
                            if (ImGui::InputText("##RenameField", g_renameBuffer, IM_ARRAYSIZE(g_renameBuffer),
                                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                            {
                                if (g_renameBuffer[0] != '\0')
                                    obj.name = g_renameBuffer;
                                g_renamingIndex = -1;
                            }

                            ImGui::SameLine();
                            if (ImGui::SmallButton("OK")) {
                                if (g_renameBuffer[0] != '\0')
                                    obj.name = g_renameBuffer;
                                g_renamingIndex = -1;
                            }
                        }
                        else
                        {
                            char label[128];
                            snprintf(label, sizeof(label), "#%d  %s  (%s)",
                                obj.id, obj.name.c_str(), ToString(obj.type).c_str());

                            bool isSelected = (selectedIndex == i);

                            if (!interactable)
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
                            else if (!obj.visible)
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

                            if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_AllowDoubleClick))
                            {
                                if (interactable) {
                                    selectedIndex = i;
                                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                        g_renamingIndex = i;
                                        SetBuffer(g_renameBuffer, sizeof(g_renameBuffer), obj.name);
                                    }
                                }
                            }

                            if (!interactable || !obj.visible)
                                ImGui::PopStyleColor();
                        }

                        // ---- Per-item actions for the selected row ----
                        if (selectedIndex == i && g_renamingIndex != i)
                        {
                            ImGui::Indent();
                            if (ImGui::SmallButton("Focus (F)")) {
                                g_focusRequest = true;
                            }
                            ImGui::SameLine();
                            if (ImGui::SmallButton("Rename")) {
                                g_renamingIndex = i;
                                SetBuffer(g_renameBuffer, sizeof(g_renameBuffer), obj.name);
                            }
                            ImGui::TextDisabled("pos  %.2f, %.2f, %.2f",
                                obj.position.x, obj.position.y, obj.position.z);
                            ImGui::Unindent();
                        }

                        ImGui::PopID();
                    }

                    if (objects.empty())
                        ImGui::TextDisabled("No objects in the viewport yet.");

                    ImGui::EndChild();
                }
            }
            ImGui::End();

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        // 3. OVERLAY DIRECT VIEWPORT MANIPULATION GIZMO
        if (selectedIndex >= 0 && selectedIndex < (int)objects.size() &&
            objects[selectedIndex].visible &&
            (lockedIndex < 0 || selectedIndex == lockedIndex)) {
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