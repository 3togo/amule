// Shared client software label. familyName is supplied by the caller
// so this formatter has no translation, UI, or browser dependencies.
export const clientSoftwareLabel = (c, familyName) => {
  const mod = c.client_mod_name;
  const name = mod && c.software && c.software !== "emule" && c.software !== "old_emule"
    ? mod : familyName;
  const software = mod && mod !== name ? `${name} (${mod})` : name;
  return [software, c.software === "unknown" ? "" : c.software_version].filter(Boolean).join(" ") || "—";
};
