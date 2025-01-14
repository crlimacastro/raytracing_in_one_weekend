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
    auto material_white = std::make_shared<lambertian>(lambertian::from_color(color{0.73f, 0.73f, 0.73f}));
    auto material_normals = std::make_shared<normals>();
    auto material_ground = std::make_shared<lambertian>(lambertian::from_color(color{0.8f, 0.8f, 0.f}));
    auto material_center = std::make_shared<lambertian>(lambertian::from_color(color{0.1f, 0.2f, 0.5f}));
    auto material_left = std::make_shared<dielectric>(1.5f);
    auto material_bubble = std::make_shared<dielectric>(1.f / 1.5f);
    auto material_right = std::make_shared<metal>(color{0.8f, 0.6f, 0.2f}, 1.f);
    auto checker_tex = checker_texture::from_colors(0.32f, color{0.2f, 0.3f, 0.1f}, color{0.9f, 0.9f, 0.9f});
    auto material_checker = std::make_shared<lambertian>(
        lambertian::from_texture(checker_tex));

    world.add(std::make_shared<sphere>(sphere::stationary(vec3{0.f, -100.5f, -1.f}, 100.0f, material_checker)));
    world.add(std::make_shared<sphere>(sphere::stationary(vec3{0.f, 0.f, -1.2f}, 0.5f, material_center)));
    world.add(std::make_shared<sphere>(sphere::stationary(vec3{-1.f, 0.f, -1.f}, 0.5f, material_left)));
    world.add(std::make_shared<sphere>(sphere::stationary(vec3{-1.f, 0.f, -1.f}, 0.4f, material_bubble)));
    world.add(std::make_shared<sphere>(sphere::stationary(vec3{1.f, 0.f, -1.f}, 0.5f, material_right)));

    cam.aspect_ratio = 16.f / 9.f;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.background = color{0.70, 0.80, 1.00};
    cam.vfov = angle::from_degrees(20);
    cam.look_from = vec3{-2.f, 2.f, 1.f};
    cam.look_at = vec3{0.f, 0.f, -1.f};
    cam.up = vec3{0.f, 1.f, 0.f};
    cam.defocus_angle = angle::from_degrees(0.f);
}

auto scene_earth(world &world, camera &cam) -> void
{
    auto earth_text = image_texture::from_file("earthmap.jpg");
    auto earth_material = std::make_shared<lambertian>(
        lambertian::from_texture(earth_text));

    world.add(std::make_shared<sphere>(sphere::stationary(vec3{0.f, 0.f, 0.f}, 2.f, earth_material)));

    cam.aspect_ratio = 16.f / 9.f;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.background = color{0.70, 0.80, 1.00};
    cam.vfov = angle::from_degrees(20);
    cam.look_from = vec3{0.f, 0.f, 12.f};
    cam.look_at = vec3{0.f, 0.f, 0.f};
    cam.up = vec3{0.f, 1.f, 0.f};
    cam.defocus_angle = angle::from_radians(0.f);
}

auto scene_perlin(world &world, camera &cam) -> void
{
    auto noise_tex = std::make_shared<noise_texture>(4);
    world.add(std::make_shared<sphere>(sphere::stationary(vec3{0, -1000, 0}, 1000, std::make_shared<lambertian>(lambertian::from_texture(noise_tex)))));
    world.add(std::make_shared<sphere>(sphere::stationary(vec3{0, 2, 0}, 2, std::make_shared<lambertian>(lambertian::from_texture(noise_tex)))));

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.background = color{0.70, 0.80, 1.00};
    cam.vfov = angle::from_degrees(20);
    cam.look_from = vec3{13, 2, 3};
    cam.look_at = vec3{0, 0, 0};
    cam.up = vec3{0, 1, 0};
    cam.defocus_angle = angle::from_radians(0.f);
}

