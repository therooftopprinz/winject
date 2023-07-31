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

#ifndef __ILCRRC_HPP__
#define __ILCRRC_HPP__

#include "IEndPoint.hpp"
#include "IRRC.hpp"
#include "IPDCP.hpp"
#include "ILLC.hpp"

#include "interface/rrc.hpp"

struct ILCRRC
{
    ~ILCRRC() {}

    virtual void on_init(bool=false) = 0;
    virtual void on_rlf() = 0;
    virtual void on_rrc_message(int req_id, const RRC_ExchangeRequest& req) = 0;
    virtual void on_rrc_message(int req_id, const RRC_ExchangeResponse& rsp) = 0;
};

#endif // __ILCRRC_HPP__