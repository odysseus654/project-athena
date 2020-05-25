//
//  RopeBuffer.inl
//  libraries/networking/src/serialization
//
//  Created by Heather Anderson on 2020-05-24.
//  Copyright 2020 Vircadia.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef serialization_RopeBuffer_inl
#define serialization_RopeBuffer_inl

#include "RopeBuffer.h"

// ------------------------------------------------------------------------------------------------ RopeBufferStore

template <typename char_type, int block_size, typename traits>
RopeBufferStore<char_type, block_size, traits>::RopeBufferStore() {
    newBlock();
}

template <typename char_type, int block_size, typename traits>
void RopeBufferStore<char_type, block_size, traits>::newBlock() {
    Q_ASSERT(_buffer.empty() || 0 == _Pnavail());
    _currentBlock = BufferBlockPointer::create();
    _buffer.append(_currentBlock);
    char_type* beginning = &(*_currentBlock)[0];
    setp(beginning, beginning + block_size);
}

template <typename char_type, int block_size, typename traits>
auto RopeBufferStore<char_type, block_size, traits>::overflow(int_type ch /* = _Traits::eof() */) -> int_type {
    Q_ASSERT(0 == _Pnavail());
    newBlock();
    if (traits::eq_int_type(traits::eof(), ch)) {
        return 0;  // succeeded with no char put requested
    } else {
        return xsputn(&static_cast<char_type>(ch), 1);
    }
}

template <typename char_type, int block_size, typename traits>
size_t RopeBufferStore<char_type, block_size, traits>::length() const {
    return static_cast<size_type>(_buffer.count() - 1) * size_type + (pptr() - pbase());
}

// ------------------------------------------------------------------------------------------------ RopeBuffer

template <typename char_type, int block_size, class traits>
RopeBuffer<char_type, block_size, traits>::RopeBuffer() : std::basic_ostream(&_store){
}

#endif /* serialization_RopeBuffer_inl */