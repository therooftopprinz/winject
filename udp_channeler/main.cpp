#include <regex>
#include <sstream>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <cstdlib>
#include <condition_variable>

#include <bfc/Udp.hpp>

using options_t = std::map<std::string, std::string>;

using buffer_t = std::vector<uint8_t>;
std::deque<buffer_t> buffer_list;
std::mutex buffer_list_mutex;
std::condition_variable buffer_list_cv;

uint64_t now_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

template<typename T, typename U, typename V>
const V value_or(const T& t, const U& u, const V& v)
{
    if (!t.count(u))
    {
        return v;
    }
    return t.at(u);
}

template<typename... Ts>
void log(const char* msg, Ts... ts)
{
    char buff[1024*64];
    snprintf(buff,sizeof(buff),msg,ts...);
    printf("%lu | %04lu | %s\n", now_us(), std::hash<std::thread::id>{}(std::this_thread::get_id()) % 1024, buff);
}

int main(int argc, char* argv[])
{
    std::regex arger("^--(.+?)=(.+?)$");
    std::smatch match;
    options_t options; 

    for (int i=1; i<argc; i++)
    {
        auto s = std::string(argv[i]);
        if (std::regex_match(s, match, arger))
        {
            options.emplace(match[1].str(), match[2].str());
        }
        else
        {
            throw std::runtime_error(std::string("invalid argument: `") + argv[i] + "`");
        }
    }

    using std::string_literals::operator""s;

    auto from = bfc::toIp4Port(options.at("rx"));
    auto to = bfc::toIp4Port(options.at("tx"));
    auto packrate_s = std::stoul(value_or(options,"rate","0"s));
    auto failrate_pct = std::stold(value_or(options,"fail","0"s));
    auto jitter_us    = std::stoul(value_or(options,"jitter","0"s));
    auto seed         = std::stoul(value_or(options,"seed","0"s));
    auto qmax         = std::stoul(value_or(options,"qmax","0"s));

    if (!options.count("seed"))
    {
        seed = std::time(nullptr);
    }

    log("seed %lu", seed);

    std::srand(seed);

    bfc::UdpSocket sock;
    if (0 > sock.bind(from))
    {
        std::stringstream ss;
        ss << "bind failed: " << strerror(errno);
        throw std::runtime_error(ss.str());
    }
    
    log("receiving on %s", options.at("rx").c_str());
    log("sending on %s", options.at("tx").c_str());

    std::thread sender([&](){
            uint64_t wt = 0;

            if (packrate_s)
            {
                wt = double(1000000)/packrate_s;
            }

            log("sleep_time_us=%lu (%lu p/s)", wt, packrate_s);
            log("jitter_us=%lu", jitter_us);

            std::unique_lock<std::mutex> lg(buffer_list_mutex);
            while (true)
            {
                buffer_list_cv.wait(lg, [](){
                        return buffer_list.size();
                    });
                while (buffer_list.size())
                {
                    auto tosend = std::move(buffer_list.front());
                    buffer_list.pop_front();
                    lg.unlock();
                    bfc::ConstBufferView bv(tosend.data(), tosend.size());

                    bool fail = false;

                    if (failrate_pct > double(rand())*100/RAND_MAX)
                    {
                        fail = true;
                    }

                    log("sending fail=%d q_sz= %3lu sz= %3li", fail, buffer_list.size(), bv.size());
                    if (!fail)
                    {
                        auto rv = sock.sendto(bv, to);

                        if (0>rv)
                        {
                            std::stringstream ss;
                            ss << "send failed: " << strerror(errno);
                            throw std::runtime_error(ss.str());
                        }
                    }

                    int64_t jitter = 0;
                    
                    if (jitter_us)
                    {
                        jitter = (std::rand() % jitter_us);
                        jitter = jitter - (jitter_us/2);
                    }

                    auto lwt = wt + jitter;

                    std::this_thread::sleep_for(std::chrono::microseconds(lwt));

                    lg.lock();
                }
            }
        });

    while (true)
    {
        uint8_t btmp[1024*64];
        bfc::BufferView bv(btmp, sizeof(btmp));
        bfc::Ip4Port from;
        auto rv = sock.recvfrom(bv, from);
        if (0>rv)
        {
            std::stringstream ss;
            ss << "recv failed: " << strerror(errno);
            throw std::runtime_error(ss.str());
        }

        std::unique_lock<std::mutex> lg(buffer_list_mutex);
        if (qmax && buffer_list.size() > qmax)
        {
            continue;
        }

        buffer_list.emplace_back(btmp, btmp+rv);
        buffer_list_cv.notify_one();
    }
}