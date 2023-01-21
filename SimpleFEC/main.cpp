#include <schifra/schifra_galois_field.hpp>
#include <schifra/schifra_galois_field_polynomial.hpp>
#include <schifra/schifra_sequential_root_generator_polynomial_creator.hpp>
#include <schifra/schifra_reed_solomon_encoder.hpp>
#include <schifra/schifra_reed_solomon_decoder.hpp>
#include <schifra/schifra_reed_solomon_block.hpp>
#include <schifra/schifra_error_processes.hpp>

#include <winject/wifi.hpp>
#include <winject/radiotap.hpp>
#include <winject/802_11.hpp>
#include <winject/802_11_filters.hpp>

#include <regex>
#include <list>
#include <vector>
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

class SimpleFEC_LLC_t
{
public:
    SimpleFEC_LLC_t(uint8_t* base, size_t data_size, size_t fec_size)
        : data_size(data_size)
        , fec_size(fec_size)
    {
        datanumber = base;
        fecnumber = base + 1;
    }

    uint8_t* data()
    {
        return datanumber+2;
    }

    uint8_t* fec()
    {
        return data()+data_size;
    }

    uint8_t* end()
    {
        return datanumber + 2 + data_size + fec_size;
    }

    size_t size()
    {
        return 2 + data_size + fec_size;
    }
    uint8_t *datanumber;
    uint8_t *fecnumber;
    size_t data_size=0;
    size_t fec_size=0;
};

class packet_fragments_t
{
public:
packet_fragments_t(uint8_t* base)
    : header((uint32_t*)base)
{}

uint8_t get_sn()
{
    constexpr uint32_t mask = 0x0000007F;
    return get_header() & mask;
}

void set_sn(uint8_t sn)
{
    constexpr uint32_t mask = 0x0000007F;
    set_header((get_header() & ~mask) | (sn & mask));
}

bool get_is_last()
{
    constexpr uint32_t mask = 0x00000080;
    return get_header() & mask;
}

void set_is_last(bool val)
{
    constexpr uint32_t mask = 0x00000080;
    set_header((get_header() & ~mask) | (val ? mask : 0));
}

uint16_t get_offset()
{
    constexpr uint32_t mask = 0x000FFF00;
    return (get_header() & mask) >> 8;
}

void set_offset(uint16_t offset_)
{
    constexpr uint32_t mask = 0x000FFF00;
    uint32_t offset = uint32_t(offset_) << 8;
    set_header((get_header() & ~mask) | (offset &  mask));
}

uint16_t get_size()
{
    constexpr uint32_t mask = 0xFFF00000;
    return (get_header() & mask) >> 20;
}

void set_size(uint16_t offset_)
{
    constexpr uint32_t mask = 0xFFF00000;
    uint32_t offset = uint32_t(offset_) << 20;
    set_header((get_header() & ~mask) | (offset &  mask));
}

uint32_t get_header()
{
    return htonl(*header);
}

void set_header(uint32_t val)
{
    *header = ntohl(val);
}

static size_t header_size()
{
    return sizeof(uint32_t);
}

size_t fragment_size()
{
    return get_size()+header_size();
}

uint8_t* data()
{
    return (uint8_t*)header+header_size();
}

uint8_t* base()
{
    return (uint8_t*)header;
}

private:
    uint32_t *header;
};

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

