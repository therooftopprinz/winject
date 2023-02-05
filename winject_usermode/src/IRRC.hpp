#ifndef __WINJECTUM_IRRC_HPP__
#define __WINJECTUM_IRRC_HPP__

#include "frame_defs.hpp"

struct IRRC
{
    virtual void on_rlf(lcid_t) = 0;
};

#endif // __WINJECTUM_IRRC_HPP__
