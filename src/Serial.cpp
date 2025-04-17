#include "Serial.hpp"

#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <mutex>
#include <termios.h>
#include <unistd.h>

auto Serial::open(const std::filesystem::path &path, const int baud_rate)
    -> std::optional<std::shared_ptr<Serial>>
{
    // NOLINTNEXTLINE
    const auto fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        std::cerr << std::format(
            "[ERROR] failed to open serial port {}: {}\n", path.string(), strerror(errno)
        );
        return std::nullopt;
    }

    struct termios tty{};
    if (tcgetattr(fd, &tty) < 0) {
        std::cerr << std::format(
            "[ERROR] failed to get tty current attributes for {}: {}\n", path.string(), strerror(errno)
        );
        ::close(fd);
        return std::nullopt;
    }

    cfsetospeed(&tty, static_cast<speed_t>(baud_rate));
    cfsetispeed(&tty, static_cast<speed_t>(baud_rate));

    tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8; /* 8-bit characters */
    tty.c_cflag &= ~PARENB; /* no parity bit */
    tty.c_cflag &= ~CSTOPB; /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &=
        ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10; // Adjust timeout as needed

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << std::format(
            "[ERROR] failed to set tty attributes for {}: {}\n", path.string(), strerror(errno)
        );
        ::close(fd);
        return std::nullopt;
    }

    auto serial_instance = std::shared_ptr<Serial>(new Serial(fd));
    serial_instance->read_thread = std::thread(&Serial::read_loop, serial_instance);
    serial_instance->connected.store(true, std::memory_order_relaxed);
    serial_instance->stop_reading.store(false, std::memory_order_relaxed);

    return serial_instance;
}

Serial::Serial(int fd) : fd(fd), connected(false), stop_reading(false) {}

Serial::~Serial()
{
    close();
}

void Serial::close()
{
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
    connected.store(false, std::memory_order_relaxed);
    stop_reading.store(true, std::memory_order_relaxed);
    if (read_thread.joinable()) {
        read_thread.join();
    }
}

void Serial::read_loop()
{
    while (!stop_reading.load(std::memory_order_relaxed)) {
        char msg[256];
        const auto bytes = ::read(fd, static_cast<char*>(msg), sizeof(msg));
        if (bytes > 0) {
            std::string received_data(static_cast<char*>(msg), bytes);
            {
                std::lock_guard<std::mutex> lock(read_buffer_mutex);
                read_buffer.push_back(received_data);
            }
        } else if (bytes < 0) {
            std::cerr << std::format(
                "[ERROR] failed to read from serial port: {}\n", strerror(errno)
            );
            // FIXME: Consider what to do on read error: try to reconnect, signal error, etc.
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

std::vector<std::string> Serial::read_all()
{
    std::vector<std::string> temp_buffer;
    {
        std::lock_guard<std::mutex> lock(read_buffer_mutex);
        temp_buffer = read_buffer;
    }
    read_buffer.clear();
    return temp_buffer;
}
