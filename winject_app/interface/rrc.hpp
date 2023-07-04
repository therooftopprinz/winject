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
// Type:  ('RRC_STR', {'type': 'asciiz'})
// Type:  ('RRC_U16_OPTIONAL', {'type': 'unsigned'})
// Type:  ('RRC_U16_OPTIONAL', {'width': '16'})
// Type:  ('RRC_U16_OPTIONAL', {'optional': ''})
// Enumeration:  ('RRC_FECType', ('E_RRC_FECType_RS_NONE', None))
// Enumeration:  ('RRC_FECType', ('E_RRC_FECType_RS_255_247', None))
// Enumeration:  ('RRC_FECType', ('E_RRC_FECType_RS_255_239', None))
// Enumeration:  ('RRC_FECType', ('E_RRC_FECType_RS_255_223', None))
// Enumeration:  ('RRC_FECType', ('E_RRC_FECType_RS_255_191', None))
// Enumeration:  ('RRC_FECType', ('E_RRC_FECType_RS_255_127', None))
// Sequence:  RRC_FECConfig ('RRC_FECType', 'type')
// Sequence:  RRC_FECConfig ('RRC_U32', 'threshold')
// Type:  ('RRC_FECConfigList', {'type': 'RRC_FECConfig'})
// Type:  ('RRC_FECConfigList', {'dynamic_array': '256'})
// Sequence:  RRC_FrameConfig ('RRC_U64', 'slotInterval')
// Sequence:  RRC_FrameConfig ('RRC_FECConfigList', 'fecConfig')
// Sequence:  RRC_FrameConfig ('RRC_U16', 'framePayloadSize')
// Enumeration:  ('RRC_LLCTxMode', ('E_RRC_LLCTxMode_TM', None))
// Enumeration:  ('RRC_LLCTxMode', ('E_RRC_LLCTxMode_AM', None))
// Enumeration:  ('RRC_LLCCRCType', ('E_RRC_LLCCRCType_NONE', None))
// Enumeration:  ('RRC_LLCCRCType', ('E_RRC_LLCCRCType_CRC32_04C11DB7', None))
// Sequence:  RRC_LLCTxConfig ('RRC_LLCTxMode', 'mode')
// Sequence:  RRC_LLCTxConfig ('RRC_U16', 'arqWindowSize')
// Sequence:  RRC_LLCTxConfig ('RRC_U16', 'maxRetxCount')
// Sequence:  RRC_LLCTxConfig ('RRC_LLCCRCType', 'crcType')
// Sequence:  RRC_SchedulingConfig ('RRC_U16', 'ndGpduMaxSize')
// Sequence:  RRC_SchedulingConfig ('RRC_U16', 'quanta')
// Sequence:  RRC_LLCConfig ('RRC_U8', 'llcid')
// Sequence:  RRC_LLCConfig ('RRC_LLCTxConfig', 'txConfig')
// Sequence:  RRC_LLCConfig ('RRC_SchedulingConfig', 'schedulingConfig')
// Enumeration:  ('RRC_EPType', ('E_RRC_EPType_INTERNAL', None))
// Enumeration:  ('RRC_EPType', ('E_RRC_EPType_UDP', None))
// Enumeration:  ('RRC_EPType', ('E_RRC_EPType_UART', None))
// Enumeration:  ('RRC_EPType', ('E_RRC_EPType_TUN', None))
// Enumeration:  ('RRC_CipherAlgorithm', ('E_RRC_CipherAlgorithm_NONE', None))
// Enumeration:  ('RRC_CipherAlgorithm', ('E_RRC_CipherAlgorithm_AES128_CBC', None))
// Enumeration:  ('RRC_IntegrityAlgorithm', ('E_RRC_CipherAlgorithm_NONE', None))
// Enumeration:  ('RRC_IntegrityAlgorithm', ('E_RRC_CipherAlgorithm_HMAC_SHA1', None))
// Enumeration:  ('RRC_CompressionAlgorithm', ('RRC_CompressionAlgorithm_NONE', None))
// Enumeration:  ('RRC_CompressionAlgorithm', ('RRC_CompressionAlgorithm_ZLIB', None))
// Sequence:  RRC_PDCPConfig ('RRC_U8', 'lcid')
// Sequence:  RRC_PDCPConfig ('RRC_BOOL', 'allowRLF')
// Sequence:  RRC_PDCPConfig ('RRC_BOOL', 'allowSegmentation')
// Sequence:  RRC_PDCPConfig ('RRC_BOOL', 'allowReordering')
// Sequence:  RRC_PDCPConfig ('RRC_U16_OPTIONAL', 'maxSnDistance')
// Sequence:  RRC_PDCPConfig ('RRC_U16', 'minCommitSize')
// Sequence:  RRC_PDCPConfig ('RRC_U8A', 'cipherKey')
// Sequence:  RRC_PDCPConfig ('RRC_U8A', 'integrityKey')
// Sequence:  RRC_PDCPConfig ('RRC_CipherAlgorithm', 'cipherAlgorithm')
// Sequence:  RRC_PDCPConfig ('RRC_IntegrityAlgorithm', 'integrityAlgorithm')
// Sequence:  RRC_PDCPConfig ('RRC_CompressionAlgorithm', 'compressionAlgorithm')
// Sequence:  RRC_PDCPConfig ('RRC_U8', 'compressionLevel')
// Sequence:  RRC_PDCPConfig ('RRC_EPType', 'type')
// Sequence:  RRC_PDCPConfig ('RRC_STR', 'address')
// Sequence:  RRC_PullRequest ('RRC_BOOL', 'includeFrameConfig')
// Sequence:  RRC_PullRequest ('RRC_BOOL', 'includeLLCConfig')
// Sequence:  RRC_PullRequest ('RRC_BOOL', 'includePDCPConfig')
// Sequence:  RRC_PullRequest ('RRC_U8', 'lcid')
// Type:  ('RRC_FrameConfigOptional', {'type': 'RRC_FrameConfig'})
// Type:  ('RRC_FrameConfigOptional', {'optional': ''})
// Type:  ('RRC_LLCConfigOptional', {'type': 'RRC_LLCConfig'})
// Type:  ('RRC_LLCConfigOptional', {'optional': ''})
// Type:  ('RRC_PDCPConfigOptional', {'type': 'RRC_PDCPConfig'})
// Type:  ('RRC_PDCPConfigOptional', {'optional': ''})
// Sequence:  RRC_PullResponse ('RRC_FrameConfigOptional', 'frameConfig')
// Sequence:  RRC_PullResponse ('RRC_LLCConfigOptional', 'llcConfig')
// Sequence:  RRC_PullResponse ('RRC_PDCPConfigOptional', 'pdcpConfig')
// Sequence:  RRC_PushRequest ('RRC_FrameConfigOptional', 'frameConfig')
// Sequence:  RRC_PushRequest ('RRC_LLCConfigOptional', 'llcConfig')
// Sequence:  RRC_PushRequest ('RRC_PDCPConfigOptional', 'pdcpConfig')
// Sequence:  RRC_ExchangeRequest ('RRC_FrameConfigOptional', 'frameConfig')
// Sequence:  RRC_ExchangeRequest ('RRC_LLCConfigOptional', 'llcConfig')
// Sequence:  RRC_ExchangeRequest ('RRC_PDCPConfigOptional', 'pdcpConfig')
// Sequence:  RRC_ExchangeResponse ('RRC_FrameConfigOptional', 'frameConfig')
// Sequence:  RRC_ExchangeResponse ('RRC_LLCConfigOptional', 'llcConfig')
// Sequence:  RRC_ExchangeResponse ('RRC_PDCPConfigOptional', 'pdcpConfig')
// Sequence:  RRC_PushResponse ('RRC_U8', 'spare')
// Choice:  ('RRC_Message', 'RRC_PullRequest')
// Choice:  ('RRC_Message', 'RRC_PullResponse')
// Choice:  ('RRC_Message', 'RRC_PushRequest')
// Choice:  ('RRC_Message', 'RRC_PushResponse')
// Choice:  ('RRC_Message', 'RRC_ExchangeRequest')
// Choice:  ('RRC_Message', 'RRC_ExchangeResponse')
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
using RRC_STR = std::string;
using RRC_U16_OPTIONAL = std::optional<uint16_t>;
enum class RRC_FECType : uint8_t
{
    E_RRC_FECType_RS_NONE,
    E_RRC_FECType_RS_255_247,
    E_RRC_FECType_RS_255_239,
    E_RRC_FECType_RS_255_223,
    E_RRC_FECType_RS_255_191,
    E_RRC_FECType_RS_255_127
};

