#pragma once

#include "input/i_input_source.h"

#include <fstream>
#include <string>

namespace outdoor::input {

class FileReplayInput final : public IInputSource {
public:
    explicit FileReplayInput(std::string filePath);
    ~FileReplayInput() override;

    bool open() override;
    bool readLine(std::string& line) override;
    void close() override;
    const char* name() const override;

    const std::string& filePath() const;

private:
    std::string filePath_;
    std::ifstream stream_;
};

} // namespace outdoor::input
