#include <gtest/gtest.h>
#include "LLC.hpp"

struct TestLLC : public testing::Test
{
    // LLC sut;
};

// should not allocate llc when pdu.size is zero
// should allocate ACKs first
// should allocate PDCP
// should allocate retx PDCP
