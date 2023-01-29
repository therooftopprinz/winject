#ifndef __WINJECTEST_INFO_DEPS_MATCHER_HPP__
#define __WINJECTEST_INFO_DEPS_MATCHER_HPP__

#include <gtest/gtest.h>
#include <gmock/gmock.h>

MATCHER_P(IsSlot, slot, "")
{
    return arg.in_frame_info.slot_number == slot;
}

#endif // __WINJECTEST_INFO_DEPS_MATCHER_HPP__