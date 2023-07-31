/*
 * Copyright (C) 2023 Prinz Rainer Buyo <mynameisrainer@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __WINJECT_CRC_HPP__
#define __WINJECT_CRC_HPP__

#include <array>
#include <cstdlib>

template<size_t POLYNOMIAL, size_t CRCWIDTH, typename crc_t, bool ref_in, bool ref_out, crc_t INITIAL=0, crc_t FINAL=0>
struct crc{
    crc_t operator()(const void* buffer_, size_t size)
    {
        auto data = (const uint8_t*) buffer_;
        static std::array<crc_t, 256> crc_rem_lut;
        static std::once_flag lut_setup{};
        std::call_once(lut_setup, [](){
                crc_t  crc_rem;
                constexpr auto CRC_MSB = 1 << (CRCWIDTH-1);
                for (int div = 0; div < 256; div++)
                {
                    crc_rem = div << (CRCWIDTH - 8);
                    for (uint8_t b = 8; b > 0; b--)
                    {
                        if (crc_rem & CRC_MSB)
                        {
                            crc_rem = (crc_rem << 1) ^ POLYNOMIAL;
                        }
                        else
                        {
                            crc_rem = (crc_rem << 1);
                        }
                    }
                    crc_rem_lut[div] = crc_rem;
                }
            });

        auto reflector = [](size_t data, const size_t N){
                size_t reflection = 0;
                for (uint8_t bit = 0; bit < N; ++bit)
                {
                    if (data & 1)
                    {
                        auto currbit =  (size_t(1) << ((N - 1) - bit));
                        reflection |= currbit;
                    }
                    data >>= 1;
                }
                return reflection;
            };

        crc_t crcv = INITIAL;
        for (size_t i = 0; i < size; i++)
        {
            constexpr auto SHIFT = (CRCWIDTH - 8);
            uint8_t c = ref_in ? reflector(data[i], 8): data[i];
            uint8_t div = c ^ (crcv >> SHIFT);
            crcv = (crcv << 8) ^ crc_rem_lut[div];
        }

        return (ref_out ? reflector(crcv, CRCWIDTH) : crcv) ^ FINAL;
    }
};

using crc32_04C11DB7 = crc<0x04C11DB7,32,uint32_t,false,false,0,0>;

#endif // __CRC_HPP__