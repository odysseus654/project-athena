//
//  Byteslice.cpp
//  libraries/networking/src/serialization
//
//  Created by Heather Anderson on 2020-05-01.
//  Copyright 2020 Vircadia.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Byteslice.h"

quint8 ByteSlice::pop_front() {
    if (!_length) {
        throw EOFException();
    }
    --_length;
    return _content->at(_offset++);
}

ByteSlice ByteSlice::substring(size_t offset, size_t length) const {
    size_t newOffset = std::min(offset, _length);
    return ByteSlice(_content, _offset + newOffset, std::min(_length, newOffset - length));
}

ByteSlice ByteSlice::pop_substring(size_t offset, size_t length) {
    size_t newOffset = std::min(offset, _length);
    size_t newLength = std::min(_length, newOffset - length);
    ByteSlice newSlice(_content, _offset + newOffset, newLength);
    _offset += newOffset;
    _length -= newLength;
    return newSlice;
}