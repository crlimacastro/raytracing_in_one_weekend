#pragma once

#include <cstddef>
#include <filesystem>
#include <print>
#include <chrono>

#include "common.hpp"
#include "raytraceable.hpp"
#include "material.hpp"
#include "image.hpp"

struct camera
{
    float aspect_ratio = 1.0f;
    std::size_t image_width = 100.f;
    bool initialized = false;
    std::size_t samples_per_pixel = 10;
    std::size_t max_depth = 10;
    angle vfov = angle::from_degrees(90);
    vec3 look_from = vec3{0.f, 0.f, 0.f};
    vec3 look_at = vec3{0.f, 0.f, -1.f};
    vec3 up = vec3{0.f, 1.f, 0.f};

    angle defocus_angle = angle::from_radians(0.f);
    float focus_dist = 10.f;

    auto render(world_view world, std::filesystem::path path) -> void
    {
        if (!initialized)
            init();

        auto img = image{image_width, image_height};
        std::println("rendering {}x{} image at {} samples per pixel to {}", image_width, image_height, samples_per_pixel, path.string());
        auto start = std::chrono::high_resolution_clock::now();
        const auto pixel_sample_scale = 1.0f / samples_per_pixel;
        for (std::size_t i = 0; i < image_height; ++i)
        {
            for (std::size_t j = 0; j < image_width; ++j)
            {
                auto pixel_color = color{0, 0, 0};
                for (std::size_t s = 0; s < samples_per_pixel; ++s)
                {
                    const auto r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                img.set_color(j, i, pixel_sample_scale * pixel_color);
            }
            auto percent = static_cast<float>(i + 1) / static_cast<float>(image_height) * 100.f;
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
            auto estimated_time_left = elapsed_s * (100.f - percent) / percent;
            auto time_unit = "seconds";
            if (estimated_time_left > 60.f)
            {
                estimated_time_left /= 60.f;
                time_unit = "minutes";
            }
            if (estimated_time_left > 60.f)
            {
                estimated_time_left /= 60.f;
                time_unit = "hours";
            }
            std::println("{}% in {}s, estimated {}{} left", percent, elapsed_s, estimated_time_left, time_unit);
        }
        img.write(path);
        std::println("finished in {}s, output at {}", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count(), path.string());
    }

private:
    std::size_t image_height{};
    vec3 center{};
    vec3 pixel00_loc{};
    vec3 pixel_delta_u{};
    vec3 pixel_delta_v{};
    vec3 u{}, v{}, w{};
    vec3 defocus_disk_u{};
    vec3 defocus_disk_v{};

    auto init() -> void
    {
        image_height = std::max(static_cast<int>(image_width / aspect_ratio), 1);

        center = look_from;

        const auto theta = vfov.radians;
        const auto h = std::tanf(theta / 2.f);
        const auto viewport_height = 2.f * h * focus_dist;
        const auto viewport_width = viewport_height * static_cast<float>(image_width) / image_height;

        w = (look_from - look_at).normalized();
        u = up.cross(w).normalized();
        v = w.cross(u);

        const auto viewport_u = viewport_width * u;
        const auto viewport_v = viewport_height * -v;

        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        const auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2.f - viewport_v / 2.f;
        pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

        const auto defocus_radius = focus_dist * std::tanf((defocus_angle / 2.f).radians);
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    auto ray_color(const ray &r, std::size_t depth, world_view world) const -> color
    {
        if (depth <= 0)
        {
            return color{0, 0, 0};
        }
        hit_result res;
        if (hit_anything(world, r, interval{0.001f, infinity}, res))
        {
            ray scattered{};
            color attenuation{};
            if (res.mat->scatter(r, res, attenuation, scattered))
            {
                return attenuation * ray_color(scattered, depth - 1, world);
            }
            return color{0, 0, 0};
        }
        const float t = 0.5 * (r.direction.y + 1.0f);
        return lerp(color{1.0f, 1.0f, 1.0f}, color{0.5f, 0.7f, 1.0f}, t);
    }

    auto get_ray(std::size_t j, std::size_t i) const -> ray
    {
        auto offset = sample_square();
        auto pixel_sample = pixel00_loc + ((i + offset.x) * pixel_delta_u) + ((j + offset.y) * pixel_delta_v);

        auto ray_origin = (defocus_angle.radians <= 0.f) ? center : sample_defocus_disk();
        auto ray_direction = pixel_sample - ray_origin;

        return ray{ray_origin, ray_direction};
    }

    auto sample_square() const -> vec3
    {
        return vec3{randf() - 0.5f, randf() - 0.5f, 0};
    }

    auto sample_defocus_disk() const -> vec3
    {
        const auto p = vec3::random_in_unit_disk();
        return center + (p.x * defocus_disk_u) + (p.y * defocus_disk_v);
    }
};
