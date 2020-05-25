//
//  RopeBuffer.h
//  libraries/networking/src/serialization
//
//  Created by Heather Anderson on 2020-05-02.
//  Copyright 2020 Vircadia.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef serialization_RopeBuffer_h
#define serialization_RopeBuffer_h

#include <QtCore/QSharedPointer>
#include <string>

template <typename char_type,
          int block_size,
          class traits = std::char_traits<char_type>>
class RopeBufferStore : public std::basic_streambuf<char_type, traits> {
public:
    RopeBufferStore();
    size_t length() const;

protected:
    typedef typename traits::int_type int_type;
    int_type overflow(int_type ch /* = _Traits::eof() */) override;

private:
    typedef char_type BufferBlock[block_size];
    typedef QSharedPointer<BufferBlock> BufferBlockPointer;
    typedef QList<BufferBlockPointer> BufferList;

    void newBlock();

    BufferList _buffer;
    BufferBlockPointer _currentBlock;
};

template<typename char_type, int block_size, class traits = std::char_traits<char_type>>
class RopeBuffer : public std::basic_ostream<char_type, traits> {
private:
    typedef RopeBufferStore<char_type, block_size, traits> StoreType;
    StoreType _store;

public:
    RopeBuffer();
};

#include "RopeBuffer.inl"
#endif /* serialization_RopeBuffer_h */