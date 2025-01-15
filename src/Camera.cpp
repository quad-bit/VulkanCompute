#include "Camera.h"

Camera::Camera(const Transform& transform, float aspectRatio, glm::vec3 up, float yaw, float pitch,
    CameraType projectionType) : movementSpeed(SPEED), mouseSensitivity(SENSITIVTY), fov(FOV)
{
    this->transform = transform;
    this->worldUp = up;
    this->yaw = yaw;
    this->pitch = pitch;
    this->aspect = aspectRatio;
    this->projectionType = projectionType;
    //this->updateCameraVectors();
}

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
const glm::mat4& Camera::GetViewMatrix()
{
    auto matrix = transform.GetGlobalModelMatrix();
    glm::vec3 camRight = glm::normalize(glm::vec3(matrix[0][0], matrix[0][1], matrix[0][2]));
    glm::vec3 camUp = glm::normalize(glm::vec3(matrix[1][0], matrix[1][1], matrix[1][2]));
    glm::vec3 camFront = glm::normalize(glm::vec3(matrix[2][0], matrix[2][1], matrix[2][2]));

    viewMat = glm::lookAt(transform.GetGlobalPosition(), transform.GetGlobalPosition() + camFront, camUp);

    return viewMat;
}

void Camera::UpdateLocalAxes()
{
    glm::vec3 front;
    front.x = cos(glm::radians(transform.GetLocalEulerAngles().y)) * cos(glm::radians(transform.GetLocalEulerAngles().x));
    front.y = sin(glm::radians(transform.GetLocalEulerAngles().x));
    front.z = sin(glm::radians(transform.GetLocalEulerAngles().y)) * cos(glm::radians(transform.GetLocalEulerAngles().x));
    glm::vec3 camFront = glm::normalize(front);
    // also re-calculate the Right and Up vector
    glm::vec3 camRight = glm::normalize(glm::cross(camFront, worldUp));
    glm::vec3 camUp = glm::normalize(glm::cross(camRight, camFront));
}

const glm::mat4& Camera::GetProjectionMat()
{
    switch (projectionType)
    {
    case CameraType::ORTHOGONAL:
        //ASSERT_MSG_DEBUG(0, "Need the correct design");
        //projectionMat = glm::ortho( (glm::radians(this->fov), this->aspect, this->zNear, this->zFar);

        break;

    case CameraType::PERSPECTIVE:
        projectionMat = glm::perspective(glm::radians(this->fov), this->aspect, this->zNear, this->zFar);
        break;
    }
    return projectionMat;
}

const float& Camera::GetFOV() const 
{
    return this->fov;
}

const float&  Camera::GetFar() const 
{
    return zFar;
}

const float& Camera::GetNear() const 
{
    return zNear;
}

void Camera::SetFOV(const float & fov)
{
    this->fov = fov;
}

void Camera::SetNearPlane(const float & near)
{
    this->zNear = near;
}

void Camera::SetFarPlane(const float & far)
{
    this->zFar = far;
}

void Camera::SetProjectionType(const CameraType & type)
{
    this->projectionType = type;
}
