#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <thread>
#include <chrono>
#include <bitset>
#include <shared_mutex>

#include <unistd.h>

using BufferLog = std::pair<uint16_t, const void*>;

inline std::string toHexString(const uint8_t* pData, size_t size)
{
    std::stringstream ss;;
    for (size_t i=0; i<size; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << unsigned(pData[i]);
    }

    return ss.str();
}

#define _ static constexpr
template<typename>   struct TypeTraits;
template<typename T> struct TypeTraits<T*>       {_ auto type_id = 0xad; _ size_t size =  sizeof(void*);              _ char cfmt[] = "%p";};
template<> struct TypeTraits<unsigned char>      {_ auto type_id = 0xa1; _ size_t size =  sizeof(unsigned char);      _ char cfmt[] = "%c";};
template<> struct TypeTraits<signed char>        {_ auto type_id = 0xa2; _ size_t size =  sizeof(signed char);        _ char cfmt[] = "%c";};
template<> struct TypeTraits<unsigned short>     {_ auto type_id = 0xa3; _ size_t size =  sizeof(unsigned short);     _ char cfmt[] = "%u";};
template<> struct TypeTraits<short>              {_ auto type_id = 0xa4; _ size_t size =  sizeof(short);              _ char cfmt[] = "%d";};
template<> struct TypeTraits<unsigned int>       {_ auto type_id = 0xa5; _ size_t size =  sizeof(unsigned int);       _ char cfmt[] = "%u";};
template<> struct TypeTraits<int>                {_ auto type_id = 0xa6; _ size_t size =  sizeof(int);                _ char cfmt[] = "%d";};
template<> struct TypeTraits<unsigned long>      {_ auto type_id = 0xa7; _ size_t size =  sizeof(unsigned long);      _ char cfmt[] = "%lu";};
template<> struct TypeTraits<long>               {_ auto type_id = 0xa8; _ size_t size =  sizeof(long);               _ char cfmt[] = "%ld";};
template<> struct TypeTraits<unsigned long long> {_ auto type_id = 0xa9; _ size_t size =  sizeof(unsigned long long); _ char cfmt[] = "%llu";};
template<> struct TypeTraits<long long>          {_ auto type_id = 0xaa; _ size_t size =  sizeof(long long);          _ char cfmt[] = "%lld";};
template<> struct TypeTraits<float>              {_ auto type_id = 0xab; _ size_t size =  sizeof(float);              _ char cfmt[] = "%f";};
template<> struct TypeTraits<double>             {_ auto type_id = 0xac; _ size_t size =  sizeof(double);             _ char cfmt[] = "%f";};
template<> struct TypeTraits<BufferLog>          {_ auto type_id = 0xae; _ size_t size = 0;                           _ char cfmt[] = "%s";};
template<> struct TypeTraits<const char*>        {_ auto type_id = 0xaf; _ size_t size = 0;                           _ char cfmt[] = "%s";};
template<> struct TypeTraits<char*>              {_ auto type_id = 0xaf; _ size_t size = 0;                           _ char cfmt[] = "%s";};
#undef _

template <typename... Ts>
struct TotalSize
{
    static constexpr size_t value = (TypeTraits<Ts>::size+...);
};

template<>
struct TotalSize<>
{
    static constexpr size_t value = 0;
};

template <size_t LOGBITSZ>
class Logger
{
public:
    using HeaderType = int64_t;
    using TagType    = uint8_t;
    using TailType   = uint8_t;
    
    using logbit_t = std::bitset<LOGBITSZ>;

    Logger(const char* pFilename)
        : mOutputFile(std::fopen(pFilename, "wb"))
    {
    }

    ~Logger()
    {
        std::fclose(mOutputFile);
    }

    void set_logbit(bool value, size_t index)
    {
        std::unique_lock<std::shared_mutex> lg(this_mutex);
        logbits.set(index, value);
    }

    bool get_logbit(size_t index)
    {
        std::shared_lock<std::shared_mutex> lg(this_mutex);
        return logbits.test(index);
    }

    template<typename... Ts>
    void log(const char * id, uint64_t pTime, uint64_t pThread, const Ts&... ts)
    {
        if (mLogful)
        {
            uint8_t logbuff[4096*2];

            double dptime = double(pTime)/1000000;

            int flen = std::sprintf((char*)logbuff, "%.6lf us %lluT ", dptime, (unsigned long long)(pThread));
            size_t sz = logful(logbuff + flen, id, ts...) + flen;
            logbuff[sz++] = '\n';
            [[maybe_unused]] auto rv = ::write(1, logbuff, sz);
        }
        else
        {
            uint8_t buffer[2048];
            uint8_t* usedBuffer = buffer;
            new (usedBuffer) HeaderType(intptr_t(id)-intptr_t(LoggerRef));
            usedBuffer += sizeof(HeaderType);
            size_t sz = logless(usedBuffer, pTime, pThread, ts...) + sizeof(HeaderType);
            std::fwrite((char*)buffer, 1, sz, mOutputFile);
        }
    }
    void logful()
    {
        mLogful = true;
    }
    void logless()
    {
        mLogful = false;
    }
    void flush()
    {
        std::fflush(mOutputFile);
    }

private:

    static constexpr char rep_token = '#';

    const char* findNextToken(char pTok, const char* pStr)
    {
        while (*pStr!=0&&*pStr!=pTok)
        {
            pStr++;
        }
        return pStr;
    }

    size_t logful(uint8_t* pOut, const char* pMsg)
    {
        const char *nTok = findNextToken(rep_token, pMsg);
        size_t sglen = uintptr_t(nTok)-uintptr_t(pMsg);
        std::memcpy(pOut, pMsg, sglen);
        return sglen;
    }

