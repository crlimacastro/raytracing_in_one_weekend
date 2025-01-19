#pragma once

#include <memory>

#include "common.hpp"

struct scatter_result
{
    color attenuation;
    std::shared_ptr<pdf> pdf_ptr;
    bool skip_pdf;
    ray skip_pdf_ray;
};
