// Type:  ('RRC_BOOL', {'type': 'unsigned'})
// Type:  ('RRC_BOOL', {'width': '8'})
// Type:  ('RRC_U8', {'type': 'unsigned'})
// Type:  ('RRC_U8', {'width': '8'})
// Type:  ('RRC_U8A', {'type': 'RRC_U8'})
// Type:  ('RRC_U8A', {'dynamic_array': '256'})
// Type:  ('RRC_U16', {'type': 'unsigned'})
// Type:  ('RRC_U16', {'width': '16'})
// Type:  ('RRC_U32', {'type': 'unsigned'})
// Type:  ('RRC_U32', {'width': '64'})
// Type:  ('RRC_U64', {'type': 'unsigned'})
// Type:  ('RRC_U64', {'width': '64'})
// Type:  ('RRC_U16_OPTIONAL', {'type': 'unsigned'})
// Type:  ('RRC_U16_OPTIONAL', {'width': '16'})
// Type:  ('RRC_U16_OPTIONAL', {'optional': ''})
// Enumeration:  ('RRC_LLCTxMode', ('E_RRC_LLCTxMode_TM', None))
// Enumeration:  ('RRC_LLCTxMode', ('E_RRC_LLCTxMode_AM', None))
// Enumeration:  ('RRC_LLCCRCType', ('E_RRC_LLCCRCType_NONE', None))
// Enumeration:  ('RRC_LLCCRCType', ('E_RRC_LLCCRCType_CRC32_04C11DB7', None))
// Sequence:  RRC_LLCTxConfig ('RRC_LLCTxMode', 'mode')
// Sequence:  RRC_LLCTxConfig ('RRC_LLCCRCType', 'crcType')
// Sequence:  RRC_LLCConfig ('RRC_LLCTxConfig', 'txConfig')
// Enumeration:  ('RRC_CipherAlgorithm', ('E_RRC_CipherAlgorithm_NONE', None))
// Enumeration:  ('RRC_CipherAlgorithm', ('E_RRC_CipherAlgorithm_AES128_CBC', None))
// Enumeration:  ('RRC_IntegrityAlgorithm', ('E_RRC_CipherAlgorithm_NONE', None))
// Enumeration:  ('RRC_IntegrityAlgorithm', ('E_RRC_CipherAlgorithm_HMAC_SHA1', None))
// Enumeration:  ('RRC_CompressionAlgorithm', ('RRC_CompressionAlgorithm_NONE', None))
// Enumeration:  ('RRC_CompressionAlgorithm', ('RRC_CompressionAlgorithm_ZLIB', None))
// Sequence:  RRC_PDCPConfig ('RRC_BOOL', 'allowRLF')
// Sequence:  RRC_PDCPConfig ('RRC_BOOL', 'allowSegmentation')
// Sequence:  RRC_PDCPConfig ('RRC_BOOL', 'allowReordering')
// Sequence:  RRC_PDCPConfig ('RRC_U16_OPTIONAL', 'maxSnDistance')
// Sequence:  RRC_PDCPConfig ('RRC_CipherAlgorithm', 'cipherAlgorithm')
// Sequence:  RRC_PDCPConfig ('RRC_IntegrityAlgorithm', 'integrityAlgorithm')
// Sequence:  RRC_PDCPConfig ('RRC_CompressionAlgorithm', 'compressionAlgorithm')
// Sequence:  RRC_PDCPConfig ('RRC_U8', 'compressionLevel')
// Type:  ('RRC_LLCConfigOptional', {'type': 'RRC_LLCConfig'})
// Type:  ('RRC_LLCConfigOptional', {'optional': ''})
// Type:  ('RRC_PDCPConfigOptional', {'type': 'RRC_PDCPConfig'})
// Type:  ('RRC_PDCPConfigOptional', {'optional': ''})
// Sequence:  RRC_PullRequest ('RRC_U8', 'lcid')
// Sequence:  RRC_PullRequest ('RRC_BOOL', 'includeLLCConfig')
// Sequence:  RRC_PullRequest ('RRC_BOOL', 'includePDCPConfig')
// Sequence:  RRC_PullResponse ('RRC_U8', 'lcid')
// Sequence:  RRC_PullResponse ('RRC_LLCConfigOptional', 'llcConfig')
// Sequence:  RRC_PullResponse ('RRC_PDCPConfigOptional', 'pdcpConfig')
// Sequence:  RRC_PushRequest ('RRC_U8', 'lcid')
// Sequence:  RRC_PushRequest ('RRC_LLCConfigOptional', 'llcConfig')
// Sequence:  RRC_PushRequest ('RRC_PDCPConfigOptional', 'pdcpConfig')
// Sequence:  RRC_PushResponse ('RRC_U8', 'lcid')
// Choice:  ('RRC_Message', 'RRC_PullRequest')
// Choice:  ('RRC_Message', 'RRC_PullResponse')
// Choice:  ('RRC_Message', 'RRC_PushRequest')
// Choice:  ('RRC_Message', 'RRC_PushResponse')
// Sequence:  RRC ('RRC_U8', 'requestID')
// Sequence:  RRC ('RRC_Message', 'message')
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
using RRC_U8A = cum::vector<RRC_U8, 256>;
using RRC_U16 = uint16_t;
using RRC_U32 = uint64_t;
using RRC_U64 = uint64_t;
using RRC_U16_OPTIONAL = std::optional<uint16_t>;
enum class RRC_LLCTxMode : uint8_t
{
    E_RRC_LLCTxMode_TM,
    E_RRC_LLCTxMode_AM
};

