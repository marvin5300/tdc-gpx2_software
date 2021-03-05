#ifndef GPIO_H
#define GPIO_H
#include <vector>
#include <shared_mutex>
#include <chrono>
#include <future>
#include <map>
extern "C" {
#include <gpiod.h>
}

// TODO: check correctness of mutex'
// TODO: implement and check init, step, shutdown, write functions

enum class PinBias : std::uint8_t {
    OpenDrain = 0x01,
    OpenSource = 0x02,
    ActiveLow = 0x04,
    PullDown = 0x10,
    PullUp = 0x20
};

class gpio
{

public:
    struct event
    {
        enum Type
        {
            Invalid,
            Rising,
            Falling
        } type{ Invalid };

        std::size_t pin{};

        operator bool()
        {
            return type != Invalid;
        }
    };

    struct setting
    {
        [[nodiscard]] auto matches(const event& e) const -> bool;
        std::vector<std::size_t> gpio_pins;
    };

    class callback
    {
        friend class gpio;
    public:
        [[nodiscard]] auto wait_async(std::chrono::milliseconds timeout)->std::future<event>;
        [[nodiscard]] auto wait(std::chrono::milliseconds timeout)->event;

        [[nodiscard]] auto write_async(const event& e)->std::future<bool>;
        [[nodiscard]] auto write(const event& e) -> bool;

        callback(setting s, std::shared_ptr<gpio> handler); // somehow can't put it in private because at some point std::make_shared<callback>(..) is called

    private:
        void notify(const event& e);

        setting m_setting{};
        event m_event {};
        std::condition_variable m_wait{};
        std::mutex m_wait_mutex{};
        std::shared_mutex m_access_mutex{};
        std::shared_ptr<gpio> m_handler;
    };

    virtual ~gpio();

    void start();

    void stop();

    void join();

    [[nodiscard]] auto list_callback(setting s)->std::shared_ptr<callback>;
    void remove_callback(std::size_t id);

private:
    [[nodiscard]] auto setup() -> int;
    [[nodiscard]] auto step() -> int;
    [[nodiscard]] auto shutdown() -> int;
    [[nodiscard]] auto write(const event& e) -> bool;

    void notify_all(event e);

    std::string consumer{ "gpio" };
    std::string chipname{ "gpiochip0" };
    //std::unique_ptr<gpiod_chip> chip{};
    gpiod_chip* chip{};
    //std::vector<std::unique_ptr<gpiod_line_bulk> > lines{};
    gpiod_line_bulk* lines{};

    //inline static std::size_t global_id_counter{ 0 };
    std::size_t global_id_counter{ 0 };
    std::map < std::size_t, std::shared_ptr<callback> >  m_callback{};

    std::future<int> m_result{};

    std::chrono::milliseconds m_timeout{ 5U };

    std::atomic<bool> m_run{ true };

    std::vector<setting> m_settings{};

    std::mutex m_gpio_mutex{};
};

#endif // GPIO_H