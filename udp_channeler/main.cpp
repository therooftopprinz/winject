#include <regex>
#include <sstream>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
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

    auto from = bfc::toIp4Port(options.at("rx"));
    auto to = bfc::toIp4Port(options.at("tx"));
    auto packrate = std::stoul(options.at("pr"));
    // auto failrate = std::stoul(options.at("fr"));
    // auto errorate = std::stoul(options.at("er"));

    bfc::UdpSocket sock;
    if (0 > sock.bind(from))
    {
        std::stringstream ss;
        ss << "bind failed: " << strerror(errno);
        throw std::runtime_error(ss.str());
    }

    std::thread sender([&](){
            if (!packrate)
            {
                return;
            }

            uint64_t wt = double(1000000)/packrate;
            printf("sleep_time_us=%lu\n", wt);
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
                    auto rv = sock.sendto(bv, to);
                    if (0>rv)
                    {
                        std::stringstream ss;
                        ss << "send failed: " << strerror(errno);
                        throw std::runtime_error(ss.str());
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(wt));
                    lg.lock();
                    printf("%15lu sending[sd:%d q_sz:%5lu] %li\n", now_us(), sock.handle(), buffer_list.size(), bv.size());
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
        if (packrate)
        {
            std::unique_lock<std::mutex> lg(buffer_list_mutex);
            buffer_list.emplace_back(btmp, btmp+rv);
            buffer_list_cv.notify_one();
        }
        else
        {
            bfc::ConstBufferView bv(btmp, rv);
            printf("%15lu sending %li\n", now_us(), rv);
            auto rv = sock.sendto(bv, to);
            if (0>rv)
            {
                std::stringstream ss;
                ss << "send failed: " << strerror(errno);
                throw std::runtime_error(ss.str());
            }
        }
    }
}