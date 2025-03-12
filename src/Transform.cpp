#include "Transform.h"

void Transform::Init()
{
    up = glm::vec3(0, 1, 0);
    right = glm::vec3(1, 0, 0);
    forward = glm::vec3(0, 0, -1);
    parent = nullptr;

    localModelMatrix = glm::identity<glm::mat4>();
    UpdateLocalParams();

    globalPosition = glm::vec3(0, 0, 0);
    globalScale = glm::vec3(1, 1, 1);
    globalEulerAngle = glm::vec3(0, 0, 0);
    globalModelMatrix = glm::identity<glm::mat4>();
    UpdateGlobalParams();
}

Transform::Transform()
{
    localPosition = glm::vec3(0, 0, 0);
    localScale = glm::vec3(1, 1, 1);
    localEulerAngle = glm::vec3(0, 0, 0);

    Init();
}

Transform::Transform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    :localPosition(position), localEulerAngle(rotation), localScale(scale)
{
    Init();
}

const Transform& Transform::GetParent()
{
    return *parent;
}

void Transform::SetParent(Transform * transform)
{
    parent = transform;
}

void Transform::UpdateGlobalParams()
{
    GetGlobalPosition();
    this->translationMat = glm::translate(this->globalPosition);
    this->scaleMat = glm::scale(this->globalScale);

    glm::mat4 rotXMat = glm::rotate(this->globalEulerAngle.x, glm::vec3(1, 0, 0));
    glm::mat4 rotYMat = glm::rotate(this->globalEulerAngle.y, glm::vec3(0, 1, 0));
    glm::mat4 rotZMat = glm::rotate(this->globalEulerAngle.z, glm::vec3(0, 0, 1));

    this->rotationMat = rotZMat * rotYMat * rotXMat;
    this->globalModelMatrix = this->translationMat * this->rotationMat * this->scaleMat;
}

void Transform::UpdateLocalParams()
{
    this->translationMat = glm::translate(this->localPosition);
    this->scaleMat = glm::scale(this->localScale);

    glm::mat4 rotXMat = glm::rotate(this->localEulerAngle.x, glm::vec3(1, 0, 0));
    glm::mat4 rotYMat = glm::rotate(this->localEulerAngle.y, glm::vec3(0, 1, 0));
    glm::mat4 rotZMat = glm::rotate(this->localEulerAngle.z, glm::vec3(0, 0, 1));

    this->rotationMat = rotZMat * rotYMat * rotXMat;

    glm::vec3 globalForward = glm::vec3(0, 0, -1);
    glm::vec4 temp = this->rotationMat * Vec3ToVec4_0(globalForward);
    //temp = glm::normalize(temp);
    forward = glm::normalize(Vec4ToVec3(temp));

    glm::vec3 globalRight = glm::vec3(1, 0, 0);
    temp = rotationMat * Vec3ToVec4_0(globalRight);
    temp = glm::normalize(temp);
    right = Vec4ToVec3(temp);

    glm::vec3 globalUp = glm::vec3(0, 1, 0);
    temp = rotationMat * Vec3ToVec4_0(globalUp);
    temp = glm::normalize(temp);
    up = Vec4ToVec3(temp);

    this->localModelMatrix = this->translationMat * this->rotationMat * this->scaleMat;
}

const std::vector<Transform*>& Transform::GetChildren()
{
    return childrenList;
}

glm::vec3 Transform::GetForward()
{
    return ( forward );
}

glm::vec3 Transform::GetUp()
{
    return up;
}

glm::vec3 Transform::GetRight()
{
    return right;
}

glm::vec3 Transform::GetLocalPosition()
{
    return localPosition;
}

glm::vec3 Transform::GetLocalEulerAngles()
{
    return localEulerAngle;
}

glm::vec3 Transform::GetLocalScale()
{
    return localScale;
}

glm::mat4 Transform::GetLocalModelMatrix()
{
    return localModelMatrix;
}

glm::vec3 Transform::GetGlobalPosition()
{
    //ASSERT_MSG_DEBUG(0, "Yet to be implemented");
    //not sure.. this is not required as using the localPosition localModelMat gets derived
    
    glm::vec3 origin = glm::vec3(0, 0, 0);
    glm::vec4 temp = localModelMatrix * Vec3ToVec4_1(origin);
    glm::vec3 position = Vec4ToVec3(temp);

    Transform * parentTransform = parent;
    glm::mat4 globalTransform = glm::identity<glm::mat4>();

    while (parentTransform != nullptr)
    {
        glm::vec4 temp = parentTransform->GetLocalModelMatrix() * Vec3ToVec4_1(position);
        position = Vec4ToVec3(temp);

        parentTransform = parentTransform->parent;
    }

    globalPosition = position;

    return globalPosition;
}

glm::vec3 Transform::GetGlobalEulerAngles()
{
    glm::vec3 zero = glm::vec3(0, 0, 0);
    glm::vec4 temp0 = localModelMatrix * Vec3ToVec4_0(zero);
    glm::vec3 angle = Vec4ToVec3(temp0);

    Transform* parentTransform = parent;

    while (parentTransform != nullptr)
    {
        glm::vec4 temp = parentTransform->GetLocalModelMatrix() * Vec3ToVec4_0(angle);
        angle = Vec4ToVec3(temp);

        parentTransform = parentTransform->parent;
    }

    globalEulerAngle = angle;

    return globalEulerAngle;
}

glm::vec3 Transform::GetGlobalScale()
{
    glm::vec3 one = glm::vec3(1, 1, 1);
    glm::vec4 temp0 = localModelMatrix * Vec3ToVec4_0(one);
    glm::vec3 scale = Vec4ToVec3(temp0);

    Transform* parentTransform = parent;

    while (parentTransform != nullptr)
    {
        glm::vec4 temp = parentTransform->GetLocalModelMatrix() * Vec3ToVec4_0(scale);
        scale = Vec4ToVec3(temp);

        parentTransform = parentTransform->parent;
    }

    globalScale = scale;

    return globalScale;
}

const glm::mat4& Transform::GetGlobalModelMatrix()
{
    globalModelMatrix = localModelMatrix;

    Transform* parentTransform = parent;
    while (parentTransform != nullptr)
    {
        globalModelMatrix = parentTransform->GetLocalModelMatrix() * globalModelMatrix;
        parentTransform = parentTransform->parent;
    }

    return globalModelMatrix;
}

void Transform::SetLocalPosition(const glm::vec3 & pos)
{
    localPosition = pos;
    UpdateLocalParams();
    UpdateGlobalParams();
}

void Transform::SetLocalEulerAngles(const glm::vec3 & angle)
{
    localEulerAngle = angle;
    UpdateLocalParams();
    UpdateGlobalParams();
}

void Transform::SetLocalScale(const glm::vec3 & scale)
{
    localScale = scale;
    UpdateLocalParams();
    UpdateGlobalParams();
}

void Transform::SetLocalModelMatrix(const glm::mat4 & mat)
{
    localModelMatrix = mat;
}
