// Type:  ('RRC_BOOL', {'type': 'unsigned'})
// Type:  ('RRC_BOOL', {'width': '8'})
// Type:  ('RRC_U8', {'type': 'unsigned'})
// Type:  ('RRC_U8', {'width': '8'})
// Type:  ('RRC_U8O', {'type': 'u8'})
// Type:  ('RRC_U8O', {'optional': ''})
// Type:  ('RRC_U16', {'type': 'unsigned'})
// Type:  ('RRC_U16', {'width': '16'})
// Type:  ('RRC_U32', {'type': 'unsigned'})
// Type:  ('RRC_U32', {'width': '64'})
// Type:  ('RRC_U64', {'type': 'unsigned'})
// Type:  ('RRC_U64', {'width': '64'})
// Type:  ('RRC_STR', {'type': 'asciiz'})
// Enumeration:  ('RRCFECType', ('E_RRCFECType_RS_NONE', None))
// Enumeration:  ('RRCFECType', ('E_RRCFECType_RS_255_247', None))
// Enumeration:  ('RRCFECType', ('E_RRCFECType_RS_255_239', None))
// Enumeration:  ('RRCFECType', ('E_RRCFECType_RS_255_223', None))
// Enumeration:  ('RRCFECType', ('E_RRCFECType_RS_255_191', None))
// Enumeration:  ('RRCFECType', ('E_RRCFECType_RS_255_127', None))
// Sequence:  RRCFECConfig ('RRCFECType', 'type')
// Sequence:  RRCFECConfig ('RRC_U32', 'threshold')
// Type:  ('RRCFECConfigList', {'type': 'RRCFECConfig'})
// Type:  ('RRCFECConfigList', {'dynamic_array': '256'})
// Sequence:  RRCFrameConfig ('RRC_U64', 'slotInterval')
// Sequence:  RRCFrameConfig ('RRCFECConfigList', 'fecConfig')
// Sequence:  RRCConfigRequest ('RRC_BOOL', 'includeFrameConfig')
// Sequence:  RRCConfigRequest ('RRC_U8O', 'lcid')
// Type:  ('RRCFrameConfigOptional', {'type': 'RRCFrameConfig'})
// Type:  ('RRCFrameConfigOptional', {'optional': ''})
// Sequence:  RRCConfigResponse ('RRCFrameConfigOptional', 'frameConfig')
// Sequence:  RRCConfigUpdateRequest ('RRC_U8', 'spare')
// Sequence:  RRCConfigUpdateResponse ('RRC_U8', 'spare')
// Sequence:  RRCActivateLLCRequest ('RRC_U8', 'spare')
// Sequence:  RRCActivateLLCResponse ('RRC_U8', 'spare')
// Choice:  ('RRC', 'RRCConfigRequest')
// Choice:  ('RRC', 'RRCConfigResponse')
// Choice:  ('RRC', 'RRCConfigUpdateRequest')
// Choice:  ('RRC', 'RRCConfigUpdateResponse')
// Choice:  ('RRC', 'RRCActivateLLCRequest')
// Choice:  ('RRC', 'RRCActivateLLCResponse')
// Choice:  ('RRC', '')
// Generating for C++
#ifndef __CUM_MSG_HPP__
#define __CUM_MSG_HPP__
#include "cum/cum.hpp"
#include <optional>

/***********************************************
/
/            Message Definitions
/
************************************************/

using RRC_BOOL = uint8_t;
using RRC_U8 = uint8_t;
using RRC_U8O = std::optional<u8>;
using RRC_U16 = uint16_t;
using RRC_U32 = uint64_t;
using RRC_U64 = uint64_t;
using RRC_STR = std::string;
enum class RRCFECType : uint8_t
{
    E_RRCFECType_RS_NONE,
    E_RRCFECType_RS_255_247,
    E_RRCFECType_RS_255_239,
    E_RRCFECType_RS_255_223,
    E_RRCFECType_RS_255_191,
    E_RRCFECType_RS_255_127
};

struct RRCFECConfig
{
    RRCFECType type;
    RRC_U32 threshold;
};

using RRCFECConfigList = cum::vector<RRCFECConfig, 256>;
struct RRCFrameConfig
{
    RRC_U64 slotInterval;
    RRCFECConfigList fecConfig;
};

