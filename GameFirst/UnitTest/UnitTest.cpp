//
// Created by JoshuaShin on 4/11/26.
//
#include "pch.h"
#include "Physics.h"

int main(int argc, char* argv[])
{
    SDL_Log("Unit Test Running!\n");


    bool result = Physics::UnitTest();
    SDL_assert(result);

    if (!result)
    {
        SDL_Log("Physics::UnitTest Failed!\n");
        return 1;
    }

    SDL_Log("All tests passed!\n");

    return 0;
}
