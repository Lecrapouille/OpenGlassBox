# Licensing

OpenGlassBox is dual-licensed.

## Open source: GNU GPL v3

OpenGlassBox is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version. The full text is in the [`LICENSE`](LICENSE) file.

OpenGlassBox is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE. See the GNU General Public License for more details.

In short, if you distribute a product that includes or links against OpenGlassBox
(statically or dynamically), the GPLv3 requires you to release the complete
corresponding source code of that product under the GPLv3 as well, and to
let its users install modified versions on their devices.

## Commercial license

For our users who cannot use OpenGlassBox under the GPLv3, for instance because
they wish to embed it in a proprietary product or distribute it without
publishing their own source code, a commercial license to OpenGlassBox is
available.

Please contact the copyright holder directly at:

- **Email:** lecrapouille@gmail.com
- **Web:** https://github.com/Lecrapouille/OpenGlassBox

A commercial license typically covers:

- linking and redistribution of OpenGlassBox in closed-source products,
  including embedded devices;
- exemption from the GPLv3 source-disclosure and installation-information
  requirements for the licensed products;
- optionally, technical support, maintenance and updates.

Terms (scope, fees, duration, support) are agreed in a separate written
contract.

## Which license applies to me?

| Your situation | License |
| --- | --- |
| Open-source project released under a GPLv3-compatible license | GPLv3 (free) |
| Research, education, personal use, internal tools never distributed | GPLv3 (free) |
| Proprietary or closed-source product that includes OpenGlassBox | Commercial |
| Product you ship to customers without publishing its sources | Commercial |

If in doubt, contact us before shipping.

## Third-party code

OpenGlassBox depends on third-party components that keep their own licenses. Check
their terms before redistributing. The commercial license only covers code
owned by the OpenGlassBox copyright holder.

## Contributing

Because OpenGlassBox is offered under two licenses, contributions can only be
accepted if the contributor agrees to license them under both.

## Source file headers

Each source file should carry the following header:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.
```
