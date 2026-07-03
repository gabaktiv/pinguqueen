//
// Created by gabriel on 7/3/26.
//
#include "../global.hpp"
#pragma once

namespace pinguqueen::core
{
    class Visualiser {



    public:
        Visualiser() = default;
        ~Visualiser();
        Visualiser(const Visualiser&) = delete;
        Visualiser& operator=(const Visualiser&) = delete;
        Visualiser(Visualiser&&) = default;
        Visualiser& operator=(Visualiser&&) = delete;



    };
}