struct RRC_FECConfig
{
    RRC_FECType type;
    RRC_U32 threshold;
};

using RRC_FECConfigList = cum::vector<RRC_FECConfig, 256>;
struct RRC_FrameConfig
{
    RRC_U64 slotInterval;
    RRC_FECConfigList fecConfig;
    RRC_U16 framePayloadSize;
};

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
    RRC_U16 arqWindowSize;
    RRC_U16 maxRetxCount;
    RRC_LLCCRCType crcType;
};

struct RRC_SchedulingConfig
{
    RRC_U16 ndGpduMaxSize;
    RRC_U16 quanta;
};

struct RRC_LLCConfig
{
    RRC_U8 llcid;
    RRC_LLCTxConfig txConfig;
    RRC_SchedulingConfig schedulingConfig;
};

enum class RRC_EPType : uint8_t
{
    E_RRC_EPType_INTERNAL,
    E_RRC_EPType_UDP,
    E_RRC_EPType_UART,
    E_RRC_EPType_TUN
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
    RRC_U8 lcid;
    RRC_BOOL allowRLF;
    RRC_BOOL allowSegmentation;
    RRC_BOOL allowReordering;
    RRC_U16_OPTIONAL maxSnDistance;
    RRC_U16 minCommitSize;
    RRC_U8A cipherKey;
    RRC_U8A integrityKey;
    RRC_CipherAlgorithm cipherAlgorithm;
    RRC_IntegrityAlgorithm integrityAlgorithm;
    RRC_CompressionAlgorithm compressionAlgorithm;
    RRC_U8 compressionLevel;
    RRC_EPType type;
    RRC_STR address;
};

