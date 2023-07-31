#ifndef __WINJECTUMTST_BUFFER_UTIL_HPP__
#define __WINJECTUMTST_BUFFER_UTIL_HPP__

#include "pdu.hpp"

#include <string>

inline buffer_t to_buffer(std::string hexstr)
{
    buffer_t rv(hexstr.size()/2);

    for (size_t i=0; i<hexstr.size(); i++)
    {
        char x = std::toupper(hexstr[i]);
        int xi = x >= 'A' ? x-'A'+10 : x-'0';
        rv[i/2] |= (xi << 4*((i+1)%2));
    }

    return rv;
}

inline size_t to_buffer_direct(uint8_t* data, std::string hexstr)
{

    for (size_t i=0; i<hexstr.size(); i++)
    {
        if (i%2==0)
        {
            data[i/2] = 0;
        }
        char x = std::toupper(hexstr[i]);
        int xi = x >= 'A' ? x-'A'+10 : x-'0';
        data[i/2] |= (xi << 4*((i+1)%2));
    }

    return hexstr.size()/2;
}

#endif // __WINJECT_TEST_BUFFER_UTIL_HPP__
