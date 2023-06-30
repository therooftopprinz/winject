#include <winject/wifi.hpp>
#include <winject/radiotap.hpp>
#include <winject/802_11.hpp>
#include <winject/802_11_filters.hpp>

#include <regex>
#include <thread>
#include <chrono>

using options_t = std::map<std::string, std::string>;

template<typename... Ts>
void LOG(const char* msg, Ts&&... args)
{
    char buffer[1024*64];
    snprintf(buffer,sizeof(buffer), msg, args...);
    uint64_t ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    printf("[%lu] %s\n", ts, buffer);
    fflush(stdout);
}


std::string buffer_str(uint8_t* buffer, size_t size)
{
    std::stringstream bufferss;
    for (int i=0; i<size; i++)
    {
        if ((i%16) == 0)
        {
            bufferss << std::hex << std::setfill('0') << std::setw(4) << i << " - ";

            for (int ii=0; ii<16 && (buffer+i+ii) < (buffer+size); ii++)
            {
                char c = buffer[i+ii];
                c = (c>=36 && c<=126) ? c : '.';
                bufferss << std::dec << c;
            }

            if ((size-i) < 16)
            {
                int rem = 16-size%16;
                for (int ii=0; ii <= rem; ii++)
                {
                    bufferss << " ";
                }
            }
            else
            {
                bufferss << " ";
            }
        }

        if (i%16 == 8)
        {
            bufferss << " ";
        }

        bufferss << std::hex << std::setfill('0') << std::setw(2) << (int) buffer[i] << " ";
        
        if (i%16 == 15)
        {
            bufferss << "\n";
        }
    }
    return bufferss.str();
}

struct checker_frame_t
{
    checker_frame_t (uint8_t* base, size_t size)
        : base(base)
        , actual_size(size)
    {
        rescan();
    }

    void rescan()
    {
        auto ptr = base;
        id = (uint64_t*)ptr;
        ptr += sizeof(*id);
        expected_size = (uint16_t*)ptr;
        ptr += sizeof(*expected_size);
        data = ptr;
    }

    size_t frame_size()
    {
        return actual_size;
    }

    size_t payload_size()
    {
        return frame_size()-sizeof(*id)-sizeof(*expected_size);
    }

    uint8_t* base;

    uint64_t* id;
    uint16_t* expected_size;
    uint8_t* data;

private:
    size_t actual_size;
};

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

    std::string host = options.at("host");
    int port = std::stol(options.at("port"));

    uint8_t buffer[1024*4];
    uint8_t wiffer[1024*4];
    int sd  = socket(AF_INET, SOCK_DGRAM, 0);

    if (options.at("mode")=="recv")
    {
        int stats_int = std::stol(options.at("stats"));
        sockaddr_in bindaddr;
        std::memset(&bindaddr, 0, sizeof(bindaddr));
        bindaddr.sin_family = AF_INET;
        bindaddr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &(bindaddr.sin_addr));

        auto rv = bind(sd, (sockaddr*)&bindaddr, sizeof(bindaddr));

        if (rv<0)
        {
            LOG("bind failed! errno=%d error=%s\n", errno, strerror(errno));
            return -1;
        }

        int sn = 0;
        uint64_t last_id=-1;

        int stats_pkt_rcv = 0;
        int stats_byt_rcv = 0;
        int stats_ebt_rcv = 0;
        int stats_pkt_mis = 0;
        uint64_t stats_time = std::chrono::high_resolution_clock::now().time_since_epoch().count()/(1000*1000*1000);

        while (true)
        {
            auto rv = recv(sd, buffer, sizeof(buffer), 0);
            if (rv<0)
            {
                LOG("recv failed! errno=%d error=%s\n", errno, strerror(errno));
                return -1;
            }
            if (rv==0)
            {
                continue;
            }

            stats_pkt_rcv++;
            stats_byt_rcv += rv;
            uint64_t this_time = std::chrono::high_resolution_clock::now().time_since_epoch().count()/(1000*1000*1000);
            if (this_time > (stats_time+stats_int-1))
            {
                stats_time = this_time;
                LOG("STATS: rcvd_packets:%4lf tp_Mbits:%3.3lf, missd_packets:%.3lf err_Kbits:%3.3lf",
                    double(stats_pkt_rcv)/stats_int,
                    double(stats_byt_rcv)*8/(1000*1000*stats_int),
                    double(stats_pkt_mis)/stats_int,
                    double(stats_ebt_rcv)*8/(1000*stats_int));
                stats_pkt_rcv = 0;
                stats_byt_rcv = 0;
                stats_pkt_mis = 0;
                stats_ebt_rcv = 0;
            }

            checker_frame_t checker(buffer, rv);
            auto& current_id = *(checker.id);

            int errors=0;
            for (size_t i=0; i<checker.payload_size(); i++)
            {
                errors += (checker.data[i] != (i&0xFF)) ? 1 : 0;
            }

            uint64_t diff = 0;
            if (last_id > current_id)
            {
                diff = 0xFFFFFFFFFFFFFFFF;
                diff -= last_id;
                diff += current_id;
            }
            else
            {
                diff = current_id - last_id; 
            }

            if (diff>1 || errors)
            {


                stats_pkt_mis += (diff-1);
                stats_ebt_rcv += errors;
                LOG("ERROR: prev=%6lu curr=%6lu diff=%6lu errors=%3lu", last_id, current_id, diff, errors);
            }

            last_id = *(checker.id);   
        }
    }
    else if (options.at("mode")=="send")
    {
        size_t size = std::stoul(options.at("size"));
        // kbps rate
        double rate = std::stod(options.at("rate"));

        double byte_rate = (rate*1000)/8;
        double send_rate = byte_rate/size;
        double send_period = 1/send_rate;
        uint64_t send_period_us = send_period*1000*1000;

        LOG("bitrate=%8.3lf bps send_rate=%.3lf Hz", rate*1000, send_rate);

        sockaddr_in bindaddr;
        sockaddr_in sendaddr;
        std::memset(&sendaddr, 0, sizeof(sendaddr));
        sendaddr.sin_family = AF_INET;
        sendaddr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &(sendaddr.sin_addr));
    
        uint64_t id=0;
        while (true)
        {
            checker_frame_t checker(buffer, size);
            *(checker.id)=id++;
            *(checker.expected_size) = checker.payload_size();

            for (size_t i=0; i<checker.payload_size(); i++)
            {
                checker.data[i] = i;
            }

            int rv = sendto(sd, buffer, size, 0, (sockaddr*)&sendaddr, sizeof(sendaddr));
            if (rv < 0)
            {
                LOG("send failed! errno=%d error=%s", errno, strerror(errno));
                return -1;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(send_period_us));
        }
    }
    else
    {
        std::stringstream ss;
        ss << "invalid mode: " << options.at("mode");
        throw std::runtime_error(ss.str());
    }
}