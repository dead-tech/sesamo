#ifndef SESAMO_SERIAL_HPP
#define SESAMO_SERIAL_HPP

#include <atomic>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>
#include <vector>

class [[nodiscard]] Serial final
{
  public:
    static auto open(const std::filesystem::path &path, const int baud_rate)
      -> std::optional<std::shared_ptr<Serial>>;

    ~Serial();

    Serial(const Serial &serial)            = delete;
    Serial &operator=(const Serial &serial) = delete;

    Serial(Serial &&other) noexcept
    {
        swap(*this, other);
    }

    Serial &operator=(Serial &&other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    void close();

    [[nodiscard]] std::vector<std::string> read_all();

  private:
    explicit Serial(int fd);

    friend void swap(Serial& lhs, Serial& rhs) noexcept
    {
        lhs.connected    = rhs.connected.load(std::memory_order_relaxed);
        lhs.stop_reading = rhs.stop_reading.load(std::memory_order_relaxed);
        std::swap(lhs.fd, rhs.fd);
        std::swap(lhs.read_thread, rhs.read_thread);
    }

    int                      fd = -1;
    std::atomic<bool>        connected;
    std::thread              read_thread;
    std::mutex               read_buffer_mutex;
    std::vector<std::string> read_buffer;
    std::atomic<bool>        stop_reading;

    void read_loop();
};

#endif // SESAMO_SERIAL_HPP
