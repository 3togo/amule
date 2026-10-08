//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "icons/icon_data.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
	if (argc != 2) {
		return 1;
	}
	int count = 0;
	const auto *entries = amule_get_all_icons(&count);
	unsigned int checked = 0;
	for (int i = 0; i < count; ++i) {
		const auto &entry = entries[i];
		if (!entry.svg_data) {
			unsigned char output;
			if (amule_decode_icon_svg(&entry, &output, 1)) {
				return 1;
			}
			continue;
		}
		std::string name(entry.name);
		if (name.compare(0, 5, "flag_") == 0) {
			name = "flags/" + name.substr(5);
		}
		std::ifstream source(std::string(argv[1]) + "/" + name + ".svg", std::ios::binary);
		if (!source) {
			std::cerr << "Missing SVG source: " << name << '\n';
			return 1;
		}
		const std::vector<unsigned char> expected(
			(std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
		std::vector<unsigned char> decoded(entry.svg_raw_len);
		if (!amule_decode_icon_svg(&entry, decoded.data(), decoded.size()) || decoded != expected) {
			std::cerr << "SVG round trip failed: " << name << '\n';
			return 1;
		}
		if (amule_decode_icon_svg(&entry, decoded.data(), decoded.size() - 1) ||
			amule_decode_icon_svg(&entry, nullptr, decoded.size()) ||
			amule_decode_icon_svg(nullptr, decoded.data(), decoded.size())) {
			return 1;
		}
		// Reject damaged compressed streams and mismatched decoded lengths.
		std::vector<unsigned char> damaged(entry.svg_data, entry.svg_data + entry.svg_len);
		damaged[0] ^= 0xff;
		auto invalid = entry;
		invalid.svg_data = damaged.data();
		if (amule_decode_icon_svg(&invalid, decoded.data(), decoded.size())) {
			return 1;
		}
		invalid = entry;
		invalid.svg_len--;
		if (amule_decode_icon_svg(&invalid, decoded.data(), decoded.size())) {
			return 1;
		}
		invalid = entry;
		invalid.svg_raw_len++;
		decoded.resize(invalid.svg_raw_len);
		if (amule_decode_icon_svg(&invalid, decoded.data(), decoded.size())) {
			return 1;
		}
		++checked;
	}
	std::cout << "Verified " << checked << " compressed SVGs\n";
	return checked ? 0 : 1;
}
