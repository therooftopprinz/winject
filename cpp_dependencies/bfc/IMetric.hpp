#ifndef __BFC_IMETRIC_HPP__
#define __BFC_IMETRIC_HPP__

#include <string>
#include <utility>
#include <map>

namespace bfc
{

// @todo replace with atomic_double in c++20
struct IMetric
{
    virtual ~IMetric() {}
    virtual void store(double) = 0;
    virtual double load() = 0;
    virtual double fetch_add(double) = 0;
    virtual double fetch_sub(double) = 0;
};

struct IMonitor
{
    virtual ~IMonitor() {}
    using tag_t = std::pair<std::string,std::string>;
    virtual IMetric& getMetric(const std::string& name) = 0;
};

} // namespace bfc

#endif // __BFC_IMETRIC_HPP__
