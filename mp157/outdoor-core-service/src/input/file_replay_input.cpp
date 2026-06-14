#include "input/file_replay_input.h"

#include <utility>

namespace outdoor::input {

FileReplayInput::FileReplayInput(std::string filePath)
    : filePath_(std::move(filePath)) {}

FileReplayInput::~FileReplayInput()
{
    close();
}

bool FileReplayInput::open()
{
    close();
    stream_.open(filePath_);
    return stream_.is_open();
}

bool FileReplayInput::readLine(std::string& line)
{
    if (!stream_.is_open()) {
        return false;
    }

    return static_cast<bool>(std::getline(stream_, line));
}

void FileReplayInput::close()
{
    if (stream_.is_open()) {
        stream_.close();
    }
}

const char* FileReplayInput::name() const
{
    return "file_replay";
}

const std::string& FileReplayInput::filePath() const
{
    return filePath_;
}

} // namespace outdoor::input