struct RRC_PullRequest
{
    RRC_BOOL includeFrameConfig;
    RRC_BOOL includeLLCConfig;
    RRC_BOOL includePDCPConfig;
    RRC_U8 lcid;
};

using RRC_FrameConfigOptional = std::optional<RRC_FrameConfig>;
using RRC_LLCConfigOptional = std::optional<RRC_LLCConfig>;
using RRC_PDCPConfigOptional = std::optional<RRC_PDCPConfig>;
struct RRC_PullResponse
{
    RRC_FrameConfigOptional frameConfig;
    RRC_LLCConfigOptional llcConfig;
    RRC_PDCPConfigOptional pdcpConfig;
};

struct RRC_PushRequest
{
    RRC_FrameConfigOptional frameConfig;
    RRC_LLCConfigOptional llcConfig;
    RRC_PDCPConfigOptional pdcpConfig;
};

struct RRC_ExchangeRequest
{
    RRC_FrameConfigOptional frameConfig;
    RRC_LLCConfigOptional llcConfig;
    RRC_PDCPConfigOptional pdcpConfig;
};

struct RRC_ExchangeResponse
{
    RRC_FrameConfigOptional frameConfig;
    RRC_LLCConfigOptional llcConfig;
    RRC_PDCPConfigOptional pdcpConfig;
};

struct RRC_PushResponse
{
    RRC_U8 spare;
};

using RRC_Message = std::variant<RRC_PullRequest,RRC_PullResponse,RRC_PushRequest,RRC_PushResponse,RRC_ExchangeRequest,RRC_ExchangeResponse>;
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