enum class RRC_LLCCRCType : uint8_t
{
    E_RRC_LLCCRCType_NONE,
    E_RRC_LLCCRCType_CRC32_04C11DB7
};

struct RRC_LLCTxConfig
{
    RRC_LLCTxMode mode;
    RRC_LLCCRCType crcType;
};

struct RRC_LLCConfig
{
    RRC_LLCTxConfig txConfig;
};

enum class RRC_CipherAlgorithm : uint8_t
{
    E_RRC_CipherAlgorithm_NONE,
    E_RRC_CipherAlgorithm_AES128_CBC
};

enum class RRC_IntegrityAlgorithm : uint8_t
{
    E_RRC_CipherAlgorithm_NONE,
    E_RRC_CipherAlgorithm_HMAC_SHA1
};

enum class RRC_CompressionAlgorithm : uint8_t
{
    RRC_CompressionAlgorithm_NONE,
    RRC_CompressionAlgorithm_ZLIB
};

struct RRC_PDCPConfig
{
    RRC_BOOL allowRLF;
    RRC_BOOL allowSegmentation;
    RRC_BOOL allowReordering;
    RRC_U16_OPTIONAL maxSnDistance;
    RRC_CipherAlgorithm cipherAlgorithm;
    RRC_IntegrityAlgorithm integrityAlgorithm;
    RRC_CompressionAlgorithm compressionAlgorithm;
    RRC_U8 compressionLevel;
};

using RRC_LLCConfigOptional = std::optional<RRC_LLCConfig>;
using RRC_PDCPConfigOptional = std::optional<RRC_PDCPConfig>;
struct RRC_PullRequest
{
    RRC_U8 lcid;
    RRC_BOOL includeLLCConfig;
    RRC_BOOL includePDCPConfig;
};

struct RRC_PullResponse
{
    RRC_U8 lcid;
    RRC_LLCConfigOptional llcConfig;
    RRC_PDCPConfigOptional pdcpConfig;
};

struct RRC_PushRequest
{
    RRC_U8 lcid;
    RRC_LLCConfigOptional llcConfig;
    RRC_PDCPConfigOptional pdcpConfig;
};

struct RRC_PushResponse
{
    RRC_U8 lcid;
};

using RRC_Message = std::variant<RRC_PullRequest,RRC_PullResponse,RRC_PushRequest,RRC_PushResponse>;
struct RRC
{
    RRC_U8 requestID;
    RRC_Message message;
};

/***********************************************
/
/            Codec Definitions
/
************************************************/

