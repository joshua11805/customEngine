//
// Created by JoshuaShin on 2/1/26.
//

#include "../Engine/TestCube.h"
#include "Actor.h"
#include "VertexBuffer.h"
#include "VertexFormat.h"
#include "Material.h"
#include "Mesh.h"

TestCube::TestCube(Material* material)
    : Actor(nullptr)
{
    SDL_Log("TestCube constructor, material = %p", material);

    //create the vertices and indices for a cube
    static VertexPosColorUVNormal cubeVertex[] =
    {
        //red -Z
        {Vector3(-0.5f, 0.5f, -0.5f), Color4(1.0f, 0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) }, //top left
        { Vector3( 0.5f, 0.5f, -0.5f), Color4(1.0f, 0.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) }, //top right
        { Vector3( 0.5f, -0.5f, -0.5f), Color4(1.0f, 0.0f, 0.0f, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 0.0f, -1.0f) }, //bottom right
        { Vector3(-0.5f, -0.5f, -0.5f), Color4(1.0f, 0.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 0.0f, -1.0f) }, //bottom left
        //green +Z
        { Vector3(0.5f, 0.5f, 0.5f), Color4(0.0f, 1.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) },
        { Vector3( -0.5f, 0.5f, 0.5f), Color4(0.0f, 1.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) },
        { Vector3( -0.5f, -0.5f, 0.5f), Color4(0.0f, 1.0f, 0.0f, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f) },
        { Vector3(0.5f, -0.5f, 0.5f), Color4(0.0f, 1.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f) },
        //blue -X
        { Vector3(-0.5f, -0.5f, -0.5f), Color4(0.0f, 0.0f, 1.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f) },
        { Vector3( -0.5f, -0.5f, 0.5f), Color4(0.0f, 0.0f, 1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f) },
        { Vector3( -0.5f, 0.5f, 0.5f), Color4(0.0f, 0.0f, 1.0f, 1.0f), Vector2(1.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f) },
        { Vector3(-0.5f, 0.5f, -0.5f), Color4(0.0f, 0.0f, 1.0f, 1.0f), Vector2(0.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f) },
        //yellow +X
        { Vector3(0.5f, 0.5f, -0.5f), Color4(1.0f, 1.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) },
        { Vector3( 0.5f, 0.5f, 0.5f), Color4(1.0f, 1.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) },
        { Vector3( 0.5f, -0.5f, 0.5f), Color4(1.0f, 1.0f, 0.0f, 1.0f), Vector2(1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f) },
        { Vector3(0.5f, -0.5f, -0.5f), Color4(1.0f, 1.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f) },
        //cyan -y
        { Vector3(-0.5f, -0.5f, -0.5f), Color4(0.0f, 1.0f, 1.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f) },
        { Vector3( 0.5f, -0.5f, -0.5f), Color4(0.0f, 1.0f, 1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f) },
        { Vector3( 0.5f, -0.5f, 0.5f), Color4(0.0f, 1.0f, 1.0f, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, -1.0f, 0.0f) },
        { Vector3(-0.5f, -0.5f, 0.5f), Color4(0.0f, 1.0f, 1.0f, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, -1.0f, 0.0f) },
        //purple +y
        { Vector3(-0.5f, 0.5f, 0.5f), Color4(1.0f, 0.0f, 1.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) },
        { Vector3( 0.5f, 0.5f, 0.5f), Color4(1.0f, 0.0f, 1.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) },
        { Vector3( 0.5f, 0.5f, -0.5f), Color4(1.0f, 0.0f, 1.0f, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) },
        { Vector3(-0.5f, 0.5f, -0.5f), Color4(1.0f, 0.0f, 1.0f, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)},
    };


    static uint16_t cubeIndex[] =
    {
        2, 1, 0,
        3, 2, 0,
        6, 5, 4,
        7, 6, 4,
        10, 9, 8,
        11, 10, 8,
        14, 13, 12,
        15, 14, 12,
        18, 17, 16,
        19, 18, 16,
        22, 21, 20,
        23, 22, 20,
    };


    VertexBuffer* vertexBuffer = new VertexBuffer(Renderer::Get(), (void*)cubeVertex, sizeof(cubeVertex), (void*)cubeIndex, ARRAY_SIZE(cubeIndex), sizeof(uint16_t));
    SDL_Log("TestCube vertexBuffer = %p", vertexBuffer);


    m_mesh = new Mesh(vertexBuffer, material);
    SDL_Log("TestCube m_mesh = %p", m_mesh);

}

TestCube::~TestCube()
{
    delete m_mesh;
    m_mesh = nullptr;
}

