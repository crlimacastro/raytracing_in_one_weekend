#pragma once

#include <string_view>
#include "common.hpp"
#include "rtw_stb.image.hpp"
#include "perlin.hpp"

struct texture
{
    virtual ~texture() = default;
    virtual auto value(float u, float v, const vec3 &p) const -> color = 0;
};

struct solid_color : texture
{
    color albedo;

    solid_color() = default;

    static auto from_color(const color &c) -> solid_color
    {
        solid_color color;
        color.albedo = c;
        return color;
    }

    static auto from_rgb(float r, float g, float b) -> solid_color
    {
        solid_color c;
        c.albedo = {r, g, b};
        return c;
    }

    auto value(float u, float v, const vec3 &p) const -> color override { return albedo; }
};

struct checker_texture : texture
{
    float inv_scale;
    std::shared_ptr<texture> even;
    std::shared_ptr<texture> odd;

    checker_texture() = default;

    static auto from_textures(float scale, std::shared_ptr<texture> even, std::shared_ptr<texture> odd) -> std::shared_ptr<checker_texture>
    {
        auto t = std::make_shared<checker_texture>();
        t->inv_scale = 1.0 / scale;
        t->even = even;
        t->odd = odd;
        return t;
    }

    static auto from_colors(float scale, const color &c1, const color &c2) -> std::shared_ptr<checker_texture>
    {
        return from_textures(scale, std::make_shared<solid_color>(solid_color::from_color(c1)), std::make_shared<solid_color>(solid_color::from_color(c2)));
    }

    auto value(float u, float v, const vec3 &p) const -> color override
    {
        auto xInteger = int(std::floorf(inv_scale * p.x));
        auto yInteger = int(std::floorf(inv_scale * p.y));
        auto zInteger = int(std::floorf(inv_scale * p.z));

        bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

        return isEven ? even->value(u, v, p) : odd->value(u, v, p);
    }

private:
};

struct image_texture : texture
{
    std::shared_ptr<rtw_image> image;

    image_texture() = default;

    image_texture(std::shared_ptr<rtw_image> im) : image{im} {}

    static auto from_file(std::string_view filename) -> std::shared_ptr<image_texture>
    {
        auto image = std::make_shared<rtw_image>(filename.data());
        return std::make_shared<image_texture>(image);
    }

    auto value(float u, float v, const vec3 &p) const -> color override
    {
        if (image->height() <= 0)
            return color{0, 1, 1};

        u = interval{0, 1}.clamp(u);
        v = 1.0 - interval{0, 1}.clamp(v);

        auto i = int(u * image->width());
        auto j = int(v * image->height());
        auto pixel = image->pixel_data(i, j);

        auto color_scale = 1.0f / 255.0f;
        return color{color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]};
    }
};

struct noise_texture : texture
{
    perlin noise{};
    float scale{};
    float turbulence{};

    noise_texture() = default;
    noise_texture(float scale) : scale{scale} {}

    auto value(float u, float v, const vec3 &p) const -> color override
    {
        return color{.5, .5, .5} * (1 + std::sinf(scale * p.z + 10 * noise.turb(p, 7)));
    }
};
