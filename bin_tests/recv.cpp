#include <winject/wifi.hpp>
#include <winject/radiotap.hpp>
#include <winject/802_11.hpp>
#include <winject/802_11_filters.hpp>

#include <thread>

int main(int argc, char *argv[])
{
    std::string device_str = argv[1];

    winject::wifi wdev(device_str);

    uint8_t buffer[1024*4];
    memset(buffer, 0, sizeof(buffer));

    winject::ieee_802_11::filters::data_addr3 bssid_filter(0x563412);
    winject::ieee_802_11::filters::attach(wdev, bssid_filter);

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
        ssize_t rv = recv(wdev, buffer, sizeof(buffer), 0);
        if (rv < 0)
        {
            printf("Recv failed! errno=%d error=%s\n", errno, strerror(errno));
            return -1;
        }

        winject::radiotap::radiotap_t radiotap(buffer);
        winject::ieee_802_11::frame_t frame80211(radiotap.end());
        auto llc = (llc_dummy_t*)(frame80211.frame_body);
        payload = frame80211.frame_body+sizeof(llc_dummy_t);

        auto now = std::chrono::high_resolution_clock::now();
        auto tse_us = std::chrono::duration_cast<std::chrono::microseconds>(now-tref).count();

        printf("time: %ld size: %lu\n",  tse_us, rv);
        printf("--- radiotap header info ---\n%s\n", winject::radiotap::to_string(radiotap).c_str());
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
        printf("--- LLC info ---\n");
        printf("LLC:\n");
        printf("  dsap: %d\n", llc->dsap);
        printf("  ssap: %d\n", llc->ssap);
        printf("  ctl: %d\n", llc->ctl);
        printf("--------------------------------------------\n");
    }
}