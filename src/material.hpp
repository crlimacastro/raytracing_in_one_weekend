#pragma once

#include "raytraceable.hpp"

struct material
{
    virtual ~material() = default;

    virtual bool scatter(const ray &r_in, const hit_result &res, color &attenuation, ray &scattered) const
    {
        return false;
    }
};

struct normals : material
{
    auto scatter(const ray &r_in, const hit_result &res, color &attenuation, ray &scattered) const -> bool override
    {
        attenuation = 0.5f * (res.normal + color{1, 1, 1});
        return true;
    }
};

struct lambertian : material
{
    color albedo = color{1, 1, 1};

    lambertian() = default;
    lambertian(color a) : albedo{a} {}

    auto scatter(const ray &r_in, const hit_result &res, color &attenuation, ray &scattered) const -> bool override
    {
        auto scatter_dir = res.normal + vec3::random_unit_vector();
        if (scatter_dir.near_zero())
        {
            scatter_dir = res.normal;
        }

        scattered = ray{res.p, scatter_dir};
        attenuation = albedo;
        return true;
    }
};

struct metal : material
{
    color albedo = color{1, 1, 1};
    float fuzz = 1.f;

    metal() = default;
    metal(color a, float fuzz) : albedo{a}, fuzz{std::min(fuzz, 1.f)} {}

    auto scatter(const ray &r_in, const hit_result &res, color &attenuation, ray &scattered) const -> bool override
    {
        const auto reflected = r_in.direction.reflect(res.normal).normalized() + (fuzz * vec3::random_unit_vector());
        scattered = ray{res.p, reflected};
        attenuation = albedo;
        return scattered.direction.dot(res.normal) > 0.f;
    }
};

struct dielectric : material
{
    float refraction_index = 1.f;

    dielectric() = default;
    dielectric(float refraction_index) : refraction_index{refraction_index} {}

    auto scatter(const ray &r_in, const hit_result &res, color &attenuation, ray &scattered) const -> bool override
    {
        attenuation = color{1.f, 1.f, 1.f};
        const auto ri = res.front_face ? (1.f / refraction_index) : refraction_index;

        const auto dir_norm = r_in.direction.normalized();
        const auto cos_theta = std::fmin((-dir_norm).dot(res.normal), 1.f);
        const auto sin_theta = std::sqrtf(1.f - cos_theta * cos_theta);

        bool cannot_refract = ri * sin_theta > 1.f;
        vec3 dir{};

        if (cannot_refract || reflectance(cos_theta, ri) > randf())
        {
            dir = dir_norm.reflect(res.normal);
        }
        else
        {
            dir = dir_norm.refract(res.normal, ri);
        }
        scattered = ray{res.p, dir};
        return true;
    }

private:
    auto reflectance(float cos, float refraction_index) const -> float
    {
        // Schlik's approximation
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0 * r0;
        return r0 + (1 - r0) * std::powf(1 - cos, 5);
    }
};
