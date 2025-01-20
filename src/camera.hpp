#pragma once

#include <cstddef>
#include <filesystem>
#include <print>
#include <chrono>
#include <string>
#include <thread>
#include <functional>
#include <unordered_map>

#include "common.hpp"
#include "raytraceable.hpp"
#include "material.hpp"
#include "image.hpp"

auto seconds_to_time_display_units(float seconds, float &units, std::string &unit_name) -> void
{
    units = seconds;
    unit_name = "seconds";
    if (units > 60.f)
    {
        units /= 60.f;
        unit_name = "minutes";
    }
    if (units > 60.f)
    {
        units /= 60.f;
        unit_name = "hours";
    }
}

struct camera
{
    float aspect_ratio = 1.0f;
    std::size_t image_width = 100.f;
    bool initialized = false;
    std::size_t samples_per_pixel = 10;
    std::size_t max_depth = 10;
    color background;
    angle vfov = angle::from_degrees(90);
    vec3 look_from = vec3{0.f, 0.f, 0.f};
    vec3 look_at = vec3{0.f, 0.f, -1.f};
    vec3 up = vec3{0.f, 1.f, 0.f};

    angle defocus_angle = angle::from_radians(0.f);
    float focus_dist = 10.f;

    auto render(const world &w, const world &lights, std::filesystem::path path, std::size_t thread_count = 1) -> void
    {
        if (!initialized)
            init();

        auto img = image{image_width, image_height};
        std::println("rendering {}x{} image at {} samples per pixel to {}", image_width, image_height, samples_per_pixel, path.string());
        auto start = std::chrono::high_resolution_clock::now();

        auto threads_progress = std::unordered_map<std::size_t, float>();
        auto report_progress = [&](std::size_t thread_id, float percent_done)
        {
            const auto elapsed = std::chrono::high_resolution_clock::now() - start;

            auto &thread_progress = threads_progress[thread_id];
            thread_progress += percent_done;

            float percent_done_total = 0.f;
            for (const auto &[thread_id, percent_done] : threads_progress)
            {
                percent_done_total += percent_done / thread_count;
            }

            const auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
            auto estimated_time_left = elapsed_s / percent_done_total * (100.f - percent_done_total);
            std::string time_unit = "seconds";
            seconds_to_time_display_units(estimated_time_left, estimated_time_left, time_unit);
            std::println("{}% in {}s, estimated {}{} left", percent_done_total, elapsed_s, estimated_time_left, time_unit);
        };
        if (thread_count > 1)
        {
            std::println("using {} threads", thread_count);
            std::vector<std::thread> threads;
            for (std::size_t i = 0; i < thread_count; ++i)
            {
                threads.emplace_back([&, i]()
                                     { render_thread(i, thread_count, w, lights, img, report_progress); });
            }
            for (std::size_t i = 0; i < threads.size(); ++i)
            {
                threads[i].join();
            }
        }
        else
        {
            render_thread(0, 1, w, lights, img, report_progress);
        }
        img.write(path);
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        float elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        std::string time_unit = "seconds";
        seconds_to_time_display_units(elapsed_time, elapsed_time, time_unit);
        std::println("finished in {}{}, output at {}", elapsed_time, time_unit, path.string());
    }

private:
    std::size_t image_height{};
    float pixel_sample_scale{};
    int sqrt_spp{};
    float recip_sqrt_spp{};
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

        sqrt_spp = static_cast<int>(std::sqrtf(samples_per_pixel));
        pixel_sample_scale = 1.f / (sqrt_spp * sqrt_spp);
        recip_sqrt_spp = 1.f / sqrt_spp;

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

    auto ray_color(const ray &r, std::size_t depth, const world &w, const world &lights) const -> color
    {
        if (depth <= 0)
        {
            return color{0, 0, 0};
        }
        hit_result res;
        if (!w.hit(r, interval{0.001f, infinity}, res))
        {
            return background;
            // const float t = 0.5 * (r.direction.y + 1.0f);
            // return lerp(color{1.0f, 1.0f, 1.0f}, color{0.5f, 0.7f, 1.0f}, t);
        }

        scatter_result sres;
        const auto color_from_emission = res.mat->emitted(r, res, res.u, res.v, res.p);

        if (!res.mat->scatter(r, res, sres))
        {
            return color_from_emission;
        }

        if (sres.skip_pdf)
        {
            return sres.attenuation * ray_color(sres.skip_pdf_ray, depth - 1, w, lights);
        }

        const auto light_ptr = std::make_shared<raytraceable_pdf>(lights, res.p);
        const auto p = mixture_pdf{light_ptr, sres.pdf_ptr};

        const auto scattered = ray{res.p, p.generate(), r.time};
        const auto pdf_value = p.value(scattered.direction);

        const auto scatter_pdf = res.mat->scatter_pdf(r, res, scattered);

        const auto sample_color = ray_color(scattered, depth - 1, w, lights);
        const auto color_from_scatter = (sres.attenuation * scatter_pdf * sample_color) / pdf_value;

        return color_from_emission + color_from_scatter;
    }

    auto get_ray(std::size_t j, std::size_t i, std::size_t s_j, std::size_t s_i) const -> ray
    {
        auto offset = sample_square_stratified(s_j, s_i);
        auto pixel_sample = pixel00_loc + ((j + offset.x) * pixel_delta_u) + ((i + offset.y) * pixel_delta_v);

        auto ray_origin = (defocus_angle.radians <= 0.f) ? center : sample_defocus_disk();
        auto ray_direction = pixel_sample - ray_origin;
        auto ray_time = randf();

        return ray{ray_origin, ray_direction, ray_time};
    }

    auto sample_square() const -> vec3
    {
        return vec3{randf() - 0.5f, randf() - 0.5f, 0};
    }

    auto sample_square_stratified(int s_j, int s_i) const -> vec3
    {
        const auto px = ((s_j + randf()) * recip_sqrt_spp) - 0.5f;
        const auto py = ((s_i + randf()) * recip_sqrt_spp) - 0.5f;

        return vec3{px, py, 0.f};
    }

    auto sample_defocus_disk() const -> vec3
    {
        const auto p = vec3::random_in_unit_disk();
        return center + (p.x * defocus_disk_u) + (p.y * defocus_disk_v);
    }

    using time_point = decltype(std::chrono::high_resolution_clock::now());
    using report_progress_fn = std::function<void(std::size_t thread_id, float percent_done)>;

    auto render_thread(std::size_t thread_id, std::size_t num_threads, const world &w, const world &lights, image &img, report_progress_fn report_progress) -> void
    {
        std::string time_unit = "seconds";

        const auto start_y_pixel = thread_id * img.height() / num_threads;
        const auto end_y_pixel = (thread_id + 1) * img.height() / num_threads;
        for (std::size_t i = start_y_pixel; i < end_y_pixel; ++i)
        {
            for (std::size_t j = 0; j < img.width(); ++j)
            {
                auto pixel_color = color{0, 0, 0};
                for (std::size_t s_i = 0; s_i < sqrt_spp; ++s_i)
                {
                    for (std::size_t s_j = 0; s_j < sqrt_spp; ++s_j)
                    {
                        const auto r = get_ray(j, i, s_j, s_i);
                        pixel_color += ray_color(r, max_depth, w, lights);
                    }
                }
                img.set_color(j, i, pixel_sample_scale * pixel_color);
            }

            report_progress(thread_id, static_cast<float>(start_y_pixel + i + 1) / end_y_pixel);
        }
    }
};
