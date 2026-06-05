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
    // Computes (and caches) the VP matrix from the current worldToCamera + projection.
    // Call this before reading GetViewProj() outside of the render pass.
    void UpdateViewProj() { m_PCC.c_viewProj = m_worldToCamera * m_projectionMatrix; }
    const Matrix4& GetViewProj() const { return m_PCC.c_viewProj; }


private:
    PerCameraConstants m_PCC;
    Matrix4 m_worldToCamera;
    Matrix4 m_projectionMatrix;
};


#endif //GAME_CAMERA_H