// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "OpenGlassBox/Script/IScriptParser.hpp"

#include <sstream>

namespace ogb {

std::string ParseError::format() const
{
    std::ostringstream stream;
    stream << file << ':' << line << ':' << column << ": " << message;

    if (!source.empty())
    {
        stream << '\n' << source << '\n';

        for (uint32_t i = 1u; (i < column) && (i <= source.size()); ++i)
        {
            stream << ((source[i - 1u] == '\t') ? '\t' : ' ');
        }
        stream << '^';
    }

    return stream.str();
}

std::string IScriptParser::formatErrors() const
{
    std::ostringstream stream;
    bool first = true;

    for (auto const& e: errors())
    {
        if (!first)
        {
            stream << '\n';
        }
        stream << e.format();
        first = false;
    }

    return stream.str();
}

} // namespace ogb
