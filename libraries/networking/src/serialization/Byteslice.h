//
//  Byteslice.h
//  libraries/networking/src/serialization
//
//  Created by Heather Anderson on 2020-05-01.
//  Copyright 2020 Vircadia.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef serialization_Byteslice_h
#define serialization_Byteslice_h

#include <QByteArray>
#include <string>

typedef std::basic_string<quint8> Bytestring;

class EOFException : public std::runtime_error {
public:
    inline EOFException() : std::runtime_error("end of stream") {}
};

class ByteSlice {
public:
    inline ByteSlice();
    inline ByteSlice(const QByteArray& data);
    inline ByteSlice(const std::string& data);
    inline ByteSlice(const Bytestring& data);
    inline ByteSlice(const ByteSlice& data);

    inline size_t length() const;
    inline bool empty() const;
    quint8 pop_front();
    ByteSlice substring(size_t offset, size_t length) const;
    ByteSlice pop_substring(size_t offset, size_t length);

protected:
    typedef std::shared_ptr<Bytestring> BytestringPointer;
    inline ByteSlice(BytestringPointer content, size_t offset, size_t length);

    BytestringPointer _content;
    size_t _offset;
    size_t _length;
};

#include "Byteslice.inl"
#endif /* serialization_Byteslice_h */