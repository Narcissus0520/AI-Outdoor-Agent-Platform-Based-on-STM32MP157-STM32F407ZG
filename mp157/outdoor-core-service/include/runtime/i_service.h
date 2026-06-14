#pragma once

namespace outdoor::runtime {

class IService {
public:
    virtual ~IService() = default;

    virtual const char* name() const = 0;
    virtual bool start() = 0;
    virtual bool run() = 0;
    virtual void stop() = 0;
};

} // namespace outdoor::runtime
