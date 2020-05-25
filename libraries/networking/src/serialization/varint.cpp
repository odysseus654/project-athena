#include "Byteslice.h"
#include "RopeBuffer.h"
#include <memory>
#include <QtCore/QStack>
#include <string>

//typedef std::basic_string<quint8> Bytestring;

enum class ChunkType {
    Object = 1,
    Bitstring = 2,
    Integer = 3,
    Float = 5,
    String = 6
};

enum ChunkFlags {
    CF_ShortChunk = 4,
    CF_Array = 8,
};

struct Chunk {
    quint64 id;
    ChunkType type;
    quint8 flags;
    ByteSlice content;
};

class CorruptException : public std::runtime_error {
public:
    inline CorruptException() : std::runtime_error("bytestream corrupt") {}
};

class SdxfReader {
public:
    typedef std::shared_ptr<Chunk> ChunkPointer;

    inline SdxfReader(ByteSlice&& data);
    inline SdxfReader(const ByteSlice& data);

    bool next(ChunkPointer& chunk);
    void enter();
    void leave();

protected:
    typedef QStack<ChunkPointer> ChunkStack;
    ChunkPointer readChunk(ByteSlice& src);
    static quint64 readVarint(ByteSlice& src);

    ByteSlice _buffer;
    ChunkPointer _previousChunk;
    ChunkPointer _currentChunk;
    ChunkStack _chunkStack;
};

class SdxfWriter {
public:
    typedef RopeBuffer<unsigned char, 512> BufferType;

    SdxfWriter();
    void enter(quint64 id);
    void leave();

protected:
    typedef QStack<BufferType> BufferStack;

    void writeVarint(quint64 val);

    BufferStack _buffers;
    BufferType* _currentBuffer{nullptr};
};

// ---------------------------------------------------------------------------------------------------

SdxfReader::SdxfReader(ByteSlice&& data): _buffer(std::move(data)) {
}

SdxfReader::SdxfReader(const ByteSlice& data):_buffer(data) {
}

SdxfReader::ChunkPointer SdxfReader::readChunk(ByteSlice& src) {
    ChunkPointer newChunk(new Chunk);
    newChunk->id = readVarint(src);
    quint8 flags = src.pop_front();
    newChunk->type = static_cast<ChunkType>(flags & 0x7);
    newChunk->flags = flags >> 3;
    quint64 length = (newChunk->flags & CF_ShortChunk) ? 1ULL : readVarint(src);
    newChunk->content = src.pop_substring(0, length);
    return newChunk;
}

bool SdxfReader::next(ChunkPointer& chunk) {
    ByteSlice& src = _previousChunk ? _previousChunk->content : _buffer;
    if (src.empty()) {
        // called next after we already hit the end
        throw EOFException();
    }
    chunk = _currentChunk = readChunk(src);
    return src.empty();
}

void SdxfReader::enter() {
    _previousChunk = _currentChunk;
    _chunkStack.push(_currentChunk);
    _currentChunk.reset();
}

void SdxfReader::leave() {
    if (_chunkStack.empty()) {
        // called leave when there's nothing to leave
        throw EOFException();
    }
    _currentChunk = _chunkStack.top();
    _chunkStack.pop();
    if (!_chunkStack.empty()) {
        _previousChunk = _chunkStack.top();
    } else {
        _previousChunk.reset();
    }
}

quint64 SdxfReader::readVarint(ByteSlice& src) {
    quint8 c = src.pop_front();
    if(c < 0x80) {
        return static_cast<quint64>(c);
    }
    if(c < 0b11000000) { // unexpected continuation byte
        throw CorruptException();
    }
    size_t size;
    quint64 result;
    if(c < 0b11100000) {
        size = 1;
        result = static_cast<quint64>(c & 0b00011111);
    } else if (c < 0b11110000) {
        size = 2;
        result = static_cast<quint64>(c & 0b00001111);
    } else if (c < 0b11111000) {
        size = 3;
        result = static_cast<quint64>(c & 0b00000111);
    } else if (c < 0b11111100) {
        size = 4;
        result = static_cast<quint64>(c & 0b00000011);
    } else if (c < 0b11111110) {
        size = 5;
        result = static_cast<quint64>(c & 0b00000001);
    } else {
        throw CorruptException();
    }
    for (size_t idx = 0; idx < size; ++idx) {
        c = src.pop_front();
        if ((c & 0b11000000) != 0b10000000) { // this isn't a continuation byte
            throw CorruptException();
        }
        result = result << 6 | static_cast<quint64>(c & 0b00111111);
    }
    return result;
}

// ---------------------------------------------------------------------------------------------------

void SdxfWriter::writeVarint(quint64 val) {
    quint8 first;
    if (val < 0x80) {
        first = static_cast<quint8>(val);
        _currentBuffer->write(&first, 1);
        return;
    }
    size_t size;
    if(val < 0x00000800) {  // 0000 0000 0000 0000 0000 0111 11|11 1111
        size = 1;
        first = static_cast<quint8>((val & 0x000007C0) >> 6 | 0b11000000);
    } else if(val < 0x00010000) {  // 0000 0000 0000 0000 1111 |1111 11|11 1111
        size = 2;
        first = static_cast<quint8>((val & 0x0000F000) >> 12 | 0b11100000);
    } else if(val < 0x00200000) {  // 0000 0000 0001 11|11 1111 |1111 11|11 1111
        size = 3;
        first = static_cast<quint8>((val & 0x001C0000) >> 18 | 0b11110000);
    } else if(val < 0x04000000) {  // 0000 0011 |1111 11|11 1111 |1111 11|11 1111
        size = 4;
        first = static_cast<quint8>((val & 0x03000000) >> 24 | 0b11111000);
    } else if(val < 0x80000000) {  // 01|11 1111 |1111 11|11 1111 |1111 11|11 1111
        size = 5;
        first = static_cast<quint8>((val & 0x40000000) >> 30 | 0b11111100);
    } else {
        size = 6;
        first = 0b11111110;
	}
    Bytestring result;
    result.reserve(size + 1);
    for (size_t idx = 0; idx < size; ++idx) {
        result[size - idx] = static_cast<quint8>((val & 0b111111) | 0b10000000);
        val = val >> 6;
    }
    result[0] = first;
    _currentBuffer->write(&result[0], size + 1);
}
