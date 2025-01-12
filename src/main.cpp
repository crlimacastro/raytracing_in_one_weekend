#include <string_view>
#include <filesystem>

#include "common.hpp"
#include "raytraceable.hpp"
#include "material.hpp"
#include "camera.hpp"

struct args
{
    std::string_view path;
    std::filesystem::path output_path;

    static auto from(int argc, char *argv[]) -> args
    {
        args args{};
        args.path = argv[0];
        args.output_path = argv[1];
        return args;
    }
};

auto scene_topdown(world &world, camera &cam) -> void
{
    auto material_white = std::make_shared<lambertian>(color{0.73f, 0.73f, 0.73f});
    auto material_normals = std::make_shared<normals>();
    auto material_ground = std::make_shared<lambertian>(color{0.8f, 0.8f, 0.f});
    auto material_center = std::make_shared<lambertian>(color{0.1f, 0.2f, 0.5f});
    auto material_left = std::make_shared<dielectric>(1.5f);
    auto material_bubble = std::make_shared<dielectric>(1.f / 1.5f);
    auto material_right = std::make_shared<metal>(color{0.8f, 0.6f, 0.2f}, 1.f);

    world.emplace_back(std::make_unique<sphere>(vec3{0.f, -100.5f, -1.f}, 100.0f, material_ground));
    world.emplace_back(std::make_unique<sphere>(vec3{0.f, 0.f, -1.2f}, 0.5f, material_center));
    world.emplace_back(std::make_unique<sphere>(vec3{-1.f, 0.f, -1.f}, 0.5f, material_left));
    world.emplace_back(std::make_unique<sphere>(vec3{-1.f, 0.f, -1.f}, 0.4f, material_bubble));
    world.emplace_back(std::make_unique<sphere>(vec3{1.f, 0.f, -1.f}, 0.5f, material_right));

    cam.aspect_ratio = 16.f / 9.f;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.vfov = angle::from_degrees(20);
    cam.look_from = vec3{-2.f, 2.f, 1.f};
    cam.look_at = vec3{0.f, 0.f, -1.f};
    cam.up = vec3{0.f, 1.f, 0.f};
    cam.defocus_angle = angle::from_degrees(10.f);
    cam.focus_dist = 3.4f;
}

auto scene_complex(world &world, camera &cam) -> void
{
    auto ground_material = std::make_shared<lambertian>(color{0.5f, 0.5f, 0.5f});
    world.emplace_back(std::make_unique<sphere>(vec3{0.f, -1000.f, 0.f}, 1000.f, ground_material));

    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            auto choose_mat = randf();
            vec3 center{a + 0.9f * randf(), 0.2f, b + 0.9f * randf()};

            if ((center - vec3{4.f, 0.2f, 0.f}).magnitude() > 0.9f)
            {
                std::shared_ptr<material> sphere_material;

                if (choose_mat < 0.8f)
                {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = std::make_shared<lambertian>(albedo);
                    world.emplace_back(std::make_unique<sphere>(center, 0.2f, sphere_material));
                }
                else if (choose_mat < 0.95)
                {
                    // metal
                    auto albedo = color::random(0.5f, 1.f);
                    auto fuzz = randf(0.f, 0.5f);
                    sphere_material = std::make_shared<metal>(albedo, fuzz);
                    world.emplace_back(std::make_unique<sphere>(center, 0.2f, sphere_material));
                }
                else
                {
                    // glass
                    sphere_material = std::make_shared<dielectric>(1.5f);
                    world.emplace_back(std::make_unique<sphere>(center, 0.2f, sphere_material));
                }
            }
        }
    }

    auto material1 = std::make_shared<dielectric>(1.5f);
    world.emplace_back(std::make_unique<sphere>(vec3{0.f, 1.f, 0.f}, 1.f, material1));

    auto material2 = std::make_shared<lambertian>(color{0.4f, 0.2f, 0.1f});
    world.emplace_back(std::make_unique<sphere>(vec3{-4.f, 1.f, 0.f}, 1.f, material2));

    auto material3 = std::make_shared<metal>(color{0.7f, 0.6f, 0.5f}, 0.f);
    world.emplace_back(std::make_unique<sphere>(vec3{4.f, 1.f, 0.f}, 1.0f, material3));

    cam.aspect_ratio = 16.f / 9.f;
    cam.image_width = 1200;
    cam.samples_per_pixel = 10;
    cam.max_depth = 50;
    cam.vfov = angle::from_degrees(20.f);
    cam.look_from = vec3{13.f, 2.f, 3.f};
    cam.look_at = vec3{0.f, 0.f, 0.f};
    cam.up = vec3{0.f, 1.f, 0.f};
    cam.defocus_angle = angle::from_degrees(0.6f);
    cam.focus_dist = 10.0f;
}

auto main(int argc, char *argv[]) -> int
{
    auto args = args::from(argc, argv);
    world world{};
    camera cam{};
    scene_topdown(world, cam);
    cam.render(world, args.output_path);
}
