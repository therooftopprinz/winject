#ifndef __WINJECT_SAFEINT_HPP__
#define __WINJECT_SAFEINT_HPP__

#include <endian.h>
#include <cstdint>

namespace winject
{


template <typename T, T (*htole)(T), T (*letoh)(T)>
class safeint
{
public:
    safeint(T val)
        : value(htole(val))
    {}

    safeint& operator=(T val)
    {
        value = htole(val);
        return *this;
    }
    operator T() const
    {
        return letoh(value);
    }

    template <typename U>
    safeint operator&=(U val)
    {
        return operator T() & val;
    }

    template <typename U>
    safeint operator|=(U val)
    {
        return operator T() | val;
    }

private:
    T value;
}  __attribute__((__packed__));

inline uint16_t sf_htole16(uint16_t x) {return htole16(x);}
inline uint32_t sf_htole32(uint32_t x) {return htole32(x);}
inline uint64_t sf_htole64(uint64_t x) {return htole64(x);}
inline int16_t  sf_htole16i(int16_t x) {return htole16(x);}
inline int32_t  sf_htole32i(int32_t x) {return htole32(x);}
inline int64_t  sf_htole64i(int64_t x) {return htole64(x);}

inline uint16_t sf_htobe16(uint16_t x) {return htobe16(x);}
inline uint32_t sf_htobe32(uint32_t x) {return htobe32(x);}
inline uint64_t sf_htobe64(uint64_t x) {return htobe64(x);}
inline int16_t  sf_htobe16i(int16_t x) {return htobe16(x);}
inline int32_t  sf_htobe32i(int32_t x) {return htobe32(x);}
inline int64_t  sf_htobe64i(int64_t x) {return htobe64(x);}

inline uint16_t sf_le16toh(uint16_t x) {return le16toh(x);}
inline uint32_t sf_le32toh(uint32_t x) {return le32toh(x);}
inline uint64_t sf_le64toh(uint64_t x) {return le64toh(x);}
inline int16_t  sf_le16tohi(int16_t x) {return le16toh(x);}
inline int32_t  sf_le32tohi(int32_t x) {return le32toh(x);}
inline int64_t  sf_le64tohi(int64_t x) {return le64toh(x);}

inline uint16_t sf_be16toh(uint16_t x) {return be16toh(x);}
inline uint32_t sf_be32toh(uint32_t x) {return be32toh(x);}
inline uint64_t sf_be64toh(uint64_t x) {return be64toh(x);}
inline int16_t  sf_be16tohi(int16_t x) {return be16toh(x);}
inline int32_t  sf_be32tohi(int32_t x) {return be32toh(x);}
inline int64_t  sf_be64tohi(int64_t x) {return be64toh(x);}

using LEU16 = safeint<uint16_t, &sf_htole16, &sf_le16toh>;
using LEU32 = safeint<uint32_t, &sf_htole32, &sf_le32toh>;
using LEU64 = safeint<uint64_t, &sf_htole64, &sf_le64toh>;
using LEI16 = safeint<int16_t,  &sf_htole16i, &sf_le16tohi>;
using LEI32 = safeint<int32_t,  &sf_htole32i, &sf_le32tohi>;
using LEI64 = safeint<int64_t,  &sf_htole64i, &sf_le64tohi>;
using BEU16 = safeint<uint16_t, &sf_htobe16, &sf_be16toh>;
using BEU32 = safeint<uint32_t, &sf_htobe32, &sf_be32toh>;
using BEU64 = safeint<uint64_t, &sf_htobe64, &sf_be64toh>;
using BEI16 = safeint<int16_t,  &sf_htobe16i, &sf_be16tohi>;
using BEI32 = safeint<int32_t,  &sf_htobe32i, &sf_be32tohi>;
using BEI64 = safeint<int64_t,  &sf_htobe64i, &sf_be64tohi>;

} // namespace winject


#endif // __WINJECT_SAFEINT_HPP__