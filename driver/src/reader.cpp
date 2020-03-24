#include "reader.hpp"

struct GPIOData
{
    uint8_t byte;
    unsigned int tco;
    bool piOK;
};

GPIOData readByteAndStatusFromGPIO()
{
    setGPIOHigh(CLOCK);
    sleep55ns();
    sleep55ns();

    setGPIOLow(CLOCK);
    sleep55ns();
    sleep55ns();

    unsigned int allGPIO = readAllGPIO();
    uint8_t byte =
        ((allGPIO & DATA_POSITION[0]) >> 17) |  // Bit 7
        ((allGPIO & DATA_POSITION[1]) >> 19) |  //     6
        ((allGPIO & DATA_POSITION[2]) >> 2) |  //     5
        ((allGPIO & DATA_POSITION[3]) >> 1) |  //     4
        ((allGPIO & DATA_POSITION[4]) >> 3) |  //     3
        ((allGPIO & DATA_POSITION[5]) >> 10) | //     2
        ((allGPIO & DATA_POSITION[6]) >> 12) | //     1
        ((allGPIO & DATA_POSITION[7]) >> 16);  //     0

    sleep55ns();
    sleep55ns();

    return {
        byte,
        (allGPIO & TCO_POSITION),
        (allGPIO & PI_STATUS_POSITION) != 0};
}

constexpr bool isRisingFlank(bool last, bool now)
{
    return !last && now;
}

// chunk-Layout:
// ------+----------------------------+---------------------------
//  Byte | Bit7   Bit6   Bit5   Bit4  | Bit3   Bit2   Bit1   Bit0
// ------+----------------------------+---------------------------
//     0 | 1-14   2-14   3-14   4-14  | 1-15   2-15   3-15   4-15
//     1 | 1-12   2-12   3-12   4-12  | 1-13   2-13   3-13   4-13
//     2 | 1-10   2-10   3-10   4-10  | 1-11   2-11   3-11   4-11
//     3 |  1-8    2-8    3-8    4-8  |  1-9    2-9    3-9    4-9
//     4 |  1-6    2-6    3-6    4-6  |  1-7    2-7    3-7    4-7
//     5 |  1-4    2-4    3-4    4-4  |  1-5    2-5    3-5    4-5
//     6 |  1-2    2-2    3-2    4-2  |  1-3    2-3    3-3    4-3
//     7 |  1-0    2-0    3-0    4-0  |  1-1    2-1    3-1    4-1

const size_t BLOCKS_PER_CHUNK = 8;
const size_t CHUNK_SIZE_IN_BYTE = BLOCKS_PER_CHUNK;
const size_t TCO_SIZE = 256;

constexpr void setBit(uint16_t &word, uint8_t N, bool bit)
{
    word = (word & ~(1UL << N)) | (bit << N);
}

constexpr bool getBit(uint8_t byte, uint8_t N)
{
    return (byte & (1UL << N));
}

template <class T>
void convertChunkToRecord(const std::array<uint8_t, CHUNK_SIZE_IN_BYTE> &chunk, const std::array<int, 4> &offset, const std::array<float, 4> &mfactor, T& data)
{
    size_t count = 0;
    std::vector<uint16_t> sensors(4, 0);
    static std::vector<uint16_t> sensorOld(4, 32768);

    for (size_t i = 0; i < CHUNK_SIZE_IN_BYTE; ++i)
    {
        setBit(sensors[0], 15 - count, getBit(chunk[i], 3));
        setBit(sensors[1], 15 - count, getBit(chunk[i], 2));
        setBit(sensors[2], 15 - count, getBit(chunk[i], 1));
        setBit(sensors[3], 15 - count, getBit(chunk[i], 0));
        count++;

        setBit(sensors[0], 15 - count, getBit(chunk[i], 7));
        setBit(sensors[1], 15 - count, getBit(chunk[i], 6));
        setBit(sensors[2], 15 - count, getBit(chunk[i], 5));
        setBit(sensors[3], 15 - count, getBit(chunk[i], 4));
        count++;
    }

    for (size_t i = 0; i < 4; ++i)
    {
        //##########################//
        //TBD: Dirty fix for clippings
        //##########################//
        if (sensors[i] % 64 == 7 || sensors[i] % 64 == 56)
        {
            sensors[i] = sensorOld[i];
        }
        sensorOld[i] = sensors[i];
        //##########################//

        data[i].push_back((float)(sensors[i] - offset[i]) * mfactor[i]);
    }
}

struct RecordReader
{
    std::array<uint8_t, CHUNK_SIZE_IN_BYTE> currentChunk;
    size_t bytesRead{0};
    bool isFirst{true};
    size_t lastRead = 0;

    int sensorType;
    std::array<int, 4> offset;
    std::array<float, 4> gain;
    std::array<float, 4> transmission;
    std::array<float, 4> mfactor;

    // read records from hardware buffer
    SensorsData read()
    {
        SensorsData out;
        out.reserve(lastRead*2);
        int lastTCO;
        int currentTCO;

        int count = 0;
        bool run = true;

        // I.
        waitForPiOk();

        // II.
        bool dry_run = true;
        while (run)
        {
            auto res = readByteAndStatusFromGPIO();
            currentTCO = res.tco;

            count++;
            currentChunk[bytesRead] = res.byte;
            bytesRead++;

            if (bytesRead == CHUNK_SIZE_IN_BYTE)
            {
                convertChunkToRecord(currentChunk, offset, mfactor, out.data());
                bytesRead = 0;
            }

            run = dry_run || !(currentTCO - lastTCO == 16384);
            // run = !isRisingFlank(lastTCO,currentTCO);
            lastTCO = currentTCO;
            dry_run = false;
        }

        // discard first read - thats any old data in RAM!
        if (isFirst)
        {
            isFirst = false;
            out.clear();
        }

        // III.
        sleep55ns();
        sleep55ns();

        lastRead = out.DataSize();
        return out;
    }

    void waitForPiOk()
    {
        // for 12MHz Quartz
        std::this_thread::sleep_for(std::chrono::microseconds(700));
    }

    void setup()
    {
        setup_io();
    }

    void start()
    {
        for (size_t i = 0; i < mfactor.size(); i++)
        {
            mfactor[i] = gain[i] * transmission[i];
        }
        init(sensorType);
    }

    void stop()
    {
        shutdown();
    }
};
