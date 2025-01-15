#include "rtw_stb_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

rtw_image::~rtw_image()
{
    delete[] bdata;
    STBI_FREE(fdata);
}