int main(int argc, char* argv[])
{


    constexpr size_t fec_per_block = 3;
    constexpr size_t data_size = 223 * fec_per_block;
    constexpr size_t fec_size = 32 * fec_per_block;
    constexpr uint8_t nblockdelay = 0;

    const schifra::galois::field field(8, schifra::galois::primitive_polynomial_size06, schifra::galois::primitive_polynomial06);
    schifra::galois::field_polynomial generator_polynomial(field);
    if (!schifra::make_sequential_root_generator_polynomial(field, 120, 32, generator_polynomial))
    {
        LOG("Error - Failed to create sequential root generator!");
        return 1;
    }

   typedef schifra::reed_solomon::encoder<255,32> encoder_t;
   typedef schifra::reed_solomon::decoder<255,32> decoder_t;
   typedef schifra::reed_solomon::block<255,32> block_t;

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
        const encoder_t encoder(field, generator_polynomial);
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

        struct send_info_t
        {
            std::vector<uint8_t> buffer;
            std::unique_ptr<winject::radiotap::radiotap_t> radiotap;
            std::unique_ptr<winject::ieee_802_11::frame_t> frame80211;
            std::unique_ptr<SimpleFEC_LLC_t> simple_fec;
            uint16_t offset;
        };

        LOG("[config] src=%s", options.at("src").c_str());
        LOG("[config] dst=%s", options.at("dst").c_str());

        std::vector<send_info_t> send_info(256);
        for (auto& i : send_info)
        {
            i.buffer.resize(4096);
            i.radiotap = std::make_unique<winject::radiotap::radiotap_t>(i.buffer.data());
            i.radiotap->header->presence |= winject::radiotap::E_FIELD_PRESENCE_FLAGS;
            i.radiotap->header->presence |= winject::radiotap::E_FIELD_PRESENCE_RATE;
            i.radiotap->header->presence |= winject::radiotap::E_FIELD_PRESENCE_TX_FLAGS;
            i.radiotap->header->presence |= winject::radiotap::E_FIELD_PRESENCE_MCS;
            i.radiotap->rescan(true);
            // i.radiotap->flags->flags |= winject::radiotap::flags_t::E_FLAGS_FCS;
            i.radiotap->rate->value = 65*2;
            i.radiotap->tx_flags->value |= winject::radiotap::tx_flags_t::E_FLAGS_NOACK;
            i.radiotap->tx_flags->value |= winject::radiotap::tx_flags_t::E_FLAGS_NOREORDER;
            i.radiotap->mcs->known |= winject::radiotap::mcs_t::E_KNOWN_BW;
            i.radiotap->mcs->known |= winject::radiotap::mcs_t::E_KNOWN_MCS; 
            i.radiotap->mcs->known |= winject::radiotap::mcs_t::E_KNOWN_STBC;
            i.radiotap->mcs->mcs_index = 7;
            i.radiotap->mcs->flags |= winject::radiotap::mcs_t::E_FLAG_BW_20;
            i.radiotap->mcs->flags |= winject::radiotap::mcs_t::E_FLAG_STBC_1;

            i.frame80211 = std::make_unique<winject::ieee_802_11::frame_t>(i.radiotap->end(), i.buffer.data()+i.buffer.size());
            i.frame80211->set_enable_fcs(false);
            i.frame80211->frame_control->protocol_type = winject::ieee_802_11::frame_control_t::E_TYPE_DATA;
            i.frame80211->rescan();
            i.frame80211->address1->set(0xFFFFFFFFFFFF); // IBSS Destination
            std::stringstream srcss;
            std::stringstream dstss;
            uint64_t srcraw;
            uint64_t dstraw;
            srcss << std::hex << options.at("src");
            dstss << std::hex << options.at("dst");
            srcss >> srcraw;
            dstss >> dstraw;
            uint64_t address2 = (srcraw << 24) | (dstraw);
            i.frame80211->address2->set(address2); // IBSS Source
            i.frame80211->address3->set(0xDEADBEEFCAFE); // IBSS BSSID

            i.simple_fec = std::make_unique<SimpleFEC_LLC_t>(i.frame80211->frame_body, data_size, fec_size);
            *(i.simple_fec->datanumber) = 0;
            *(i.simple_fec->fecnumber) = 0;

            i.frame80211->set_body_size(i.simple_fec->size());
            i.offset = 0;
        }

        uint8_t data_sn = 0;
        uint8_t udp_sn = 0;
        uint64_t sn = 0;
        std::list<send_info_t*> to_send;
        while (true)
        {
            auto rv = recv(sd, buffer, sizeof(buffer), 0);
            LOG("recv from udp sn=%d size=%li ", sn++, rv);
            if (rv<0)
            {
                LOG("recv failed! errno=%d error=%s\n", errno, strerror(errno));
                return -1;
            }

            if (rv==0)
            {
                continue;
            }

            auto udp_rem = rv;
            int sn = 0;

            while (true)
            {
                uint16_t udp_offset = rv - udp_rem;
                auto& si = send_info[data_sn];
                uint8_t* data_current = si.simple_fec->data() + si.offset;
                packet_fragments_t fragment(data_current);
                fragment.set_offset(udp_offset);
                auto data_rem = data_size - si.offset - packet_fragments_t::header_size();
                
                LOG("udp[%3d : %3d]->data[%3d : %3d]: udp_rem=%3d data_rem=%3d",
                    udp_sn, udp_offset,
                    data_sn, si.offset,
                    udp_rem, data_rem);

                fragment.set_sn(udp_sn);
                fragment.set_size(data_rem);
                fragment.set_is_last(true);

                size_t frag_size = 0;

                // @case : Still has UDP bytes left, take all data
                if (udp_rem > data_rem)
                {
                    fragment.set_is_last(false);

                    std::memcpy(fragment.data(), buffer+udp_offset, data_rem);

                    si.frame80211->seq_ctl->set_seq_num(sn);
                    *(si.simple_fec->datanumber) = data_sn;
                    *(si.simple_fec->fecnumber) = data_sn-nblockdelay;

                    auto& sifec = send_info[data_sn+nblockdelay];

                    for (int h=0; h<fec_per_block; h++)
                    {
                        // @note: FEC Encode
                        block_t block;
                        for (int i=0; i<223; i++)
                        {
                            block[i] = si.simple_fec->data()[h*223+i];
                        }
                        if (!encoder.encode(block))
                        {
                            LOG("Error - Critical encoding failure! Msg: %s", block.error_as_string().c_str());
                            return 1;
                        }
                        for (int i=0; i<32; i++)
                        {
                            sifec.simple_fec->fec()[h*32+i] = block.fec(i);
                        }
                    }

                    si.offset = 0;
                    data_sn = (data_sn+1) & 0x7F;

                    to_send.emplace_back(&si);
                    udp_rem -= data_rem;
                    continue;
                }
                // @case : All UDP bytes can fit
                else
                {
                    fragment.set_size(udp_rem);
                    std::memcpy(fragment.data(), buffer+udp_offset, udp_rem);

                    // @case : All data bytes are used
                    if ((data_rem-udp_rem) <= fragment.header_size())
                    {                        
                        *(si.simple_fec->datanumber) = data_sn;
                        *(si.simple_fec->fecnumber) = data_sn-nblockdelay;

                        auto& sifec = send_info[data_sn+nblockdelay];

                        for (int h=0; h<fec_per_block; h++)
                        {
                            // @note: FEC Encode
                            block_t block;
                            for (int i=0; i<223; i++)
                            {
                                block[i] = si.simple_fec->data()[h*223+i];
                            }
                            if (!encoder.encode(block))
                            {
                                LOG("Error - Critical encoding failure! Msg: %s", block.error_as_string().c_str());
                                return 1;
                            }
                            for (int i=0; i<32; i++)
                            {
                                sifec.simple_fec->fec()[h*223+i] = block.fec(i);
                            }
                        }

                        si.offset = 0;
                        data_sn = (data_sn+1) & 0x7F;
                        to_send.emplace_back(&si);
                        break;
                    }
                    si.offset += fragment.fragment_size();
                    break;
                }
            }

            udp_sn++;

            if (to_send.size())
            {
                LOG("Sending %d frames", to_send.size());
            }

            for (auto& i : to_send)
            {
                auto& si = *i;
                size_t size = si.radiotap->size()+si.frame80211->size(); 

                rv = sendto(wdev, si.buffer.data(), size, 0, (sockaddr *) &wdev.address(), sizeof(struct sockaddr_ll));
                if (rv < 0)
                {
                    LOG("send failed! errno=%d error=%s\n", errno, strerror(errno));
                    return -1;
                }
            }

            to_send.clear();
        }
    }
    else if (options.at("mode")=="recv")
    {
        const decoder_t decoder(field, 120);
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

        struct recv_info_t
        {
            std::vector<uint8_t> data;
            std::vector<uint8_t> fec;
        };

        std::vector<recv_info_t> recv_info(256);

        for (auto& i : recv_info)
        {
            i.data.resize(data_size);
            i.fec.resize(fec_size);
        }

        while (true)
        {
            ssize_t rv = recv(wdev, wiffer, sizeof(wiffer), 0);
            if (rv < 0)
            {
                printf("recv failed! errno=%d error=%s\n", errno, strerror(errno));
                return -1;
            }

            winject::radiotap::radiotap_t radiotap(wiffer);
            winject::ieee_802_11::frame_t frame80211(radiotap.end(), wiffer+rv);

            auto bufferss = buffer_str(wiffer, rv);
            LOG("buffer[%d]=\n%s\n", rv, bufferss.c_str());

            LOG("recv size %lu", rv);
            LOG("--- radiotap info ---\n%s", winject::radiotap::to_string(radiotap).c_str());
            LOG("--- 802.11 info ---\n%s", winject::ieee_802_11::to_string(frame80211).c_str());
            LOG("  frame body size: %lu", frame80211.frame_body_size());

            if (!frame80211.frame_body)
            {
                continue;
            }

            SimpleFEC_LLC_t fec_llc(frame80211.frame_body, data_size, fec_size);

            LOG("--- SimpleFEC_LLC info ----");
            LOG("simple_fec:");
            LOG("  datanumber: %d", *(fec_llc.datanumber));
            LOG("  fecnumber: %d", *(fec_llc.fecnumber));

            auto data_idx = *(fec_llc.datanumber);
            auto fec_idx = *(fec_llc.fecnumber);
            auto& ri_data = recv_info[data_idx];
            auto& ri_fec = recv_info[fec_idx];

            std::memcpy(ri_data.data.data(), fec_llc.data(), data_size);
            std::memcpy(ri_fec.fec.data(), fec_llc.fec(), fec_size);

            uint8_t current_data_idx = fec_idx - nblockdelay;
            auto& cri_data = recv_info[current_data_idx];

            std::vector<uint8_t> decoded_data(data_size);

            // @note: FEC Decode
            for (int h=0; h<fec_per_block; h++)
            {
                block_t block;
                for (int i=0; i<223; i++)
                {
                    block[i] = cri_data.data[h*223+i];
                }
                for (int i=0; i<32; i++)
                {
                    block.fec(i) = ri_fec.fec.data()[h*32+i];
                }
                if (!decoder.decode(block))
                {
                    LOG("Error - Critical decoding failure! Msg: %s", block.error_as_string());
                    continue;
                }

                LOG("Decoding data[%d] with fec", current_data_idx);
                for (int i=0; i<223; i++)
                {
                    decoded_data[h*223+i] = block[i];
                }
            }

            uint8_t* current_fragment = decoded_data.data();

            LOG("fragments: ");
            int ifrag = 0;
            while (true)
            {
                if ((current_fragment + packet_fragments_t::header_size()) > (decoded_data.data() + data_size))
                {
                    break;
                }

                packet_fragments_t fragment(current_fragment);
                LOG("  fragment[%d]:", ifrag++);
                LOG("    sn: %d", fragment.get_sn());
                LOG("    of: %d", fragment.get_offset());
                LOG("    sz: %d", fragment.get_size());
                LOG("    ed: %d", fragment.get_is_last());
                
                if (fragment.get_offset()+fragment.get_size() > sizeof(buffer))
                {
                    LOG("  Unable to process fragment further, not enough buffer!");
                    break;
                }

                std::memcpy(buffer+fragment.get_offset(), fragment.data(), fragment.get_size());

                if (fragment.get_is_last())
                {
                    auto total_udp_size = fragment.get_offset()+fragment.get_size();
                    if (total_udp_size>0)
                    {
                        LOG("Sending to UDP: size=%d", total_udp_size);
                        // LOG("\n%s", buffer_str(buffer, total_udp_size).c_str());
                        rv = sendto(sd, buffer, total_udp_size, 0, (sockaddr*)&sendaddr, sizeof(sendaddr));
                        if (rv < 0)
                        {
                            LOG("send failed! errno=%d error=%s", errno, strerror(errno));
                            return -1;
                        }
                    }
                }
                current_fragment += fragment.fragment_size();
            }

            LOG("-----------------------\n");
        }
    }         
    else
    {
        std::stringstream ss;
        ss << "invalid mode: " << options.at("mode");
        throw std::runtime_error(ss.str());
    }

}