#ifndef __WINJECTUM_IENDPOINT_HPP__
#define __WINJECTUM_IENDPOINT_HPP__

struct IEndPoint
{
    struct config_t
    {
        lcid_t lcid;
        std::string type;
        std::string address1;
        std::string address2;
    };

    virtual void set_tx_enabled(bool) = 0;
    virtual void set_rx_enabled(bool) = 0;

    virtual ~IEndPoint() {}
};

#endif // __WINJECTUM_IENDPOINT_HPP__
