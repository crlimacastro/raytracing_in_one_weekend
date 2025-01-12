#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "common.hpp"

class image
{
public:
    image() {}
    image(std::size_t width, std::size_t height, std::size_t channels = 3)
        : width{width}, height{height}, channels{channels}, data(width * height * channels)
    {
        data.resize(width * height * channels);
    }

    auto set_color(std::size_t x, std::size_t y, const color &c) -> void
    {
        const auto rgamma = linear_to_gamma(c.x);
        const auto ggamma = linear_to_gamma(c.y);
        const auto bgamma = linear_to_gamma(c.z);

        static const auto intensity = interval{0.f, 0.999f};
        const auto rbyte = static_cast<std::uint8_t>(256.f * intensity.clamp(rgamma));
        const auto gbyte = static_cast<std::uint8_t>(256.f * intensity.clamp(ggamma));
        const auto bbyte = static_cast<std::uint8_t>(256.f * intensity.clamp(bgamma));

        data[(y * width + x) * channels + 0] = rbyte;
        data[(y * width + x) * channels + 1] = gbyte;
        data[(y * width + x) * channels + 2] = bbyte;
    }

    auto write(std::filesystem::path path) const -> void;

private:
    std::size_t width = 1;
    std::size_t height = 1;
    std::size_t channels = 3;
    std::vector<std::uint8_t> data;
};
