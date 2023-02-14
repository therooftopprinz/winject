#ifndef __WINJECTUM_UDPENDPOINT_HPP__
#define __WINJECTUM_UDPENDPOINT_HPP__

#include "IEndPoint.hpp"
#include <bfc/IReactor.hpp>
#include <string>

class UDPEndPoint : public IEndPoint
{
public:
    struct config_t
    {
        std::string host;
        int port;
    };

    UDPEndPoint(bfc::IReactor& reactor,
        
        const config_t& config)
    {}

private:
    bfc::IReactor& reactor;
};

#endif // __WINJECTUM_UDPENDPOINT_HPP__
