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

#ifndef __WINJECTUM_RRC_UTILS_HPP__
#define __WINJECTUM_RRC_UTILS_HPP__

#include "interface/rrc.hpp"
#include "IRRC.hpp"

inline std::string buffer_str(uint8_t* buffer, size_t size)
{
    std::stringstream bufferss;
    for (int i=0; i<size; i++)
    {
        if ((i%16) == 0)
        {
            bufferss << std::hex << std::setfill('0') << std::setw(4) << i << " - ";

            for (int ii=0; ii<16 && (buffer+i+ii) < (buffer+size); ii++)
            {
                char c = buffer[i+ii];
                c = (c>=36 && c<=126) ? c : '.';
                bufferss << std::dec << c;
            }

            if ((size-i) < 16)
            {
                int rem = 16-size%16;
                for (int ii=0; ii <= rem; ii++)
                {
                    bufferss << " ";
                }
            }
            else
            {
                bufferss << " ";
            }
        }

        if (i%16 == 8)
        {
            bufferss << " ";
        }

        bufferss << std::hex << std::setfill('0') << std::setw(2) << (int) buffer[i] << " ";

        if (i%16 == 15)
        {
            bufferss << "\n";
        }
    }
    return bufferss.str();
}

inline RRC_LLCTxMode to_rrc(ILLC::tx_mode_e mode)
{
    if (mode == ILLC::E_TX_MODE_AM)
        return RRC_LLCTxMode::E_RRC_LLCTxMode_AM;
    return RRC_LLCTxMode::E_RRC_LLCTxMode_TM;
}

inline RRC_LLCCRCType to_rrc(ILLC::crc_type_e crc_type)
{
    if (crc_type == ILLC::E_CRC_TYPE_CRC32_04C11DB7)
        return RRC_LLCCRCType::E_RRC_LLCCRCType_CRC32_04C11DB7;
    return RRC_LLCCRCType::E_RRC_LLCCRCType_NONE;
}

inline ILLC::tx_mode_e to_config(RRC_LLCTxMode mode)
{
    if (mode == RRC_LLCTxMode::E_RRC_LLCTxMode_AM)
        return ILLC::E_TX_MODE_AM;
    return ILLC::E_TX_MODE_TM;
}

inline ILLC::crc_type_e to_config(RRC_LLCCRCType crc_type)
{
    if (crc_type == RRC_LLCCRCType::E_RRC_LLCCRCType_CRC32_04C11DB7)
        return ILLC::E_CRC_TYPE_CRC32_04C11DB7;
    return ILLC::E_CRC_TYPE_NONE;
}

#endif // __WINJECTUM_RRC_UTILS_HPP__
