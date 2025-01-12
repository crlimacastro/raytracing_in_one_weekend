#include "image.hpp"

#include "stb_image_write.h"

auto image::write(std::filesystem::path path) const -> void
{
    auto stride_bytes = width * channels;
    stbi_write_png(path.string().c_str(), width, height, channels, data.data(), stride_bytes);
}