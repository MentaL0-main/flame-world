#include "game.hpp"
#include <exception>
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION

int main()
{
    Game game;

    try {
        game.run();
    } catch (std::exception& error_)
    {
        std::cout << error_.what() << std::endl;
        return 1;
    }

    return 0;
}