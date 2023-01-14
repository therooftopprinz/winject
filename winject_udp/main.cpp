#include <winject/wifi.hpp>
#include <winject/radiotap.hpp>
#include <winject/802_11.hpp>
#include <winject/802_11_filters.hpp>

#include <regex>
#include <chrono>

using options_t = std::map<std::string, std::string>;

template<typename... Ts>
void LOG(const char* msg, Ts&&... args)
{
    char buffer[1024*64];
    snprintf(buffer,sizeof(buffer), msg, args...);
    uint64_t ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    printf("[%lu] %s\n", ts, buffer);
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

    std::string dev = options.at("dev");
    std::string host = options.at("host");
    int port = std::stol(options.at("port"));

    LOG("[config] host=%s", options.at("host").c_str());
    LOG("[config] port=%s", options.at("port").c_str());
    LOG("[config] mode=%s", options.at("mode").c_str());
    LOG("[config] dev=%s", options.at("dev").c_str());

    uint8_t buffer[1024*4];
    uint8_t wiffer[1024*4];
    int sd  = socket(AF_INET, SOCK_DGRAM, 0);
    if (options.at("mode")=="send")
    {
        winject::wifi wdev(dev);

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
        
        std::memset(wiffer, 0, sizeof(wiffer));
        winject::radiotap::radiotap_t radiotap(wiffer);
        radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_FLAGS;
        radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_RATE;
        radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_TX_FLAGS;
        radiotap.header->presence |= winject::radiotap::E_FIELD_PRESENCE_MCS;
        radiotap.rescan(true);
        radiotap.flags->flags |= winject::radiotap::flags_t::E_FLAGS_FCS;
        radiotap.rate->value = 65*2;
        radiotap.tx_flags->value |= winject::radiotap::tx_flags_t::E_FLAGS_NOACK;
        radiotap.tx_flags->value |= winject::radiotap::tx_flags_t::E_FLAGS_NOREORDER;
        radiotap.mcs->known |= winject::radiotap::mcs_t::E_KNOWN_BW;
        radiotap.mcs->known |= winject::radiotap::mcs_t::E_KNOWN_MCS;
        radiotap.mcs->known |= winject::radiotap::mcs_t::E_KNOWN_STBC;
        radiotap.mcs->mcs_index = 7;
        radiotap.mcs->flags |= winject::radiotap::mcs_t::E_FLAG_BW_20;
        radiotap.mcs->flags |= winject::radiotap::mcs_t::E_FLAG_STBC_1;

        auto radiotap_string = winject::radiotap::to_string(radiotap);
        LOG("--- radiotap header info ---\n%s", radiotap_string.c_str());

        winject::ieee_802_11::frame_t frame80211(radiotap.end());
        frame80211.frame_control->protocol_type = winject::ieee_802_11::frame_control_t::E_TYPE_DATA;
        frame80211.rescan();
        frame80211.address1->set(0xFFFFFFFFFFFF); // IBSS Destination
        std::stringstream srcss;
        std::stringstream dstss;
        uint64_t srcraw;
        uint64_t dstraw;
        srcss << std::hex << options.at("src");
        dstss << std::hex << options.at("dst");
        srcss >> srcraw;
        dstss >> dstraw;
        uint64_t address2 = (srcraw << 24) | (dstraw);
        LOG("[config] src=%s", options.at("src").c_str());
        LOG("[config] dst=%s", options.at("dst").c_str());
        frame80211.address2->set(address2); // IBSS Source
        frame80211.address3->set(0xDEADBEEFCAFE); // IBSS BSSID

        int sn = 0;
        while (true)
        {
            auto rv = recv(sd, buffer, sizeof(buffer), 0);
            LOG("recv from udp sn=%d size=%li ", sn, rv);
            if (rv<0)
            {
                LOG("recv failed! errno=%d error=%s\n", errno, strerror(errno));
                return -1;
            }
            if (rv==0)
            {
                continue;
            }
            frame80211.set_body_size(rv);
            frame80211.seq_ctl->set_seq_num(sn++);
            size_t size = radiotap.size() + frame80211.size() - 4;
            std::memcpy(frame80211.frame_body, buffer, rv);

            rv = sendto(wdev, wiffer, size, 0, (sockaddr *) &wdev.address(), sizeof(struct sockaddr_ll));
            if (rv < 0)
            {
                LOG("send failed! errno=%d error=%s\n", errno, strerror(errno));
                return -1;
            }
        }
    }
    else if (options.at("mode")=="recv")
    {
        winject::wifi wdev(dev);


        std::stringstream srcss;
        uint32_t srcraw;
        srcss << std::hex << options.at("src");
        srcss >> srcraw;
        LOG("[config] src=%s", options.at("src").c_str());
        // winject::ieee_802_11::filters::data_addr3 src_filter(0xDEADBEEFCAFE);
        winject::ieee_802_11::filters::winject_src src_filter(srcraw);
        winject::ieee_802_11::filters::attach(wdev, src_filter);

        sockaddr_in sendaddr;
        std::memset(&sendaddr, 0, sizeof(sendaddr));
        sendaddr.sin_family = AF_INET;
        sendaddr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &(sendaddr.sin_addr));

        while (true)
        {
            ssize_t rv = recv(wdev, wiffer, sizeof(wiffer), 0);
            if (rv < 0)
            {
                printf("recv failed! errno=%d error=%s\n", errno, strerror(errno));
                return -1;
            }

            winject::radiotap::radiotap_t radiotap(wiffer);
            winject::ieee_802_11::frame_t frame80211(radiotap.end());

            auto frame80211end = wiffer+rv;
            size_t size =  frame80211end-frame80211.frame_body-4;

            LOG("buffer[%lu]:", rv);
            std::stringstream bufferss;
            for (int i=0; i<rv; i++)
            {
                int psz;
                if (i%16 == 0)
                {
                    bufferss << "\n" << std::hex << std::setfill('0') << std::setw(4) << i << " - ";
                }
                bufferss << std::hex << std::setfill('0') << std::setw(2) << (int) wiffer[i] << " ";
            }

            LOG("%s\n", bufferss.str().c_str());

            LOG("recv size %lu", rv);
            LOG("--- radiotap info ---\n%s", winject::radiotap::to_string(radiotap).c_str());
            LOG("--- 802.11 info ---\n%s", winject::ieee_802_11::to_string(frame80211).c_str());
            LOG("  frame body size: %lu", size);
            LOG("-----------------------\n");

            if (!frame80211.frame_body)
            {
                continue;
            }

            std::memcpy(buffer, frame80211.frame_body, size);

            rv = sendto(sd, buffer, size, 0, (sockaddr*)&sendaddr, sizeof(sendaddr));
            if (rv < 0)
            {
                LOG("send failed! errno=%d error=%s", errno, strerror(errno));
                return -1;
            }
        }
    }
            
    else
    {
        std::stringstream ss;
        ss << "invalid mode: " << options.at("mode");
        throw std::runtime_error(ss.str());
    }

}