#include "v2f.h"
#include "common.h"

v2f v2f::rotate(float angle)
{
    auto sina = sind(angle);
    auto cosa = cosd(angle);
    return {x * cosa - y * sina, x * sina + y * cosa};
}
