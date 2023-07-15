#ifndef __BFC_IREACTOR_HPP__
#define __BFC_IREACTOR_HPP__

#include <functional>

namespace bfc
{

struct IReactor
{
public:

    using Callback = std::function<void()>;

    virtual ~IReactor() {}
    virtual bool addReadHandler(int pFd, Callback) = 0;
    virtual bool addWriteHandler(int pFd, Callback) = 0;
    virtual bool removeReadHandler(int pFd) = 0;
    virtual bool removeWriteHandler(int pFd) = 0;
};

} // namespace bfc

#endif // __BFC_IREACTOR_HPP__