inline void str(const char* pName, const RRC_FECType& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (RRC_FECType::E_RRC_FECType_RS_NONE == pIe) pCtx += "\"E_RRC_FECType_RS_NONE\"";
    if (RRC_FECType::E_RRC_FECType_RS_255_247 == pIe) pCtx += "\"E_RRC_FECType_RS_255_247\"";
    if (RRC_FECType::E_RRC_FECType_RS_255_239 == pIe) pCtx += "\"E_RRC_FECType_RS_255_239\"";
    if (RRC_FECType::E_RRC_FECType_RS_255_223 == pIe) pCtx += "\"E_RRC_FECType_RS_255_223\"";
    if (RRC_FECType::E_RRC_FECType_RS_255_191 == pIe) pCtx += "\"E_RRC_FECType_RS_255_191\"";
    if (RRC_FECType::E_RRC_FECType_RS_255_127 == pIe) pCtx += "\"E_RRC_FECType_RS_255_127\"";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_FECConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.type, pCtx);
    encode_per(pIe.threshold, pCtx);
}

inline void decode_per(RRC_FECConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.type, pCtx);
    decode_per(pIe.threshold, pCtx);
}

inline void str(const char* pName, const RRC_FECConfig& pIe, std::string& pCtx, bool pIsLast)
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

inline void encode_per(const RRC_FrameConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.slotInterval, pCtx);
    encode_per(pIe.fecConfig, pCtx);
    encode_per(pIe.framePayloadSize, pCtx);
}

inline void decode_per(RRC_FrameConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.slotInterval, pCtx);
    decode_per(pIe.fecConfig, pCtx);
    decode_per(pIe.framePayloadSize, pCtx);
}

inline void str(const char* pName, const RRC_FrameConfig& pIe, std::string& pCtx, bool pIsLast)
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
    str("slotInterval", pIe.slotInterval, pCtx, !(--nMandatory+nOptional));
    str("fecConfig", pIe.fecConfig, pCtx, !(--nMandatory+nOptional));
    str("framePayloadSize", pIe.framePayloadSize, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

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
    encode_per(pIe.arqWindowSize, pCtx);
    encode_per(pIe.maxRetxCount, pCtx);
    encode_per(pIe.crcType, pCtx);
}

inline void decode_per(RRC_LLCTxConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.mode, pCtx);
    decode_per(pIe.arqWindowSize, pCtx);
    decode_per(pIe.maxRetxCount, pCtx);
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
    size_t nMandatory = 4;
    str("mode", pIe.mode, pCtx, !(--nMandatory+nOptional));
    str("arqWindowSize", pIe.arqWindowSize, pCtx, !(--nMandatory+nOptional));
    str("maxRetxCount", pIe.maxRetxCount, pCtx, !(--nMandatory+nOptional));
    str("crcType", pIe.crcType, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_SchedulingConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.ndGpduMaxSize, pCtx);
    encode_per(pIe.quanta, pCtx);
}

inline void decode_per(RRC_SchedulingConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.ndGpduMaxSize, pCtx);
    decode_per(pIe.quanta, pCtx);
}

inline void str(const char* pName, const RRC_SchedulingConfig& pIe, std::string& pCtx, bool pIsLast)
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
    str("ndGpduMaxSize", pIe.ndGpduMaxSize, pCtx, !(--nMandatory+nOptional));
    str("quanta", pIe.quanta, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_LLCConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.llcid, pCtx);
    encode_per(pIe.txConfig, pCtx);
    encode_per(pIe.schedulingConfig, pCtx);
}

