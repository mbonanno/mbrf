#pragma once

#include "vulkan/vulkan.h"

#include <assert.h>
#include <vector>

#define VK_CHECK(func) { VkResult res = func; assert(res == VK_SUCCESS); }
