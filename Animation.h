#pragma once
#include "Planet.h"
#include <string>

namespace astro {
constexpr double pi = 3.14159265358979323846;
constexpr double auKm = 149597870.7;
constexpr double solarGM = 132712440041.279419; // km^3/s^2, JPL DE440
constexpr double j2000 = 2451545.0;
constexpr double minJD = 2378496.5; // 1800-01-01 00:00 TDB
constexpr double maxJD = 2469807.5; // 2050-01-01 00:00 TDB (conservative bound)
double radians(double deg);
double solveKepler(double meanAnomalyRad, double eccentricity);
Elements elementsAt(const Planet& planet, double jd);
glm::dvec3 orbitPoint(const Elements& e, double eccentricAnomaly);
glm::dvec3 positionAU(const Planet& planet, double jd);
glm::dvec3 toScene(const glm::dvec3& ecliptic); // (x,y,z) -> (x,z,-y), right handed
glm::dmat4 axisOrientation(const Planet& planet);
double spinAngle(const Planet& planet, double jd);
double speedKmS(const Planet& planet, double jd); // instantaneous two-body vis-viva
double periodDays(const Planet& planet, double jd);
double julianDate(int year, int month, int day);
std::string calendarDate(double jd);
struct SimulationClock {
    double jd = j2000;
    double daysPerSecond = 10;
    bool paused = true;
    void advance(double realSeconds);
    void step(double days);
    void setDate(double date);
};
struct DisplayScale {
    double unitsPerAU = 12;
    double radiusExaggeration = 250;
    bool trueScale = false;
    double radius(const Planet& p) const;
    glm::dvec3 position(const Planet& p, double jd) const;
};
}
