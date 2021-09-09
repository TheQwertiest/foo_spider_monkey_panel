#pragma once

#include <span>
#include <string>

namespace smp
{

class MD5
{
    static constexpr size_t kBlocksize = 64;

public:
    MD5();
    MD5( std::span<const uint8_t> input );

    void Update( std::span<const uint8_t> input );
    void Finalize();

    [[nodiscard]] std::string HexDigest() const;

private:
    void Init();
    void Transform( const uint8_t block[kBlocksize] );
    static void Decode( uint32_t output[], const uint8_t input[], size_t len );
    static void Encode( uint8_t output[], const uint32_t input[], size_t len );

private:
    bool finalized = false;
    uint8_t buffer[kBlocksize]; // bytes that didn't fit in last 64 byte chunk
    uint32_t count[2];          // 64bit counter for number of bits (lo, hi)
    uint32_t state[4];          // digest so far
    uint8_t digest[16];         // the result
};

[[nodiscard]] std::string CalculateMd5( std::span<const uint8_t> input );

} // namespace smp