struct RRCConfigRequest
{
    RRC_BOOL includeFrameConfig;
    RRC_U8O lcid;
};

using RRCFrameConfigOptional = std::optional<RRCFrameConfig>;
struct RRCConfigResponse
{
    RRCFrameConfigOptional frameConfig;
};

struct RRCConfigUpdateRequest
{
    RRC_U8 spare;
};

struct RRCConfigUpdateResponse
{
    RRC_U8 spare;
};

struct RRCActivateLLCRequest
{
    RRC_U8 spare;
};

struct RRCActivateLLCResponse
{
    RRC_U8 spare;
};

using RRC = std::variant<RRCConfigRequest,RRCConfigResponse,RRCConfigUpdateRequest,RRCConfigUpdateResponse,RRCActivateLLCRequest,RRCActivateLLCResponse,>;
/***********************************************
/
/            Codec Definitions
/
************************************************/

inline void str(const char* pName, const RRCFECType& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (RRCFECType::E_RRCFECType_RS_NONE == pIe) pCtx += "\"E_RRCFECType_RS_NONE\"";
    if (RRCFECType::E_RRCFECType_RS_255_247 == pIe) pCtx += "\"E_RRCFECType_RS_255_247\"";
    if (RRCFECType::E_RRCFECType_RS_255_239 == pIe) pCtx += "\"E_RRCFECType_RS_255_239\"";
    if (RRCFECType::E_RRCFECType_RS_255_223 == pIe) pCtx += "\"E_RRCFECType_RS_255_223\"";
    if (RRCFECType::E_RRCFECType_RS_255_191 == pIe) pCtx += "\"E_RRCFECType_RS_255_191\"";
    if (RRCFECType::E_RRCFECType_RS_255_127 == pIe) pCtx += "\"E_RRCFECType_RS_255_127\"";
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRCFECConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.type, pCtx);
    encode_per(pIe.threshold, pCtx);
}

inline void decode_per(RRCFECConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.type, pCtx);
    decode_per(pIe.threshold, pCtx);
}

inline void str(const char* pName, const RRCFECConfig& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 2;
    str("type", pIe.type, pCtx, !(--nMandatory+nOptional));
    str("threshold", pIe.threshold, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRCFrameConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.slotInterval, pCtx);
    encode_per(pIe.fecConfig, pCtx);
}

inline void decode_per(RRCFrameConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.slotInterval, pCtx);
    decode_per(pIe.fecConfig, pCtx);
}

