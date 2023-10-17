/**
 * @file VKDeviceObject.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include "VKDevice.hpp"

namespace Shit {
class VKDeviceObject {
    VKDeviceObject &operator=(VKDeviceObject const &) = delete;
    VKDeviceObject(VKDeviceObject const &) = delete;

protected:
    VKDevice *mpDevice;
    VKDeviceObject(VKDevice *pDevice) : mpDevice(pDevice) {}
};
}  // namespace Shit
