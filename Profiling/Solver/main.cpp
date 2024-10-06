#include "string.hpp"
#include "array.hpp"

#include <cstdio>
#include <cmath>

struct IJsonElement {
    // @TODO
};

struct JsonField {
    HpString name;
    IJsonElement *val;
};

class JsonObject : IJsonElement {
    DynArray<JsonField> fields;

    // @TODO
};

class JsonArray : IJsonElement {
    DynArray<IJsonElement *> m_elements;

    // @TODO
};

class JsonString : IJsonElement {
    HpString m_content;

    // @TODO
};

class JsonNumber : IJsonElement {
    long double m_val;

    // @TODO
};

class JsonBool : IJsonElement {
    bool m_val;

    // @TODO
};

class JsonNull : IJsonElement {
    // @TODO
};

// @TODO: verify
static float reference_haversine_dist(float x0, float y0, float x1, float y1)
{
    constexpr float c_pi = 3.14159265359f;

    auto deg2rad = [](float deg) { return deg * c_pi / 180.f; };
    auto rad2deg = [](float rad) { return rad * 180.f / c_pi; };

    auto haversine = [](float rad) { return (1.f - cosf(rad)) * 0.5f; };

    float dx = deg2rad(fabsf(x0 - x1));
    float dy = deg2rad(fabsf(y0 - y1));
    float y0r = deg2rad(y0);
    float y1r = deg2rad(y1);

    float hav_of_diff =
        haversine(dy) + cosf(y0r)*cosf(y1r)*haversine(dx);

    float rad_of_diff = acosf(1.f - 2.f*hav_of_diff);
    return rad2deg(rad_of_diff);
}

int main(int argc, char **argv)
{
    printf("TODO: parse input json!\n");
    printf("TODO: Calc haversine!\n");
    printf("TODO: Make it not dogshit slow!\n");
    return 0;
}
