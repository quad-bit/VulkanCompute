#pragma once

#include <vector>
#include "Transform.h"

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum class Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 6.0f;
const float SENSITIVTY = 0.25f;
const float FOV = 60.0f;

enum class CameraType
{
    PERSPECTIVE,
    ORTHOGONAL
};

struct SceneUniform
{
    glm::mat4 viewMat;
    glm::mat4 projectionMat;
    glm::vec3 cameraPos;
};

// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
class Camera
{
private:
    glm::vec3 worldUp;
    glm::mat4 viewMat;
    glm::mat4 projectionMat;

    // Eular Angles
    float yaw;
    float pitch;

    // Camera options
    float movementSpeed;
    float mouseSensitivity;
    float fov;
    float aspect;
    float zNear = 0.20f, zFar = 1500.0f;
    CameraType projectionType;

    void* cameraDataRaw;

    Camera() = delete;
    void UpdateLocalAxes();

public:

    Transform transform;
    // Constructor with vectors
    Camera(const Transform& transform, float aspectRatio,
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = YAW, float pitch = PITCH,
        CameraType projectionType = CameraType::PERSPECTIVE);

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    const glm::mat4& GetViewMatrix();
    const glm::mat4& GetProjectionMat();
    const glm::vec3& GetPosition() const;
    const glm::vec3& GetFront() const;
    const glm::vec3& GetUp() const;
    const glm::vec3& GetRight() const; 
    const glm::vec3& GetWorlUp() const;

    const float& GetYaw() const;
    const float& GetPitch() const;
    const float& GetMovementSpeed() const;
    const float& GetMouseSensitivity() const;
    const float& GetFOV() const;
    const float& GetFar() const;
    const float& GetNear() const;

    void SetFOV(const float& FOV);
    void SetNearPlane(const float& near);
    void SetFarPlane(const float& far);
    void SetProjectionType(const CameraType& type);
};