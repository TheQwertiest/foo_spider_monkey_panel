#pragma once

#include <span>
#include <string>

namespace smp
{

class MD5
{
public:
    MD5();

public:
    std::string Digest( std::span<const uint8_t> data );

private:
    void Init();
    void Update( std::span<const uint8_t> input );
    void Final();
    void WriteToString();

    // The core of the MD5 algorithm is here.
    // MD5 basic transformation. Transforms state based on block.
    static void MD5Transform( uint32_t state[4], const uint8_t block[64] );

    // Encodes input (uint32_t) into output (uint8_t). Assumes len is
    // a multiple of 4.
    static void Encode( uint8_t* output, std::span<const uint32_t> input );

    // Decodes input (uint8_t) into output (uint32_t). Assumes len is
    // a multiple of 4.
    static void Decode( uint32_t* output, std::span<const uint8_t> input );

private:
    struct __context_t
    {
        uint32_t state[4];  /* state (ABCD) */
        uint32_t count[2];  /* number of bits, modulo 2^64 (lsb first) */
        uint8_t buffer[64]; /* input buffer */
    } context;

    std::array<uint8_t, 16> digestRaw;
    std::string digestChars;
};

} // namespace smp
