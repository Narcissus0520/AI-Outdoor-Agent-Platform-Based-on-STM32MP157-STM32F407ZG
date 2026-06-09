#pragma once

#include <string>

namespace outdoor::input {

class IInputSource {
public:
    virtual ~IInputSource() = default;

    virtual bool open() = 0;
    virtual bool readLine(std::string& line) = 0;
    virtual void close() = 0;
    virtual const char* name() const = 0;
};

} // namespace outdoor::input
