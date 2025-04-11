#ifndef SESAMO_SERIAL_HPP
#define SESAMO_SERIAL_HPP

#include <string>
#include <filesystem>
#include <optional>

class [[nodiscard]] SerialChannel final
{
public:
    [[nodiscard]] static auto
    open(const std::filesystem::path& path, const int baud_rate)
    -> std::optional<SerialChannel>;

    [[nodiscard]] auto has_data_to_read() const -> bool;
    [[nodiscard]] auto read() const -> std::optional<std::string>;

    [[nodiscard]] auto is_connected() const -> bool;

    void close();


private:
    explicit SerialChannel(const int fd)
        : fd { fd }
    {}

public:
    int fd = -1;
    bool connected = true;
};

#endif // SESAMO_SERIAL_HPP