auto scene_quads(world &world, camera &cam) -> void
{
    auto left_red = std::make_shared<lambertian>(lambertian::from_color(color{1.0, 0.2, 0.2}));
    auto back_green = std::make_shared<lambertian>(lambertian::from_color(color{0.2, 1.0, 0.2}));
    auto right_blue = std::make_shared<lambertian>(lambertian::from_color(color{0.2, 0.2, 1.0}));
    auto upper_orange = std::make_shared<lambertian>(lambertian::from_color(color{1.0, 0.5, 0.0}));
    auto lower_teal = std::make_shared<lambertian>(lambertian::from_color(color{0.2, 0.8, 0.8}));

    // Quads
    world.add(std::make_shared<quad>(vec3{-3, -2, 5}, vec3{0, 0, -4}, vec3{0, 4, 0}, left_red));
    world.add(std::make_shared<quad>(vec3{-2, -2, 0}, vec3{4, 0, 0}, vec3{0, 4, 0}, back_green));
    world.add(std::make_shared<quad>(vec3{3, -2, 1}, vec3{0, 0, 4}, vec3{0, 4, 0}, right_blue));
    world.add(std::make_shared<quad>(vec3{-2, 3, 1}, vec3{4, 0, 0}, vec3{0, 0, 4}, upper_orange));
    world.add(std::make_shared<quad>(vec3{-2, -3, 5}, vec3{4, 0, 0}, vec3{0, 0, -4}, lower_teal));

    cam.aspect_ratio = 1.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.background = color{0.70, 0.80, 1.00};
    cam.vfov = angle::from_degrees(80);
    cam.look_from = vec3{0, 0, 9};
    cam.look_at = vec3{0, 0, 0};
    cam.up = vec3{0, 1, 0};

    cam.defocus_angle = angle::from_radians(0);
}

auto scene_cornell_box(world &world, camera &cam) -> void
{
    auto red = std::make_shared<lambertian>(lambertian::from_color(color{.65, .05, .05}));
    auto white = std::make_shared<lambertian>(lambertian::from_color(color{.73, .73, .73}));
    auto green = std::make_shared<lambertian>(lambertian::from_color(color{.12, .45, .15}));
    auto light = std::make_shared<diffuse_light>(color{15, 15, 15});

    world.add(std::make_shared<quad>(vec3{555, 0, 0}, vec3{0, 555, 0}, vec3{0, 0, 555}, green));
    world.add(std::make_shared<quad>(vec3{0, 0, 0}, vec3{0, 555, 0}, vec3{0, 0, 555}, red));
    world.add(std::make_shared<quad>(vec3{343, 554, 332}, vec3{-130, 0, 0}, vec3{0, 0, -105}, light));
    world.add(std::make_shared<quad>(vec3{0, 0, 0}, vec3{555, 0, 0}, vec3{0, 0, 555}, white));
    world.add(std::make_shared<quad>(vec3{555, 555, 555}, vec3{-555, 0, 0}, vec3{0, 0, -555}, white));
    world.add(std::make_shared<quad>(vec3{0, 0, 555}, vec3{555, 0, 0}, vec3{0, 555, 0}, white));

    std::shared_ptr<raytraceable> box1 = box(vec3{0, 0, 0}, vec3{165, 330, 165}, white);
    box1 = std::make_shared<rotate_y>(box1, angle::from_degrees(15));
    box1 = std::make_shared<translate>(box1, vec3{265, 0, 295});
    world.add(box1);

    std::shared_ptr<raytraceable> box2 = box(vec3{0, 0, 0}, vec3{165, 165, 165}, white);
    box2 = std::make_shared<rotate_y>(box2, angle::from_degrees(-18));
    box2 = std::make_shared<translate>(box2, vec3{130, 0, 65});
    world.add(box2);

    cam.aspect_ratio = 1.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.background = color{0, 0, 0};
    cam.vfov = angle::from_degrees(40);
    cam.look_from = vec3{278, 278, -800};
    cam.look_at = vec3{278, 278, 0};
    cam.up = vec3{0, 1, 0};
    cam.defocus_angle = angle::from_radians(0);
}

auto main(int argc, char *argv[]) -> int
{
    auto args = args::from(argc, argv);
    world world{};
    camera cam{};
    scene_cornell_box(world, cam);
    world.optimize();
    cam.render(world, args.output_path);
}