inline void decode_per(RRC_LLCConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.llcid, pCtx);
    decode_per(pIe.txConfig, pCtx);
    decode_per(pIe.schedulingConfig, pCtx);
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
    size_t nMandatory = 3;
    str("llcid", pIe.llcid, pCtx, !(--nMandatory+nOptional));
    str("txConfig", pIe.txConfig, pCtx, !(--nMandatory+nOptional));
    str("schedulingConfig", pIe.schedulingConfig, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void str(const char* pName, const RRC_EPType& pIe, std::string& pCtx, bool pIsLast)
{
    using namespace cum;
    if (pName)
    {
        pCtx = pCtx + "\"" + pName + "\":";
    }
    if (RRC_EPType::E_RRC_EPType_INTERNAL == pIe) pCtx += "\"E_RRC_EPType_INTERNAL\"";
    if (RRC_EPType::E_RRC_EPType_UDP == pIe) pCtx += "\"E_RRC_EPType_UDP\"";
    if (RRC_EPType::E_RRC_EPType_UART == pIe) pCtx += "\"E_RRC_EPType_UART\"";
    if (RRC_EPType::E_RRC_EPType_TUN == pIe) pCtx += "\"E_RRC_EPType_TUN\"";
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
    encode_per(pIe.lcid, pCtx);
    encode_per(pIe.allowRLF, pCtx);
    encode_per(pIe.allowSegmentation, pCtx);
    encode_per(pIe.allowReordering, pCtx);
    if (pIe.maxSnDistance)
    {
        encode_per(*pIe.maxSnDistance, pCtx);
    }
    encode_per(pIe.minCommitSize, pCtx);
    encode_per(pIe.cipherKey, pCtx);
    encode_per(pIe.integrityKey, pCtx);
    encode_per(pIe.cipherAlgorithm, pCtx);
    encode_per(pIe.integrityAlgorithm, pCtx);
    encode_per(pIe.compressionAlgorithm, pCtx);
    encode_per(pIe.compressionLevel, pCtx);
    encode_per(pIe.type, pCtx);
    encode_per(pIe.address, pCtx);
}

inline void decode_per(RRC_PDCPConfig& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    decode_per(optionalmask, sizeof(optionalmask), pCtx);
    decode_per(pIe.lcid, pCtx);
    decode_per(pIe.allowRLF, pCtx);
    decode_per(pIe.allowSegmentation, pCtx);
    decode_per(pIe.allowReordering, pCtx);
    if (check_optional(optionalmask, 0))
    {
        pIe.maxSnDistance = decltype(pIe.maxSnDistance)::value_type{};
        decode_per(*pIe.maxSnDistance, pCtx);
    }
    decode_per(pIe.minCommitSize, pCtx);
    decode_per(pIe.cipherKey, pCtx);
    decode_per(pIe.integrityKey, pCtx);
    decode_per(pIe.cipherAlgorithm, pCtx);
    decode_per(pIe.integrityAlgorithm, pCtx);
    decode_per(pIe.compressionAlgorithm, pCtx);
    decode_per(pIe.compressionLevel, pCtx);
    decode_per(pIe.type, pCtx);
    decode_per(pIe.address, pCtx);
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
    size_t nMandatory = 13;
    str("lcid", pIe.lcid, pCtx, !(--nMandatory+nOptional));
    str("allowRLF", pIe.allowRLF, pCtx, !(--nMandatory+nOptional));
    str("allowSegmentation", pIe.allowSegmentation, pCtx, !(--nMandatory+nOptional));
    str("allowReordering", pIe.allowReordering, pCtx, !(--nMandatory+nOptional));
    if (pIe.maxSnDistance)
    {
        str("maxSnDistance", *pIe.maxSnDistance, pCtx, !(nMandatory+--nOptional));
    }
    str("minCommitSize", pIe.minCommitSize, pCtx, !(--nMandatory+nOptional));
    str("cipherKey", pIe.cipherKey, pCtx, !(--nMandatory+nOptional));
    str("integrityKey", pIe.integrityKey, pCtx, !(--nMandatory+nOptional));
    str("cipherAlgorithm", pIe.cipherAlgorithm, pCtx, !(--nMandatory+nOptional));
    str("integrityAlgorithm", pIe.integrityAlgorithm, pCtx, !(--nMandatory+nOptional));
    str("compressionAlgorithm", pIe.compressionAlgorithm, pCtx, !(--nMandatory+nOptional));
    str("compressionLevel", pIe.compressionLevel, pCtx, !(--nMandatory+nOptional));
    str("type", pIe.type, pCtx, !(--nMandatory+nOptional));
    str("address", pIe.address, pCtx, !(--nMandatory+nOptional));
    pCtx = pCtx + "}";
    if (!pIsLast)
    {
        pCtx += ",";
    }
}

inline void encode_per(const RRC_PullRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    encode_per(pIe.includeFrameConfig, pCtx);
    encode_per(pIe.includeLLCConfig, pCtx);
    encode_per(pIe.includePDCPConfig, pCtx);
    encode_per(pIe.lcid, pCtx);
}

inline void decode_per(RRC_PullRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.includeFrameConfig, pCtx);
    decode_per(pIe.includeLLCConfig, pCtx);
    decode_per(pIe.includePDCPConfig, pCtx);
    decode_per(pIe.lcid, pCtx);
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
    size_t nMandatory = 4;
    str("includeFrameConfig", pIe.includeFrameConfig, pCtx, !(--nMandatory+nOptional));
    str("includeLLCConfig", pIe.includeLLCConfig, pCtx, !(--nMandatory+nOptional));
    str("includePDCPConfig", pIe.includePDCPConfig, pCtx, !(--nMandatory+nOptional));
    str("lcid", pIe.lcid, pCtx, !(--nMandatory+nOptional));
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
    if (pIe.frameConfig)
    {
        set_optional(optionalmask, 0);
    }
    if (pIe.llcConfig)
    {
        set_optional(optionalmask, 1);
    }
    if (pIe.pdcpConfig)
    {
        set_optional(optionalmask, 2);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    if (pIe.frameConfig)
    {
        encode_per(*pIe.frameConfig, pCtx);
    }
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
    if (check_optional(optionalmask, 0))
    {
        pIe.frameConfig = decltype(pIe.frameConfig)::value_type{};
        decode_per(*pIe.frameConfig, pCtx);
    }
    if (check_optional(optionalmask, 1))
    {
        pIe.llcConfig = decltype(pIe.llcConfig)::value_type{};
        decode_per(*pIe.llcConfig, pCtx);
    }
    if (check_optional(optionalmask, 2))
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
    if (pIe.frameConfig) nOptional++;
    if (pIe.llcConfig) nOptional++;
    if (pIe.pdcpConfig) nOptional++;
    size_t nMandatory = 0;
    if (pIe.frameConfig)
    {
        str("frameConfig", *pIe.frameConfig, pCtx, !(nMandatory+--nOptional));
    }
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
    if (pIe.frameConfig)
    {
        set_optional(optionalmask, 0);
    }
    if (pIe.llcConfig)
    {
        set_optional(optionalmask, 1);
    }
    if (pIe.pdcpConfig)
    {
        set_optional(optionalmask, 2);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    if (pIe.frameConfig)
    {
        encode_per(*pIe.frameConfig, pCtx);
    }
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
    if (check_optional(optionalmask, 0))
    {
        pIe.frameConfig = decltype(pIe.frameConfig)::value_type{};
        decode_per(*pIe.frameConfig, pCtx);
    }
    if (check_optional(optionalmask, 1))
    {
        pIe.llcConfig = decltype(pIe.llcConfig)::value_type{};
        decode_per(*pIe.llcConfig, pCtx);
    }
    if (check_optional(optionalmask, 2))
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
    if (pIe.frameConfig) nOptional++;
    if (pIe.llcConfig) nOptional++;
    if (pIe.pdcpConfig) nOptional++;
    size_t nMandatory = 0;
    if (pIe.frameConfig)
    {
        str("frameConfig", *pIe.frameConfig, pCtx, !(nMandatory+--nOptional));
    }
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

inline void encode_per(const RRC_ExchangeRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    if (pIe.frameConfig)
    {
        set_optional(optionalmask, 0);
    }
    if (pIe.llcConfig)
    {
        set_optional(optionalmask, 1);
    }
    if (pIe.pdcpConfig)
    {
        set_optional(optionalmask, 2);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    if (pIe.frameConfig)
    {
        encode_per(*pIe.frameConfig, pCtx);
    }
    if (pIe.llcConfig)
    {
        encode_per(*pIe.llcConfig, pCtx);
    }
    if (pIe.pdcpConfig)
    {
        encode_per(*pIe.pdcpConfig, pCtx);
    }
}

inline void decode_per(RRC_ExchangeRequest& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    decode_per(optionalmask, sizeof(optionalmask), pCtx);
    if (check_optional(optionalmask, 0))
    {
        pIe.frameConfig = decltype(pIe.frameConfig)::value_type{};
        decode_per(*pIe.frameConfig, pCtx);
    }
    if (check_optional(optionalmask, 1))
    {
        pIe.llcConfig = decltype(pIe.llcConfig)::value_type{};
        decode_per(*pIe.llcConfig, pCtx);
    }
    if (check_optional(optionalmask, 2))
    {
        pIe.pdcpConfig = decltype(pIe.pdcpConfig)::value_type{};
        decode_per(*pIe.pdcpConfig, pCtx);
    }
}

inline void str(const char* pName, const RRC_ExchangeRequest& pIe, std::string& pCtx, bool pIsLast)
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
    if (pIe.llcConfig) nOptional++;
    if (pIe.pdcpConfig) nOptional++;
    size_t nMandatory = 0;
    if (pIe.frameConfig)
    {
        str("frameConfig", *pIe.frameConfig, pCtx, !(nMandatory+--nOptional));
    }
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

inline void encode_per(const RRC_ExchangeResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    if (pIe.frameConfig)
    {
        set_optional(optionalmask, 0);
    }
    if (pIe.llcConfig)
    {
        set_optional(optionalmask, 1);
    }
    if (pIe.pdcpConfig)
    {
        set_optional(optionalmask, 2);
    }
    encode_per(optionalmask, sizeof(optionalmask), pCtx);
    if (pIe.frameConfig)
    {
        encode_per(*pIe.frameConfig, pCtx);
    }
    if (pIe.llcConfig)
    {
        encode_per(*pIe.llcConfig, pCtx);
    }
    if (pIe.pdcpConfig)
    {
        encode_per(*pIe.pdcpConfig, pCtx);
    }
}

inline void decode_per(RRC_ExchangeResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    uint8_t optionalmask[1] = {};
    decode_per(optionalmask, sizeof(optionalmask), pCtx);
    if (check_optional(optionalmask, 0))
    {
        pIe.frameConfig = decltype(pIe.frameConfig)::value_type{};
        decode_per(*pIe.frameConfig, pCtx);
    }
    if (check_optional(optionalmask, 1))
    {
        pIe.llcConfig = decltype(pIe.llcConfig)::value_type{};
        decode_per(*pIe.llcConfig, pCtx);
    }
    if (check_optional(optionalmask, 2))
    {
        pIe.pdcpConfig = decltype(pIe.pdcpConfig)::value_type{};
        decode_per(*pIe.pdcpConfig, pCtx);
    }
}

inline void str(const char* pName, const RRC_ExchangeResponse& pIe, std::string& pCtx, bool pIsLast)
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
    if (pIe.llcConfig) nOptional++;
    if (pIe.pdcpConfig) nOptional++;
    size_t nMandatory = 0;
    if (pIe.frameConfig)
    {
        str("frameConfig", *pIe.frameConfig, pCtx, !(nMandatory+--nOptional));
    }
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
    encode_per(pIe.spare, pCtx);
}

inline void decode_per(RRC_PushResponse& pIe, cum::per_codec_ctx& pCtx)
{
    using namespace cum;
    decode_per(pIe.spare, pCtx);
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
    str("spare", pIe.spare, pCtx, !(--nMandatory+nOptional));
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
    else if (4 == type)
    {
        encode_per(std::get<4>(pIe), pCtx);
    }
    else if (5 == type)
    {
        encode_per(std::get<5>(pIe), pCtx);
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
    else if (4 == type)
    {
        pIe = RRC_ExchangeRequest();
        decode_per(std::get<4>(pIe), pCtx);
    }
    else if (5 == type)
    {
        pIe = RRC_ExchangeResponse();
        decode_per(std::get<5>(pIe), pCtx);
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
    else if (4 == type)
    {
        if (pName)
            pCtx += "\"" + std::string(pName) + "\":{";
        else
            pCtx += "{";
        std::string name = "RRC_ExchangeRequest";
        str(name.c_str(), std::get<4>(pIe), pCtx, true);
        pCtx += "}";
    }
    else if (5 == type)
    {
        if (pName)
            pCtx += "\"" + std::string(pName) + "\":{";
        else
            pCtx += "{";
        std::string name = "RRC_ExchangeResponse";
        str(name.c_str(), std::get<5>(pIe), pCtx, true);
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
