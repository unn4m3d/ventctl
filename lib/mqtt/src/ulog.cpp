#include <ulog.hpp>

static ulog::callback_t log_callback;

void ulog::set_callback(ulog::callback_t& cb)
{
    log_callback = cb;
}

void ulog::log(ulog::log_level level, const ulog::string_t& str)
{
    log_callback(level, str);
}