inline void str(const char* pName, const RRC_LLCTxMode& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (RRC_LLCTxMode::E_RRC_LLCTxMode_TM == pIe) pCtx += "\"E_RRC_LLCTxMode_TM\"";
    if (RRC_LLCTxMode::E_RRC_LLCTxMode_AM == pIe) pCtx += "\"E_RRC_LLCTxMode_AM\"";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void str(const char* pName, const RRC_LLCCRCType& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (RRC_LLCCRCType::E_RRC_LLCCRCType_NONE == pIe) pCtx += "\"E_RRC_LLCCRCType_NONE\"";
    if (RRC_LLCCRCType::E_RRC_LLCCRCType_CRC32_04C11DB7 == pIe) pCtx += "\"E_RRC_LLCCRCType_CRC32_04C11DB7\"";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_LLCTxConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.mode, pCtx);
    encode_per(pIe.crcType, pCtx);
}

inline void decode_per(RRC_LLCTxConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.mode, pCtx);
    decode_per(pIe.crcType, pCtx);
}

inline void str(const char* pName, const RRC_LLCTxConfig& pIe, std::string& pCtx, bool pIsLast)
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
    str("mode", pIe.mode, pCtx, !(--nMandatory+nOptional));
    str("crcType", pIe.crcType, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_LLCConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.txConfig, pCtx);
}

inline void decode_per(RRC_LLCConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.txConfig, pCtx);
}

