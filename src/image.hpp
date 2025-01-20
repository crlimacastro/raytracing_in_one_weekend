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
        : m_width{width}, m_height{height}, channels{channels}, data(width * height * channels)
    {
        data.resize(width * height * channels);
    }

    auto set_color(std::size_t x, std::size_t y, const color &c) -> void
    {
        auto r = c.x;
        auto g = c.y;
        auto b = c.z;

        if (is_nan(r)) r = 0.f;
        if (is_nan(g)) g = 0.f;
        if (is_nan(b)) b = 0.f;

        const auto rgamma = linear_to_gamma(r);
        const auto ggamma = linear_to_gamma(g);
        const auto bgamma = linear_to_gamma(b);

        static const auto intensity = interval{0.f, 0.999f};
        const auto rbyte = static_cast<std::uint8_t>(256.f * intensity.clamp(rgamma));
        const auto gbyte = static_cast<std::uint8_t>(256.f * intensity.clamp(ggamma));
        const auto bbyte = static_cast<std::uint8_t>(256.f * intensity.clamp(bgamma));

        data[(y * m_width + x) * channels + 0] = rbyte;
        data[(y * m_width + x) * channels + 1] = gbyte;
        data[(y * m_width + x) * channels + 2] = bbyte;
    }

    auto write(std::filesystem::path path) const -> void;

    auto width() const -> std::size_t { return m_width; }
    auto height() const -> std::size_t { return m_height; }
    
private:
    std::size_t m_width = 1;
    std::size_t m_height = 1;
    std::size_t channels = 3;
    std::vector<std::uint8_t> data;
};
