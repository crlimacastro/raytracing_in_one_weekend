#pragma once

#include "texture.hpp"
#include "hit_result.hpp"
#include "scatter_result.hpp"

struct material
{
    virtual ~material() = default;

    virtual auto scatter(const ray &r_in, const hit_result &res, scatter_result& sres) const -> bool
    {
        return false;
    }

    virtual auto scatter_pdf(const ray &r_in, const hit_result &res, const ray &scattered) const -> float
    {
        return 0.f;
    }

    virtual auto emitted(const ray& r_in, const hit_result& res, float u, float v, const vec3 &p) const -> color
    {
        return color{0, 0, 0};
    }
};

struct normals : material
{
    auto scatter(const ray &r_in, const hit_result &res, scatter_result& sres) const -> bool override
    {
        sres.attenuation = 0.5f * (res.normal + color{1, 1, 1});
        sres.pdf_ptr = std::make_shared<cosine_pdf>(res.normal);
        sres.skip_pdf = false;
        return true;
    }
};

struct lambertian : material
{
    std::shared_ptr<texture> albedo;

    lambertian() = default;

    static auto from_color(color a) -> lambertian
    {
        lambertian l;
        l.albedo = std::make_shared<solid_color>(solid_color::from_color(a));
        return l;
    }

    static auto from_texture(std::shared_ptr<texture> a) -> lambertian
    {
        lambertian l;
        l.albedo = a;
        return l;
    }

    auto scatter(const ray &r_in, const hit_result &res, scatter_result& sres) const -> bool override
    {
        sres.attenuation = albedo->value(res.u, res.v, res.p);
        sres.pdf_ptr = std::make_shared<cosine_pdf>(res.normal);
        sres.skip_pdf = false;
        return true;
    }

    auto scatter_pdf(const ray &r_in, const hit_result &res, const ray &scattered) const -> float override
    {
        const auto cos_theta = res.normal.dot(scattered.direction.normalized());
        return cos_theta < 0.f ? 0.f : cos_theta / pi;
    }
};

struct metal : material
{
    color albedo = color{1, 1, 1};
    float fuzz = 1.f;

    metal() = default;
    metal(color a, float fuzz) : albedo{a}, fuzz{std::min(fuzz, 1.f)} {}

    auto scatter(const ray &r_in, const hit_result &res, scatter_result& sres) const -> bool override
    {
        const auto reflected = r_in.direction.reflect(res.normal).normalized() + (fuzz * vec3::random_unit_vector());
        sres.attenuation = albedo;
        sres.pdf_ptr = nullptr;
        sres.skip_pdf = true;
        sres.skip_pdf_ray = ray{res.p, reflected, r_in.time};
        return true;
    }
};

struct dielectric : material
{
    float refraction_index = 1.f;

    dielectric() = default;
    dielectric(float refraction_index) : refraction_index{refraction_index} {}

    auto scatter(const ray &r_in, const hit_result &res, scatter_result& sres) const -> bool override
    {
        sres.attenuation = color{1.f, 1.f, 1.f};
        sres.pdf_ptr = nullptr;
        sres.skip_pdf = true;
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
        sres.skip_pdf_ray = ray{res.p, dir, r_in.time};
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

struct diffuse_light : material
{
    std::shared_ptr<texture> emit;

    diffuse_light(std::shared_ptr<texture> emit) : emit(emit) {}
    diffuse_light(const color &emit) : emit(std::make_shared<solid_color>(solid_color::from_color(emit))) {}

    auto emitted(const ray& r_in, const hit_result& res, float u, float v, const vec3 &p) const -> color override
    {
        if (!res.front_face)
        {
            return color{0.f, 0.f, 0.f};
        }
        return emit->value(u, v, p);
    }
};

struct isotropic : material
{
    std::shared_ptr<texture> tex;

    isotropic() = default;
    isotropic(const color &albedo) : tex(std::make_shared<solid_color>(solid_color::from_color(albedo))) {}
    isotropic(std::shared_ptr<texture> tex) : tex(tex) {}

    auto scatter(const ray &r_in, const hit_result &res, scatter_result& sres) const -> bool override
    {
        sres.attenuation = tex->value(res.u, res.v, res.p);
        sres.pdf_ptr = std::make_shared<sphere_pdf>();
        sres.skip_pdf = false;
        return true;
    }

    auto scatter_pdf(const ray &r_in, const hit_result &res, const ray &scattered) const -> float override
    {
        return 1.f / (4.f * pi);
    }
};