#pragma once 

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class Camera {
public:
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    float MouseSensitivity;
    float Zoom;

    // Movement Speed
    float Yaw;
    float Pitch;
    float Speed;

    // Constructor sets up default positions
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        Front = glm::vec3(0.0f, 0.0f, -1.0f);
        Speed = 2.5f;
        MouseSensitivity = 0.1f;
        Zoom = 45.0f;
        updateCameraVectors();
    }

    // Generates the view matrix using the lookAt formula
    glm::mat4 GetViewMatrix() const {
        // Parameters: (Where camera is, Where camera looks, Which way is up)
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Process keyboard inputs passed from main.cpp
    void ProcessKeyboard(int direction, float deltaTime) {
        float velocity = Speed * deltaTime;
        if (direction == 1) Position += Front * velocity; // W key: Move Forward
        if (direction == 2) Position -= Front * velocity; // S key: Move Backward
        if (direction == 3) Position -= glm::normalize(glm::cross(Front, Up)) * velocity; // A key: Strafe Left
        if (direction == 4) Position += glm::normalize(glm::cross(Front, Up)) * velocity; // D key: Strafe Right
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;
        Yaw += xoffset;
        Pitch += yoffset;
        if (constrainPitch) {
            if (Pitch > 89.0f)  Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }
        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)  Zoom = 1.0f;
        if (Zoom > 45.0f) Zoom = 45.0f;
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};