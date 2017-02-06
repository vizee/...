#include "common.h"
#include <cmath>

float sind(float d)
{
	constexpr auto c = 3.14159265358979323846 / 180;
	return std::sin(d * c);
}

float cosd(float d)
{
	constexpr auto c = 3.14159265358979323846 / 180;
	return std::cos(d * c);
}
