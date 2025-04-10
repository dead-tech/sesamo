#include "Serial.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <format>
#include <cstring>

auto SerialChannel::open(const std::filesystem::path& path, const int baud_rate)
-> std::optional<SerialChannel>
{
    const auto fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_SYNC);

    struct termios tty;
    if (tcgetattr(fd, &tty) < 0) {
        std::cerr << std::format("[ERROR] failed to get tty current attributes: {}\n", strerror(errno));
        return std::nullopt;
    }

    cfsetospeed(&tty, static_cast<speed_t>(baud_rate));
    cfsetispeed(&tty, static_cast<speed_t>(baud_rate));

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << std::format("[ERROR] failed to set tty attributes: {}\n", strerror(errno));
        return std::nullopt;
    }

    return std::make_optional(SerialChannel(fd));
}

auto SerialChannel::read() const -> std::optional<std::string>
{
    std::stringstream ss;
    for (;;) {
        char msg[256];
        const auto bytes = ::read(fd, &msg, 256);
        if (bytes < 0) {
            std::cerr << std::format("[ERROR] failed to read from serial port: {}\n", strerror(errno));
            return std::nullopt;
        }

        ss << msg;
        memset(msg, 0, 256);
        if (bytes == 0) break;
    }

    return ss.str();
}

void SerialChannel::close() const
{
    ::close(fd);
}