inline void str(const char* pName, const RRC_LLCConfig& pIe, std::string& pCtx, bool pIsLast)
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
    str("txConfig", pIe.txConfig, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void str(const char* pName, const RRC_CipherAlgorithm& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (RRC_CipherAlgorithm::E_RRC_CipherAlgorithm_NONE == pIe) pCtx += "\"E_RRC_CipherAlgorithm_NONE\"";
    if (RRC_CipherAlgorithm::E_RRC_CipherAlgorithm_AES128_CBC == pIe) pCtx += "\"E_RRC_CipherAlgorithm_AES128_CBC\"";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void str(const char* pName, const RRC_IntegrityAlgorithm& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (RRC_IntegrityAlgorithm::E_RRC_CipherAlgorithm_NONE == pIe) pCtx += "\"E_RRC_CipherAlgorithm_NONE\"";
    if (RRC_IntegrityAlgorithm::E_RRC_CipherAlgorithm_HMAC_SHA1 == pIe) pCtx += "\"E_RRC_CipherAlgorithm_HMAC_SHA1\"";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void str(const char* pName, const RRC_CompressionAlgorithm& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (RRC_CompressionAlgorithm::RRC_CompressionAlgorithm_NONE == pIe) pCtx += "\"RRC_CompressionAlgorithm_NONE\"";
    if (RRC_CompressionAlgorithm::RRC_CompressionAlgorithm_ZLIB == pIe) pCtx += "\"RRC_CompressionAlgorithm_ZLIB\"";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_PDCPConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    if (pIe.maxSnDistance)
    {
        set_optional(optionalmask, 0);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    encode_per(pIe.allowRLF, pCtx);
    encode_per(pIe.allowSegmentation, pCtx);
    encode_per(pIe.allowReordering, pCtx);
    if (pIe.maxSnDistance)
    {
        encode_per(*pIe.maxSnDistance, pCtx);
    }
    encode_per(pIe.cipherAlgorithm, pCtx);
    encode_per(pIe.integrityAlgorithm, pCtx);
    encode_per(pIe.compressionAlgorithm, pCtx);
    encode_per(pIe.compressionLevel, pCtx);
}

inline void decode_per(RRC_PDCPConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    decode_per(optionalmask, sizeof(optionalmask), pCtx);
    decode_per(pIe.allowRLF, pCtx);
    decode_per(pIe.allowSegmentation, pCtx);
    decode_per(pIe.allowReordering, pCtx);
    if (check_optional(optionalmask, 0))
    {
        pIe.maxSnDistance = decltype(pIe.maxSnDistance)::value_type{};
        decode_per(*pIe.maxSnDistance, pCtx);
    }
    decode_per(pIe.cipherAlgorithm, pCtx);
    decode_per(pIe.integrityAlgorithm, pCtx);
    decode_per(pIe.compressionAlgorithm, pCtx);
    decode_per(pIe.compressionLevel, pCtx);
}

inline void str(const char* pName, const RRC_PDCPConfig& pIe, std::string& pCtx, bool pIsLast)
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
    if (pIe.maxSnDistance) nOptional++;
    size_t nMandatory = 7;
    str("allowRLF", pIe.allowRLF, pCtx, !(--nMandatory+nOptional));
    str("allowSegmentation", pIe.allowSegmentation, pCtx, !(--nMandatory+nOptional));
    str("allowReordering", pIe.allowReordering, pCtx, !(--nMandatory+nOptional));
    if (pIe.maxSnDistance)
    {
        str("maxSnDistance", *pIe.maxSnDistance, pCtx, !(nMandatory+--nOptional));
    }
    str("cipherAlgorithm", pIe.cipherAlgorithm, pCtx, !(--nMandatory+nOptional));
    str("integrityAlgorithm", pIe.integrityAlgorithm, pCtx, !(--nMandatory+nOptional));
    str("compressionAlgorithm", pIe.compressionAlgorithm, pCtx, !(--nMandatory+nOptional));
    str("compressionLevel", pIe.compressionLevel, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_PullRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.lcid, pCtx);
    encode_per(pIe.includeLLCConfig, pCtx);
    encode_per(pIe.includePDCPConfig, pCtx);
}

inline void decode_per(RRC_PullRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.lcid, pCtx);
    decode_per(pIe.includeLLCConfig, pCtx);
    decode_per(pIe.includePDCPConfig, pCtx);
}

inline void str(const char* pName, const RRC_PullRequest& pIe, std::string& pCtx, bool pIsLast)
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
    size_t nMandatory = 3;
    str("lcid", pIe.lcid, pCtx, !(--nMandatory+nOptional));
    str("includeLLCConfig", pIe.includeLLCConfig, pCtx, !(--nMandatory+nOptional));
    str("includePDCPConfig", pIe.includePDCPConfig, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_PullResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    if (pIe.llcConfig)
    {
        set_optional(optionalmask, 0);
    }
    if (pIe.pdcpConfig)
    {
        set_optional(optionalmask, 1);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    encode_per(pIe.lcid, pCtx);
    if (pIe.llcConfig)
    {
        encode_per(*pIe.llcConfig, pCtx);
    }
    if (pIe.pdcpConfig)
    {
        encode_per(*pIe.pdcpConfig, pCtx);
    }
}

inline void decode_per(RRC_PullResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    decode_per(optionalmask, sizeof(optionalmask), pCtx);
    decode_per(pIe.lcid, pCtx);
    if (check_optional(optionalmask, 0))
    {
        pIe.llcConfig = decltype(pIe.llcConfig)::value_type{};
        decode_per(*pIe.llcConfig, pCtx);
    }
    if (check_optional(optionalmask, 1))
    {
        pIe.pdcpConfig = decltype(pIe.pdcpConfig)::value_type{};
        decode_per(*pIe.pdcpConfig, pCtx);
    }
}

inline void str(const char* pName, const RRC_PullResponse& pIe, std::string& pCtx, bool pIsLast)
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
    if (pIe.llcConfig) nOptional++;
    if (pIe.pdcpConfig) nOptional++;
    size_t nMandatory = 1;
    str("lcid", pIe.lcid, pCtx, !(--nMandatory+nOptional));
    if (pIe.llcConfig)
    {
        str("llcConfig", *pIe.llcConfig, pCtx, !(nMandatory+--nOptional));
    }
    if (pIe.pdcpConfig)
    {
        str("pdcpConfig", *pIe.pdcpConfig, pCtx, !(nMandatory+--nOptional));
    }
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_PushRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    if (pIe.llcConfig)
    {
        set_optional(optionalmask, 0);
    }
    if (pIe.pdcpConfig)
    {
        set_optional(optionalmask, 1);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    encode_per(pIe.lcid, pCtx);
    if (pIe.llcConfig)
    {
        encode_per(*pIe.llcConfig, pCtx);
    }
    if (pIe.pdcpConfig)
    {
        encode_per(*pIe.pdcpConfig, pCtx);
    }
}

inline void decode_per(RRC_PushRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    decode_per(optionalmask, sizeof(optionalmask), pCtx);
    decode_per(pIe.lcid, pCtx);
    if (check_optional(optionalmask, 0))
    {
        pIe.llcConfig = decltype(pIe.llcConfig)::value_type{};
        decode_per(*pIe.llcConfig, pCtx);
    }
    if (check_optional(optionalmask, 1))
    {
        pIe.pdcpConfig = decltype(pIe.pdcpConfig)::value_type{};
        decode_per(*pIe.pdcpConfig, pCtx);
    }
}

inline void str(const char* pName, const RRC_PushRequest& pIe, std::string& pCtx, bool pIsLast)
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
    if (pIe.llcConfig) nOptional++;
    if (pIe.pdcpConfig) nOptional++;
    size_t nMandatory = 1;
    str("lcid", pIe.lcid, pCtx, !(--nMandatory+nOptional));
    if (pIe.llcConfig)
    {
        str("llcConfig", *pIe.llcConfig, pCtx, !(nMandatory+--nOptional));
    }
    if (pIe.pdcpConfig)
    {
        str("pdcpConfig", *pIe.pdcpConfig, pCtx, !(nMandatory+--nOptional));
    }
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_PushResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.lcid, pCtx);
}

inline void decode_per(RRC_PushResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.lcid, pCtx);
}

