// CamuleArtProvider -- see CamuleArtProvider.h for the contract.

#include "CamuleArtProvider.h"

#include "icons/icon_data.h"

#include <wx/bitmap.h>
#include <wx/bmpbndl.h> // Needed for wxBitmapBundle::FromSVG / FromBitmaps
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/settings.h>

const wxString CamuleArtProvider::PREFIX = "amule:";

namespace
{
const struct AMuleIconEntry *FindIcon(const wxArtID &id)
{
	// Only resolve our own art ids; let other providers handle the rest.
	if (!id.StartsWith(CamuleArtProvider::PREFIX)) {
		return nullptr;
	}
	return amule_find_icon(id.Mid(CamuleArtProvider::PREFIX.length()).utf8_str().data());
}

bool LoadPng(const struct AMuleIconEntry *entry, wxImage &image, const wxColour *colour = nullptr)
{
	wxMemoryInputStream stream(entry->png_data, entry->png_len);
	if (!image.LoadFile(stream, wxBITMAP_TYPE_PNG)) {
		return false;
	}
	if (colour) {
		// Monochrome PNG fallbacks retain their alpha, including antialiased edges.
		image.SetRGB(wxRect(image.GetSize()), colour->Red(), colour->Green(), colour->Blue());
	}
	return true;
}

// The icon's SVG twin as a bundle, or an invalid bundle when there is none or it does not parse.
// wxDefaultSize takes the PNG twin's natural size, which FromSVG needs as its default.
wxBitmapBundle SvgBundle(
	const struct AMuleIconEntry *entry, const wxSize &size, const wxColour *colour = nullptr)
{
#ifdef wxHAS_SVG
	if (entry->svg_data != nullptr && entry->svg_len > 0) {
		wxSize sizeDef(size);
		wxImage probe;
		if (sizeDef == wxDefaultSize && LoadPng(entry, probe)) {
			sizeDef = probe.GetSize();
		}
		if (sizeDef != wxDefaultSize) {
			if (colour) {
				wxString svg = wxString::FromUTF8(
					reinterpret_cast<const char *>(entry->svg_data), entry->svg_len);
				svg.Replace("#212529", colour->GetAsString(wxC2S_HTML_SYNTAX));
				return wxBitmapBundle::FromSVG(svg.utf8_str(), sizeDef);
			}
			return wxBitmapBundle::FromSVG(entry->svg_data, entry->svg_len, sizeDef);
		}
	}
#else
	wxUnusedVar(entry);
	wxUnusedVar(size);
	wxUnusedVar(colour);
#endif
	return wxBitmapBundle();
}
wxBitmapBundle IconBundle(const struct AMuleIconEntry *entry,
	const wxSize &size,
	const wxColour *colour = nullptr,
	bool preferSvg = true)
{
	// A malformed SVG, or one using features NanoSVG cannot render, yields a non-ok bundle;
	// fall through to the PNG raster path below rather than returning nothing.
	wxBitmapBundle svg = preferSvg ? SvgBundle(entry, size, colour) : wxBitmapBundle();
	if (svg.IsOk()) {
		return svg;
	}

	// No SVG twin (or wx built without NanoSVG support): serve the PNG at the requested logical
	// size plus a smooth 2x upscale, so DPI-aware widgets still get something better than
	// stretched 1x art. The mask is turned into an alpha channel first, because high-quality
	// scaling of a masked image smears the mask colour into the icon edges.
	wxImage image;
	if (!LoadPng(entry, image, colour)) {
		return wxBitmapBundle();
	}
	if (!image.HasAlpha()) {
		image.InitAlpha();
	}
	// Target logical size: the caller's request, or the PNG's natural size when it does not
	// care. Derive both the 1x and 2x renditions from the original image so each is a single
	// high-quality resample -- building the 2x from an already rescaled 1x would resample
	// twice.
	const wxSize target = (size == wxDefaultSize) ? wxSize(image.GetWidth(), image.GetHeight()) : size;
	wxImage image1x = (target.GetWidth() == image.GetWidth() && target.GetHeight() == image.GetHeight())
				  ? image
				  : image.Scale(target.GetWidth(), target.GetHeight(), wxIMAGE_QUALITY_HIGH);
	wxImage image2x = image.Scale(target.GetWidth() * 2, target.GetHeight() * 2, wxIMAGE_QUALITY_HIGH);
	return wxBitmapBundle::FromBitmaps(wxBitmap(image1x), wxBitmap(image2x));
}
} // namespace

wxBitmap CamuleArtProvider::CreateBitmap(
	const wxArtID &id, const wxArtClient &WXUNUSED(client), const wxSize &size)
{
	const struct AMuleIconEntry *entry = FindIcon(id);
	if (entry == nullptr) {
		return wxNullBitmap;
	}

	// Rendered from the SVG twin at the requested pixel size when there is one, so raster
	// consumers (image lists, DC drawing) get the same artwork as bundle consumers.
	const wxBitmapBundle svg = SvgBundle(entry, size);
	if (svg.IsOk()) {
		return svg.GetBitmap(size == wxDefaultSize ? svg.GetDefaultSize() : size);
	}

	// Decode the embedded PNG bytes. wxImage::LoadFile via a wxMemoryInputStream wins over
	// wxBitmap::NewFromPNGData here because we may need to rescale below, and Scale() is on
	// wxImage, not wxBitmap.
	wxImage image;
	if (!LoadPng(entry, image)) {
		return wxNullBitmap;
	}

	// Honour an explicit size request from the caller. GetBitmap() passes wxDefaultSize when
	// the caller does not care; then the natural size from the PNG header wins.
	if (size != wxDefaultSize &&
		(size.GetWidth() != image.GetWidth() || size.GetHeight() != image.GetHeight())) {
		image = image.Scale(size.GetWidth(), size.GetHeight(), wxIMAGE_QUALITY_HIGH);
	}

	return wxBitmap(image);
}

wxBitmapBundle CamuleArtProvider::CreateBitmapBundle(
	const wxArtID &id, const wxArtClient &WXUNUSED(client), const wxSize &size)
{
	const struct AMuleIconEntry *entry = FindIcon(id);
	if (entry == nullptr) {
		return wxBitmapBundle();
	}

	return IconBundle(entry, size);
}

wxBitmapBundle CamuleArtProvider::GetMenuBitmapBundle(
	const wxString &name, const wxSize &size, const wxColour &colour, bool preferSvg)
{
	if (!name.StartsWith("menu_")) {
		return wxBitmapBundle();
	}
	const auto *entry = amule_find_icon(name.utf8_str());
	// Colour-dependent artwork must not enter wxArtProvider's global cache.
	// A new popup always uses the current theme, even on wx versions whose
	// Push/Pop cache clear leaves bitmap bundles cached.
	return entry ? IconBundle(entry, size, &colour, preferSvg) : wxBitmapBundle();
}