inline void str(const char* pName, const RRCFrameConfig& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 2;
    str("slotInterval", pIe.slotInterval, pCtx, !(--nMandatory+nOptional));
    str("fecConfig", pIe.fecConfig, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRCConfigRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    if (pIe.lcid)
    {
        set_optional(optionalmask, 0);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    encode_per(pIe.includeFrameConfig, pCtx);
    if (pIe.lcid)
    {
        encode_per(*pIe.lcid, pCtx);
    }
}

inline void decode_per(RRCConfigRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    decode_per(optionalmask, sizeof(optionalmask), pCtx);
    decode_per(pIe.includeFrameConfig, pCtx);
    if (check_optional(optionalmask, 0))
    {
        pIe.lcid = decltype(pIe.lcid)::value_type{};
        decode_per(*pIe.lcid, pCtx);
    }
}

inline void str(const char* pName, const RRCConfigRequest& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    if (pIe.lcid) nOptional++;
    size_t nMandatory = 1;
    str("includeFrameConfig", pIe.includeFrameConfig, pCtx, !(--nMandatory+nOptional));
    if (pIe.lcid)
    {
        str("lcid", *pIe.lcid, pCtx, !(nMandatory+--nOptional));
    }
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRCConfigResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    if (pIe.frameConfig)
    {
        set_optional(optionalmask, 0);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    if (pIe.frameConfig)
    {
        encode_per(*pIe.frameConfig, pCtx);
    }
}

inline void decode_per(RRCConfigResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    decode_per(optionalmask, sizeof(optionalmask), pCtx);
    if (check_optional(optionalmask, 0))
    {
        pIe.frameConfig = decltype(pIe.frameConfig)::value_type{};
        decode_per(*pIe.frameConfig, pCtx);
    }
}

inline void str(const char* pName, const RRCConfigResponse& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    if (pIe.frameConfig) nOptional++;
    size_t nMandatory = 0;
    if (pIe.frameConfig)
    {
        str("frameConfig", *pIe.frameConfig, pCtx, !(nMandatory+--nOptional));
    }
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRCConfigUpdateRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(RRCConfigUpdateRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
}

inline void str(const char* pName, const RRCConfigUpdateRequest& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("spare", pIe.spare, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRCConfigUpdateResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(RRCConfigUpdateResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
}

inline void str(const char* pName, const RRCConfigUpdateResponse& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("spare", pIe.spare, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRCActivateLLCRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(RRCActivateLLCRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
}

inline void str(const char* pName, const RRCActivateLLCRequest& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("spare", pIe.spare, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRCActivateLLCResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(RRCActivateLLCResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
}

inline void str(const char* pName, const RRCActivateLLCResponse& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (!pName)
    {
        pCtx = pCtx + "{";
    }
    else
    {
        pCtx = pCtx + "\"" + pName + "\":{";
    }
    size_t nOptional = 0;
    size_t nMandatory = 1;
    str("spare", pIe.spare, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    using TypeIndex = uint8_t;
    TypeIndex type = pIe.index();
    encode_per(type, pCtx);
    if (0 == type)
    {
        encode_per(std::get<0>(pIe), pCtx);
    }
    else if (1 == type)
    {
        encode_per(std::get<1>(pIe), pCtx);
    }
    else if (2 == type)
    {
        encode_per(std::get<2>(pIe), pCtx);
    }
    else if (3 == type)
    {
        encode_per(std::get<3>(pIe), pCtx);
    }
    else if (4 == type)
    {
        encode_per(std::get<4>(pIe), pCtx);
    }
    else if (5 == type)
    {
        encode_per(std::get<5>(pIe), pCtx);
    }
    else if (6 == type)
    {
        encode_per(std::get<6>(pIe), pCtx);
    }
}

inline void decode_per(RRC& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    using TypeIndex = uint8_t;
    TypeIndex type;
    decode_per(type, pCtx);
    if (0 == type)
    {
        pIe = RRCConfigRequest();
        decode_per(std::get<0>(pIe), pCtx);
    }
    else if (1 == type)
    {
        pIe = RRCConfigResponse();
        decode_per(std::get<1>(pIe), pCtx);
    }
    else if (2 == type)
    {
        pIe = RRCConfigUpdateRequest();
        decode_per(std::get<2>(pIe), pCtx);
    }
    else if (3 == type)
    {
        pIe = RRCConfigUpdateResponse();
        decode_per(std::get<3>(pIe), pCtx);
    }
    else if (4 == type)
    {
        pIe = RRCActivateLLCRequest();
        decode_per(std::get<4>(pIe), pCtx);
    }
    else if (5 == type)
    {
        pIe = RRCActivateLLCResponse();
        decode_per(std::get<5>(pIe), pCtx);
    }
    else if (6 == type)
    {
        pIe = ();
        decode_per(std::get<6>(pIe), pCtx);
    }
}

inline void str(const char* pName, const RRC& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    using TypeIndex = uint8_t;
    TypeIndex type = pIe.index();
    if (0 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "RRCConfigRequest";
        str(name.c_str(), std::get<0>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (1 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "RRCConfigResponse";
        str(name.c_str(), std::get<1>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (2 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "RRCConfigUpdateRequest";
        str(name.c_str(), std::get<2>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (3 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "RRCConfigUpdateResponse";
        str(name.c_str(), std::get<3>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (4 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "RRCActivateLLCRequest";
        str(name.c_str(), std::get<4>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (5 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "RRCActivateLLCResponse";
        str(name.c_str(), std::get<5>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (6 == type)
    {
        if (pName)
            pCtx += std::string(pName) + ":{";
        else
            pCtx += "{";
        std::string name = "";
        str(name.c_str(), std::get<6>(pIe), pCtx, true);
        pCtx += "}";
    }
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

#endif //__CUM_MSG_HPP__
