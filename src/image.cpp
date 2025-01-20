#include "image.hpp"

#include "stb_image_write.h"

auto image::write(std::filesystem::path path) const -> void
{
    auto stride_bytes = m_width * channels;
    stbi_write_png(path.string().c_str(), m_width, m_height, channels, data.data(), stride_bytes);
}