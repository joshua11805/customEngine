//
// Created by JoshuaShin on 2/1/26.
//

#ifndef GAME_TESTCUBE_H
#define GAME_TESTCUBE_H
#include "Actor.h"

class Renderer;
class Material;
class Mesh;

class TestCube : public Actor
{
public:
    TestCube(Material* material);
    ~TestCube();
private:
};


#endif //GAME_TESTCUBE_H