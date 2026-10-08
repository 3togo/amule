// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>
namespace qbit_compat
{
using Fields = std::map<std::string, std::string>;
inline bool Decode(const std::string &input, std::string &out)
{
	out.clear();
	auto hex = [](unsigned char c) -> int {
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		return -1;
	};
	for (size_t i = 0; i < input.size(); ++i) {
		unsigned char c = input[i];
		if (c == '%') {
			if (i + 2 >= input.size() || hex(input[i + 1]) < 0 || hex(input[i + 2]) < 0)
				return false;
			c = static_cast<unsigned char>(hex(input[i + 1]) * 16 + hex(input[i + 2]));
			i += 2;
		} else if (c == '+')
			c = ' ';
		if (!c)
			return false;
		out += static_cast<char>(c);
	}
	return true;
}
inline bool Form(const std::string &body, Fields &fields)
{
	fields.clear();
	size_t start = 0;
	while (start < body.size()) {
		const auto end = body.find('&', start);
		const auto item = body.substr(start, end == std::string::npos ? end : end - start);
		const auto equal = item.find('=');
		std::string key, value;
		if (!Decode(item.substr(0, equal), key) ||
			!Decode(equal == std::string::npos ? "" : item.substr(equal + 1), value) ||
			key.empty() || !fields.emplace(key, value).second || fields.size() > 32)
			return false;
		if (end == std::string::npos)
			break;
		start = end + 1;
	}
	return true;
}
// Field-only multipart requests are accepted; torrent-file uploads are deliberately rejected.
inline bool Multipart(const std::string &body, const std::string &boundary, Fields &fields)
{
	fields.clear();
	if (boundary.empty() || boundary.size() > 70 || boundary.find_first_of("\r\n") != std::string::npos)
		return false;
	const std::string marker = "--" + boundary;
	size_t cursor = 0;
	while (body.compare(cursor, marker.size(), marker) == 0) {
		cursor += marker.size();
		if (body.compare(cursor, 2, "--") == 0) {
			cursor += 2;
			return cursor == body.size() || body.substr(cursor) == "\r\n";
		}
		if (body.compare(cursor, 2, "\r\n") != 0)
			return false;
		cursor += 2;
		const auto headerEnd = body.find("\r\n\r\n", cursor);
		if (headerEnd == std::string::npos || headerEnd - cursor > 4096)
			return false;
		const auto headers = body.substr(cursor, headerEnd - cursor);
		const auto name = headers.find("name=\"");
		if (name == std::string::npos || headers.find("filename=") != std::string::npos)
			return false;
		const auto close = headers.find('"', name + 6);
		if (close == std::string::npos)
			return false;
		const auto key = headers.substr(name + 6, close - name - 6);
		cursor = headerEnd + 4;
		const auto next = body.find("\r\n" + marker, cursor);
		if (next == std::string::npos || key.empty() ||
			!fields.emplace(key, body.substr(cursor, next - cursor)).second || fields.size() > 32)
			return false;
		if (fields[key].find('\0') != std::string::npos)
			return false;
		cursor = next + 2;
	}
	return false;
}
inline std::string Trim(const std::string &value)
{
	const auto start = value.find_first_not_of(" \t");
	if (start == std::string::npos)
		return {};
	return value.substr(start, value.find_last_not_of(" \t") - start + 1);
}
inline std::string Lower(std::string value)
{
	for (char &c : value)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return value;
}
inline bool ParseForm(const std::string &body, const std::string &contentType, Fields &fields)
{
	const auto first = contentType.find(';');
	const auto type = Lower(Trim(contentType.substr(0, first)));
	if (type.empty() || type == "application/x-www-form-urlencoded")
		return Form(body, fields);
	if (type != "multipart/form-data" || first == std::string::npos)
		return false;
	std::string boundary;
	bool found = false;
	size_t cursor = first + 1;
	while (cursor < contentType.size()) {
		size_t end = cursor;
		bool quoted = false;
		for (; end < contentType.size(); ++end) {
			if (contentType[end] == '"')
				quoted = !quoted;
			if (!quoted && contentType[end] == ';')
				break;
		}
		if (quoted)
			return false;
		const auto parameter = contentType.substr(cursor, end - cursor);
		const auto equal = parameter.find('=');
		if (equal != std::string::npos && Lower(Trim(parameter.substr(0, equal))) == "boundary") {
			if (found)
				return false;
			found = true;
			boundary = Trim(parameter.substr(equal + 1));
			if (boundary.size() >= 2 && boundary.front() == '"' && boundary.back() == '"')
				boundary = boundary.substr(1, boundary.size() - 2);
		}
		cursor = end + 1;
	}
	return found && Multipart(body, boundary, fields);
}
inline bool Hashes(const std::string &value, std::vector<std::string> &hashes)
{
	hashes.clear();
	if (value.empty())
		return false;
	size_t start = 0;
	do {
		const auto end = value.find('|', start);
		auto hash = value.substr(start, end == std::string::npos ? end : end - start);
		if (hash.size() != 32)
			return false;
		for (char &c : hash) {
			if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
				return false;
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		if (std::find(hashes.begin(), hashes.end(), hash) == hashes.end())
			hashes.push_back(hash);
		if (hashes.size() > 100)
			return false;
		if (end == std::string::npos)
			return true;
		start = end + 1;
	} while (start <= value.size());
	return false;
}
inline const char *State(const std::string &native, unsigned speed)
{
	if (native == "completed")
		return "stoppedUP";
	if (native == "paused" || native == "stopped")
		return "stoppedDL";
	if (native == "hashing" || native == "allocating" || native == "completing")
		return "checkingDL";
	if (native == "waiting")
		return "queuedDL";
	if (native == "erroneous" || native == "insufficient_disk")
		return "error";
	if (native == "downloading")
		return speed ? "downloading" : "stalledDL";
	return "unknown";
}
inline std::string Join(const std::string &directory, const std::string &leaf)
{
	if (directory.empty())
		return leaf;
	if (directory.back() == '/' || directory.back() == '\\')
		return directory + leaf;
	return directory + (directory.find('\\') != std::string::npos ? "\\" : "/") + leaf;
}
} // namespace qbit_compat