    template<typename T, typename... Ts>
    size_t logful(uint8_t* pOut, const char* pMsg, T t, Ts... ts)
    {
        const char *nTok = findNextToken(rep_token, pMsg);
        size_t sglen = uintptr_t(nTok)-uintptr_t(pMsg);
        std::memcpy(pOut, pMsg, sglen);
        pMsg+=sglen;
        pOut+=sglen;
        int flen = 0;
        if (*nTok)
        {
            flen = std::sprintf((char*)pOut, TypeTraits<T>::cfmt, t);
            pMsg++;
        }
        if (flen>0) pOut += flen;
        return sglen + flen + logful(pOut, pMsg, ts...);
    }

    template<typename... Ts>
    size_t logful(uint8_t* pOut, const char* pMsg, BufferLog t, Ts... ts)
    {
        const char *nTok = findNextToken(rep_token, pMsg);
        size_t sglen = uintptr_t(nTok)-uintptr_t(pMsg);
        std::memcpy(pOut, pMsg, sglen);
        pMsg+=sglen;
        pOut+=sglen;
        int flen = 0;
        if (*nTok)
        {
            auto s = toHexString((uint8_t*)t.second, t.first);
            std::memcpy(pOut, s.data(), s.size());
            flen += s.size();
            pMsg++;
        }
        if (flen>0) pOut += flen;
        return sglen + flen + logful(pOut, pMsg, ts...);
    }

    template<typename... Ts>
    size_t logful(uint8_t* pOut, const char* pMsg, const char *t, Ts... ts)
    {
        const char *nTok = findNextToken(rep_token, pMsg);
        size_t sglen = uintptr_t(nTok)-uintptr_t(pMsg);
        std::memcpy(pOut, pMsg, sglen);
        pMsg+=sglen;
        pOut+=sglen;
        int flen = 0;
        if (*nTok)
        {
            auto s = std::string_view(t, std::strlen(t));
            std::memcpy(pOut, s.data(), s.size());
            flen += s.size();
            pMsg++;
        }
        if (flen>0) pOut += flen;
        return sglen + flen + logful(pOut, pMsg, ts...);
    }

    size_t logless(uint8_t* pUsedBuffer)
    {
        new (pUsedBuffer) TailType(0);
        return sizeof(TailType);
    }

    template<typename T, typename... Ts>
    size_t logless(uint8_t* pUsedBuffer, T t, Ts... ts)
    {
        new (pUsedBuffer) TagType(TypeTraits<T>::type_id);
        pUsedBuffer += sizeof(TagType);
        new (pUsedBuffer) T(t);
        pUsedBuffer += sizeof(T);
        return logless(pUsedBuffer, ts...) + sizeof(TagType) + sizeof(T);
    }

    template<typename... Ts>
    size_t logless(uint8_t* pUsedBuffer, BufferLog t, Ts... ts)
    {
        new (pUsedBuffer) TagType(TypeTraits<BufferLog>::type_id);
        pUsedBuffer += sizeof(TagType);
        new (pUsedBuffer) BufferLog::first_type(t.first);
        pUsedBuffer += sizeof(BufferLog::first_type);
        std::memcpy(pUsedBuffer, t.second, t.first);
        pUsedBuffer += t.first;
        return logless(pUsedBuffer, ts...) + sizeof(TagType) + sizeof(BufferLog::first_type) + t.first;
    }

    template<typename... Ts>
    size_t logless(uint8_t* pUsedBuffer, const char* t, Ts... ts)
    {
        new (pUsedBuffer) TagType(TypeTraits<const char*>::type_id);
        pUsedBuffer += sizeof(TagType);
        size_t tlen = strlen(t);
        new (pUsedBuffer) BufferLog::first_type(tlen);
        pUsedBuffer += sizeof(BufferLog::first_type);
        std::memcpy(pUsedBuffer, t, tlen);
        pUsedBuffer += tlen;
        return logless(pUsedBuffer, ts...) + sizeof(TagType) + sizeof(BufferLog::first_type) + tlen;
    }

    std::FILE* mOutputFile;
    bool mLogful = false;
    static const char* LoggerRef;

    logbit_t logbits;

    std::shared_mutex this_mutex;
};
template <typename Logger, typename... Ts>
void Logless_(Logger& logger, size_t logbit, bool flush, const char* id, Ts... ts)
{
    if (logger.get_logbit(logbit))
    {
        uint64_t timeNow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        uint64_t threadId = 100000 + std::hash<std::thread::id>()(std::this_thread::get_id())%100000;
        logger.log(id, timeNow, threadId, ts...);
        if (flush)
        {
            logger.flush();
        }
    }
}


template <typename Logger, typename... Ts>
void Logless(Logger& logger, size_t logbit, const char* id, Ts... ts)
{
    Logless_(logger, logbit, false, id, ts...);
}

template <typename Logger, typename... Ts>
void LoglessF(Logger& logger, size_t logbit, const char* id, Ts... ts)
{
    Logless_(logger, logbit, true, id, ts...);
}

template <typename Logger>
struct LoglessTrace
{
    LoglessTrace(Logger& pLogger, char* pName)
        : mName(pName)
        , mStart(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())
        , mLogger(pLogger)
    {
        Logless(mLogger, Logger::TRACE, "TRACE ENTER _", mName);
    }
    ~LoglessTrace()
    {
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        auto diff = now-mStart;

        Logless(mLogger, Logger::TRACE, "TRACE LEAVE _ TIME _", mName, diff);
    }
    const char* mName;
    uint64_t mStart;
    Logger& mLogger;
};

#define LOGLESS_TRACE(logger) LoglessTrace<std::remove_reference_t<decltype(logger)>> __trace(logger, __PRETTY_FUNCTION__)

#endif // __LOGGER_HPP__
