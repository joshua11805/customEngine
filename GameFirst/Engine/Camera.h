//
// Created by JoshuaShin on 2/1/26.
//

#ifndef GAME_CAMERA_H
#define GAME_CAMERA_H

#include <EngineMath.h>

class Renderer;
class Camera
{
public:
    struct PerCameraConstants
    {
        Matrix4 c_viewProj;
    };

    Camera(Renderer* renderer);
    virtual ~Camera() = default;
    void SetActive(SDL_GPUCommandBuffer* commandBuffer);
    void SetWorldToCamera(Matrix4 mat) { m_worldToCamera = mat; }
    const Matrix4& returnWorldToCamera() { return m_worldToCamera; }
    void SetScreenSize(int w, int h);


private:
    PerCameraConstants m_PCC;
    Matrix4 m_worldToCamera;
    Matrix4 m_projectionMatrix;
};


#endif //GAME_CAMERA_H