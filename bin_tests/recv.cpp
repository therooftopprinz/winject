#include <wifi.hpp>
#include <radiotap.hpp>
#include <802_11.hpp>
#include <802_11_filters.hpp>

#include <thread>

int main(int argc, char *argv[])
{
    std::string device_str = argv[1];

    winject::wifi wdev(device_str);


    uint8_t buffer[1024*4];
    memset(buffer, 0, sizeof(buffer));
    int wdev_sd = wdev;

    // winject::ieee_802_11::filters::data_addr3 bssid_filter(0x123456);
    winject::ieee_802_11::filters::data_addr3 bssid_filter(0x563412);

    sock_fprog bpf =
        {
            .len    = bssid_filter.size(),
            .filter = bssid_filter.data(),
        };

    int ret = setsockopt(wdev_sd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf));
    if (ret < 0)
    {
        printf("SO_ATTACH_FILTER failed! %d=%s\n", errno, strerror(errno));
        return 0;
    }

    winject::radiotap::radiotap_t radiotap(buffer);

    struct llc_dummy_t
    {
        uint8_t dsap;
        uint8_t ssap;
        uint16_t ctl;
    } __attribute__((__packed__));

    uint8_t *payload = nullptr;

    auto tref = std::chrono::high_resolution_clock::now();
    while (true)
    {
        ssize_t rv = recv(wdev_sd, buffer, sizeof(buffer), 0);
        if (rv < 0)
        {
            printf("Recv failed! errno=%d error=%s\n", errno, strerror(errno));
            return -1;
        }

        radiotap.rescan();      
        winject::ieee_802_11::frame_t frame80211(radiotap.end());

        auto llc = (llc_dummy_t*)(frame80211.frame_body);
        llc->dsap = 0xFF;
        llc->ssap = 0;
        llc->ctl = 0;

        payload = frame80211.frame_body+sizeof(llc_dummy_t);

        auto now = std::chrono::high_resolution_clock::now();
        auto tse_us = std::chrono::duration_cast<std::chrono::microseconds>(now-tref).count();

        auto radiotap_string = winject::radiotap::to_string(radiotap);
        printf("time: %ld size: %lu\n",  tse_us, rv);
        printf("--- radiotap header info ---\n%s\n", radiotap_string.c_str());
        printf("--- 802.11 info ---\n");
        printf("802.11:\n");
        printf("  frame_control:\n");
        printf("    protocol_type:%p\n", (void*)(uintptr_t) frame80211.frame_control->protocol_type);
        printf("    flags:%p\n", (void*)(uintptr_t) frame80211.frame_control->flags);
        printf("  duration:%p\n", (void*)(uintptr_t) *frame80211.duration);
        printf("  address1:%p\n", (void*) frame80211.address1->get());
        printf("  address2:%p\n", (void*) frame80211.address2->get());
        printf("  address3:%p\n", (void*) frame80211.address3->get());
        printf("  seq_ctl:\n");
        printf("    seq:%d\n", frame80211.seq_ctl->get_seq_num());
        printf("    frag:%d\n", frame80211.seq_ctl->get_fragment_num());
        printf("--------------------------------------------\n");
    }
}