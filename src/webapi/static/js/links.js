// Build the ed2k-compatible magnet URI exactly like the desktop GUI's
// CamuleAppCommon::CreateMagnetLink (src/amuleAppCommon.cpp): field order
// dn, xt:urn:ed2k, xt:urn:ed2khash, xl, optional xt:urn:aich; ED2K hash
// lower-cased; the name only has spaces -> %20 and '/' stripped
// (CPath::Cleanup(false)), not full URL-encode.
export function magnetLink(d) {
  let dn = "";
  for (const ch of d.name || "") {
    if (ch === "/") continue;
    if (ch === " ") dn += "%20";
    else if (ch.codePointAt(0) >= 32) dn += ch;
  }
  const h = (d.hash || "").toLowerCase();
  return "magnet:?dn=" + dn +
    "&xt=urn:ed2k:" + h + "&xt=urn:ed2khash:" + h +
    "&xl=" + (d.size_bytes || 0) +
    (d.aich_hash ? "&xt=urn:aich:" + d.aich_hash : "");
}
