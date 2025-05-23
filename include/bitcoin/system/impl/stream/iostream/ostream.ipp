/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_SYSTEM_STREAM_IOSTREAM_OSTREAM_IPP
#define LIBBITCOIN_SYSTEM_STREAM_IOSTREAM_OSTREAM_IPP

#include <algorithm>
#include <bitcoin/system/define.hpp>
#include <bitcoin/system/math/math.hpp>

namespace libbitcoin {
namespace system {

// Allowed here for low level performance benefit.
BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)

template <typename Character>
template <typename Buffer>
ostream<Character>::ostream(Buffer& buffer) NOEXCEPT
  : position_(buffer.data()),
    begin_(position_),
    end_(begin_ + buffer.size()),
    state_(goodbit)
{
}

template <typename Character>
ostream<Character>::ostream(uint8_t* begin, ptrdiff_t size) NOEXCEPT
  : position_(begin),
    begin_(position_),
    end_(begin_ + size),
    state_(goodbit)
{
}

template <typename Character>
inline typename ostream<Character>::iostate
ostream<Character>::rdstate() const NOEXCEPT
{
    return state_;
}

template <typename Character>
inline void
ostream<Character>::setstate(iostate state) NOEXCEPT
{
    state_ |= state;
}

template <typename Character>
inline void
ostream<Character>::clear(iostate state) NOEXCEPT
{
    state_ = state;
}

template <typename Character>
inline typename ostream<Character>::pos_type
ostream<Character>::tellp() const NOEXCEPT
{
    return static_cast<pos_type>(position_ - begin_);
}

template <typename Character>
void
ostream<Character>::write(const char_type* data,
    std::streamsize count) NOEXCEPT
{
    const auto bytes = possible_narrow_sign_cast<size_t>(count);

    if (is_overflow(bytes))
    {
        setstate(badbit);
        return;
    }

    BC_PUSH_WARNING(NO_UNSAFE_COPY_N)
    std::copy_n(data, bytes, position_);
    BC_POP_WARNING()

    position_ += bytes;
}

template <typename Character>
void
ostream<Character>::flush() NOEXCEPT
{
}

// private
template <typename Character>
bool
ostream<Character>::is_overflow(pos_type size) const NOEXCEPT
{
    return (state_ != goodbit) || (size > (end_ - position_));
}

BC_POP_WARNING()

} // namespace system
} // namespace libbitcoin

#endif
