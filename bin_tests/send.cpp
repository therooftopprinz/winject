#include <winject/wifi.hpp>
#include <winject/radiotap.hpp>
#include <winject/802_11.hpp>

#include <thread>

void test_radio_tap()
{
    alignas(8) uint8_t  radiotap_header_pre[] = {
        0x00, 0x00, // <-- radiotap version
        0x0d, 0x00, // <- radiotap header length
        0x04, 0x80, 0x00, 0x00, // <-- bitmap
        0x18,       // data rate (will be overwritten)
        0x00,       // align
        0x00, 0x00, // tx flags
        0x00
    };

    winject::radiotap::radiotap_t radiotap(radiotap_header_pre);
    return; 
}

int main(int argc, char *argv[])
{
    test_radio_tap();
    std::string device_str = argv[1];
    std::string sleep_val_str = argv[2];
    
    winject::wifi wdev(device_str);

    auto sleep_val = std::stoull(sleep_val_str);

    uint8_t buffer[1024*4];
    memset(buffer, 0, sizeof(buffer));
    int wdev_sd = wdev;
    int seq_num = 0;

    winject::radiotap::radiotap_t radiotap(buffer);
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
    printf("--- radiotap header info ---\n%s\n\n", radiotap_string.c_str());

    struct llc_dummy_t
    {
        uint8_t dsap;
        uint8_t ssap;
        uint16_t ctl;
    } __attribute__((__packed__));

    uint8_t *payload;
    size_t payload_size = 1450;

    winject::ieee_802_11::frame_t frame80211(radiotap.end());
    frame80211.frame_control->protocol_type = winject::ieee_802_11::frame_control_t::E_TYPE_DATA;
    frame80211.rescan();
    frame80211.address1->set(0xFFFFFFFFFFFF); // IBSS Destination
    frame80211.address2->set(0xAABBCCDDEEFF); // IBSS Source
    frame80211.address3->set(0xDEADBEEFCAFE); // IBSS BSSID
    frame80211.set_body_size(payload_size+sizeof(llc_dummy_t));
    *frame80211.fcs = 0XFFFFFFFF;

    auto llc = (llc_dummy_t*)(frame80211.frame_body);
    llc->dsap = 0xFF; // NULL LSAP
    llc->ssap = 0xFF; // NULL LSAP
    llc->ctl =  0b00000011;

    payload = frame80211.frame_body+sizeof(llc_dummy_t);
    for (int i=0; i<payload_size; i++)
    {
        payload[i] = i & 0xFF;   
    }

    auto& sn = *(uint16_t*)payload;

    size_t size = radiotap.size() + frame80211.size();

    std::printf("buffer[%lu]:", size);

    for (int i=0; i<size; i++)
    {
        if (i%16 == 0)
        {
            printf("\n%04X  ", i);
        }
        printf("%02X ", buffer[i]);
    }

    printf("\n\n");

    sn=0;
    auto tref = std::chrono::high_resolution_clock::now();
    while (true)
    {
        frame80211.seq_ctl->set_seq_num(sn++);
        auto  rv = sendto(wdev_sd, buffer, size, 0, (sockaddr *) &wdev.address(), sizeof(struct sockaddr_ll));
        if (rv < 0)
        {
            printf("Send failed! errno=%d error=%s\n", errno, strerror(errno));
            return -1;
        }
        
        auto now = std::chrono::high_resolution_clock::now();
        auto tse_us = std::chrono::duration_cast<std::chrono::microseconds>(now-tref).count();
        printf("%ld %d\n",  tse_us, seq_num++);

        std::this_thread::sleep_for(std::chrono::microseconds(sleep_val));
    }
}