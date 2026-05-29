
#include <boost/asio/any_io_executor.hpp>
#include <zmq_async.hpp>
#include <boost/asio/steady_timer.hpp>


// Why is this causing compile time error with abysmal messages here but not in my personal project?
// I don't know, disabling it for now.
#if 0 
namespace ex = stdexec;
struct Timer
{
    explicit Timer(std::chrono::milliseconds sec, boost::asio::any_io_executor ex) :
        m_timer{ex, sec}
    {
    }


    boost::asio::steady_timer m_timer;

    using sender_concept = ex::sender_t;
    using completion_signatures =
      ex::completion_signatures<ex::set_value_t(),
                                ex::set_error_t(boost::system::error_code),
                                ex::set_stopped_t()>;

    template<ex::receiver Receiver>
    ex::operation_state auto connect(Receiver &&recv) &&
    {
        struct timer_op
        {
            struct cancel_cb
            {
                timer_op &self;
                auto operator()() noexcept
                {
                    self.m_timer.cancel();
                    std::move(self.m_recv).set_stopped();
                }
            };

            using stop_callback_t =
              ex::stop_callback_for_t<ex::stop_token_of_t<ex::env_of_t<Receiver>>,
                                      cancel_cb>;

            Receiver m_recv;
            boost::asio::steady_timer m_timer;
            std::optional<stop_callback_t> m_stop_callback{};

          public:
            void start() noexcept
            {
                auto st = ex::get_stop_token(ex::get_env(m_recv));

                if (st.stop_possible())
                    m_stop_callback.emplace(std::move(st), cancel_cb{*this});

                m_timer.async_wait([this](boost::system::error_code err) {
                    m_stop_callback.reset();
                    if (!err) {
                        std::move(m_recv).set_value();
                    } else {
                        std::move(m_recv).set_error(err);
                    }
                });
            }
        };

        return timer_op{.m_recv = std::move(recv), .m_timer = std::move(m_timer)};
    }
};
static_assert(ex::sender<Timer>);
#endif