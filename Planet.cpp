#include "Planet.h"
#include <cmath>

// Sources and units: docs/SCIENCE.md. Retrieved 2026-10-21.
std::vector<Planet> createPlanets() {
    return {
        {"Sun","sun.jpg",695700,1.9884e+30,609.12,7.25,{1,0.85,0.25},
         {0,0,0,0,0,0},
         {0,0,0,0,0,0}},
        {"Mercury","mercury.jpg",2439.5,3.3e+23,1407.6,0.034,{0.65,0.62,0.58},
         {0.38709927,0.20563593,7.00497902,252.2503235,77.45779628,48.33076593},
         {3.7e-07,1.906e-05,-0.00594749,149472.67411175,0.16047689,-0.12534081}},
        {"Venus","venus.jpg",6052,4.87e+24,-5832.5,177.4,{0.95,0.76,0.45},
         {0.72333566,0.00677672,3.39467605,181.9790995,131.60246718,76.67984255},
         {3.9e-06,-4.107e-05,-0.0007889,58517.81538729,0.00268329,-0.27769418}},
        {"Earth","earth.jpg",6378,5.97e+24,23.9,23.4,{0.2,0.6,1},
         {1.00000261,0.01671123,-1.531e-05,100.46457166,102.93768193,0},
         {5.62e-06,-4.392e-05,-0.01294668,35999.37244981,0.32327364,0}},
        {"Mars","mars.jpg",3396,6.42e+23,24.6,25.2,{1,0.35,0.2},
         {1.52371034,0.0933941,1.84969142,-4.55343205,-23.94362959,49.55953891},
         {1.847e-05,7.882e-05,-0.00813131,19140.30268499,0.44441088,-0.29257343}},
        {"Jupiter","jupiter.jpg",71492,1.898e+27,9.9,3.1,{0.85,0.65,0.45},
         {5.202887,0.04838624,1.30439695,34.39644051,14.72847983,100.47390909},
         {-0.00011607,-0.00013253,-0.00183714,3034.74612775,0.21252668,0.20469106}},
        {"Saturn","saturn.jpg",60268,5.68e+26,10.7,26.7,{0.9,0.8,0.55},
         {9.53667594,0.05386179,2.48599187,49.95424423,92.59887831,113.66242448},
         {-0.0012506,-0.00050991,0.00193609,1222.49362201,-0.41897216,-0.28867794}},
        {"Uranus","uranus.jpg",25559,8.68e+25,-17.2,97.8,{0.4,0.85,0.85},
         {19.18916464,0.04725744,0.77263783,313.23810451,170.9542763,74.01692503},
         {-0.00196176,-4.397e-05,-0.00242939,428.48202785,0.40805281,0.04240589}},
        {"Neptune","neptune.jpg",24764,1.02e+26,16.1,28.3,{0.3,0.45,1},
         {30.06992276,0.00859048,1.77004347,-55.12002969,44.96476227,131.78422574},
         {0.00026291,5.105e-05,0.00035372,218.45945325,-0.32241464,-0.00508664}},
    };
}

std::vector<float> createSphere(int segments, int rings) {
    std::vector<float> vertices;

    const float PI = 3.14159265359f;

    auto addVertex = [&](float theta, float phi, float u, float v) {
        float x = std::sin(theta) * std::cos(phi);
        float y = std::cos(theta);
        float z = std::sin(theta) * std::sin(phi);

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(u);
        vertices.push_back(v);
    };

    for (int i = 0; i < rings; i++) {
        float theta1 = PI * i / rings;
        float theta2 = PI * (i + 1) / rings;

        float v1 = 1.0f - static_cast<float>(i) / rings;
        float v2 = 1.0f - static_cast<float>(i + 1) / rings;

        for (int j = 0; j < segments; j++) {
            float phi1 = 2.0f * PI * j / segments;
            float phi2 = 2.0f * PI * (j + 1) / segments;

            float u1 = static_cast<float>(j) / segments;
            float u2 = static_cast<float>(j + 1) / segments;

            addVertex(theta1, phi1, u1, v1);
            addVertex(theta2, phi1, u1, v2);
            addVertex(theta2, phi2, u2, v2);

            addVertex(theta1, phi1, u1, v1);
            addVertex(theta2, phi2, u2, v2);
            addVertex(theta1, phi2, u2, v1);
        }
    }

    return vertices;
}