inline void str(const char* pName, const RRC_PushResponse& pIe, std::string& pCtx, bool pIsLast)
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
    str("lcid", pIe.lcid, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_Message& pIe, cum::per_codec_ctx& pCtx)
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
}

inline void decode_per(RRC_Message& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    using TypeIndex = uint8_t;
    TypeIndex type;
    decode_per(type, pCtx);
    if (0 == type)
    {
        pIe = RRC_PullRequest();
        decode_per(std::get<0>(pIe), pCtx);
    }
    else if (1 == type)
    {
        pIe = RRC_PullResponse();
        decode_per(std::get<1>(pIe), pCtx);
    }
    else if (2 == type)
    {
        pIe = RRC_PushRequest();
        decode_per(std::get<2>(pIe), pCtx);
    }
    else if (3 == type)
    {
        pIe = RRC_PushResponse();
        decode_per(std::get<3>(pIe), pCtx);
    }
}

inline void str(const char* pName, const RRC_Message& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    using TypeIndex = uint8_t;
    TypeIndex type = pIe.index();
    if (0 == type)
    {
        if (pName)
            pCtx += "\"" + std::string(pName) + "\":{";
        else
            pCtx += "{";
        std::string name = "RRC_PullRequest";
        str(name.c_str(), std::get<0>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (1 == type)
    {
        if (pName)
            pCtx += "\"" + std::string(pName) + "\":{";
        else
            pCtx += "{";
        std::string name = "RRC_PullResponse";
        str(name.c_str(), std::get<1>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (2 == type)
    {
        if (pName)
            pCtx += "\"" + std::string(pName) + "\":{";
        else
            pCtx += "{";
        std::string name = "RRC_PushRequest";
        str(name.c_str(), std::get<2>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (3 == type)
    {
        if (pName)
            pCtx += "\"" + std::string(pName) + "\":{";
        else
            pCtx += "{";
        std::string name = "RRC_PushResponse";
        str(name.c_str(), std::get<3>(pIe), pCtx, true);
        pCtx += "}";
    }
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.requestID, pCtx);
    encode_per(pIe.message, pCtx);
}

inline void decode_per(RRC& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.requestID, pCtx);
    decode_per(pIe.message, pCtx);
}

inline void str(const char* pName, const RRC& pIe, std::string& pCtx, bool pIsLast)
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
    str("requestID", pIe.requestID, pCtx, !(--nMandatory+nOptional));
    str("message", pIe.message, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

#endif //__CUM_MSG_HPP__
