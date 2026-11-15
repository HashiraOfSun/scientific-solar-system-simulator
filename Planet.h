#pragma once
#include <array>
#include <string>
#include <vector>
#include <glm/glm.hpp>

// JPL Table 1: a [AU], e [1], I, L, longitude of perihelion, node [degrees].
struct Elements { double a, e, inclination, longitude, perihelion, node; };
struct Planet {
    std::string name, texture;
    double radiusKm, massKg, rotationHours, obliquityDeg;
    glm::vec3 color;
    Elements epoch, rate; // J2000.0 and change per Julian century
};
std::vector<Planet> createPlanets();
std::vector<float> createSphere(int segments, int rings);
