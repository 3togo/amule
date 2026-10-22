//								-*- C++ -*-
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

#ifndef AMULE_TRAYICONBUNDLE_H
#define AMULE_TRAYICONBUNDLE_H

#include <algorithm>
#include <wx/bmpbndl.h>
#include <wx/colour.h>
#include <wx/image.h>
#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif

namespace MuleTray
{
// Resample the original PNG directly at the requested size. Smooth filtering
// can bleed neighbouring artwork into the bar area, so reserve it afterwards.
class ArtworkFallbackImpl : public wxBitmapBundleImpl
{
public:
	ArtworkFallbackImpl(const wxImage &image, const wxSize &size)
	: m_image(image.Copy())
	, m_size(size)
	{
	}
	wxSize GetDefaultSize() const override { return m_size; }
	wxSize GetPreferredBitmapSizeAtScale(double scale) const override
	{
		return wxSize(wxRound(m_size.x * scale), wxRound(m_size.y * scale));
	}
	wxBitmap GetBitmap(const wxSize &size) override
	{
		wxImage image = size == m_image.GetSize()
					? m_image.Copy()
					: m_image.Scale(size.x, size.y, wxIMAGE_QUALITY_HIGH);
		if (!image.HasAlpha()) {
			image.InitAlpha();
		}
		const int barWidth = std::max(1, size.x / 11);
		for (int y = 0; y < size.y; ++y) {
			for (int x = size.x - barWidth; x < size.x; ++x) {
				image.SetAlpha(x, y, wxALPHA_TRANSPARENT);
			}
		}
		return wxBitmap(image);
	}

private:
	wxImage m_image;
	wxSize m_size;
};

inline wxBitmapBundle MakeArtworkFallback(const wxImage &image, const wxSize &size = wxDefaultSize)
{
	if (!image.IsOk()) {
		return wxBitmapBundle();
	}
	return wxBitmapBundle::FromImpl(
		new ArtworkFallbackImpl(image, size == wxDefaultSize ? image.GetSize() : size));
}

// Compose at the size requested by SetIcon(). Native taskbar backends flatten the
// bundle, so callers must set it again after a DPI change.
class IconBundleImpl : public wxBitmapBundleImpl
{
public:
	IconBundleImpl(const wxBitmapBundle &artwork, unsigned int percent, const wxColour &colour)
	: m_artwork(artwork)
	, m_percent(std::min(percent, 100u))
	, m_colour(colour)
	{
	}

	wxSize GetDefaultSize() const override { return m_artwork.GetDefaultSize(); }

	wxSize GetPreferredBitmapSizeAtScale(double scale) const override
	{
#ifdef __WXMSW__
		// The notification area uses the system small-icon metric, not our 22px canvas.
		// Size it by the taskbar's DPI: wxTaskBarIcon passes the scale of its hidden
		// window, which never gets WM_DPICHANGED and keeps the DPI the process started
		// with. Resolve dynamically to keep compatibility with older Windows versions.
		const HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
		using GetMetricsForDpi = int(WINAPI *)(int, UINT);
		using GetDpiForWindowFn = UINT(WINAPI *)(HWND);
		const auto getMetrics = reinterpret_cast<GetMetricsForDpi>(
			::GetProcAddress(user32, "GetSystemMetricsForDpi"));
		const auto getDpiForWindow =
			reinterpret_cast<GetDpiForWindowFn>(::GetProcAddress(user32, "GetDpiForWindow"));
		if (getMetrics) {
			const HWND taskbar = ::FindWindowW(L"Shell_TrayWnd", nullptr);
			UINT dpi = taskbar && getDpiForWindow ? getDpiForWindow(taskbar) : 0;
			if (dpi == 0) {
				dpi = static_cast<UINT>(wxRound(96 * scale));
			}
			return wxSize(getMetrics(SM_CXSMICON, dpi), getMetrics(SM_CYSMICON, dpi));
		}
		return wxSize(::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
#else
		return m_artwork.GetPreferredBitmapSizeAtScale(scale);
#endif
	}

	wxBitmap GetBitmap(const wxSize &size) override
	{
		const wxBitmap base = m_artwork.GetBitmap(size);
		if (!base.IsOk()) {
			return wxNullBitmap;
		}
		// ConvertToImage() returns an unshared image, leaving the artwork untouched.
		wxImage image = base.ConvertToImage();
		if (!image.HasAlpha()) {
			image.InitAlpha();
		}
		const int width = image.GetWidth();
		const int height = image.GetHeight();
		// Integer arithmetic avoids dropping a row at exact boundaries (e.g.
		// 29% of 100 pixels); widen first so large images cannot overflow.
		const int barHeight = static_cast<int>((static_cast<long long>(height) * m_percent) / 100);
		// The artwork reserves the final two columns of its 22-pixel canvas.
		const int barWidth = std::max(1, width / 11);
		for (int y = height - barHeight; y < height; ++y) {
			for (int x = width - barWidth; x < width; ++x) {
				// wxMSW DC drawing does not set alpha on transparent pixels.
				image.SetRGB(x, y, m_colour.Red(), m_colour.Green(), m_colour.Blue());
				image.SetAlpha(x, y, wxALPHA_OPAQUE);
			}
		}
		return wxBitmap(image);
	}

private:
	wxBitmapBundle m_artwork;
	unsigned int m_percent;
	wxColour m_colour;
};

inline wxBitmapBundle MakeIconBundle(
	const wxBitmapBundle &artwork, unsigned int percent, const wxColour &colour)
{
	if (!artwork.IsOk()) {
		return wxBitmapBundle();
	}
	return wxBitmapBundle::FromImpl(new IconBundleImpl(artwork, percent, colour));
}
} // namespace MuleTray

#endif // AMULE_TRAYICONBUNDLE_H
