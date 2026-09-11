(() => {
  const secure = window.isSecureContext;
  const serialSupported = "serial" in navigator;
  const grid = document.getElementById("firmware-grid");
  const sdSlot = document.getElementById("sd-library-slot");

  function escapeHtml(value) {
    return String(value ?? "")
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#039;");
  }

  function isCandidate(status) {
    return /CANDIDATE|PENDING|BUILD/i.test(status || "");
  }

  function appNumber(id) {
    const match = String(id || "").match(/app(\d+)/i);
    return match ? match[1].padStart(2, "0") : String(id || "").toUpperCase();
  }

  function renderFirmware(firmware) {
    const status = escapeHtml(firmware.status);
    const name = escapeHtml(firmware.name);
    const version = escapeHtml(firmware.version);
    const description = escapeHtml(firmware.description);
    const manifest = escapeHtml(firmware.manifest);
    const source = escapeHtml(firmware.source);
    const video = firmware.video ? escapeHtml(firmware.video) : null;
    const number = appNumber(firmware.id);

    return `
      <article class="card">
        <span class="status ${isCandidate(firmware.status) ? "candidate" : ""}">${status}</span>
        <h2>App ${number} — ${name}</h2>
        <div class="version">Version ${version}</div>
        <p>${description}</p>
        <div class="installer">
          <esp-web-install-button manifest="${manifest}">
            <button slot="activate">INSTALL APP ${number}</button>
            <span slot="unsupported">Use Chrome or Edge with Web Serial support.</span>
            <span slot="not-allowed">Open this installer from the HTTPS GitHub Pages site.</span>
          </esp-web-install-button>
        </div>
        <div class="links">
          ${video ? `<a href="${video}">Physical test video</a>` : ""}
          ${source ? `<a href="${source}">Source</a>` : ""}
        </div>
      </article>`;
  }

  function renderSdLibrary(sd) {
    if (!sdSlot || !sd) return;
    const name = escapeHtml(sd.name);
    const status = escapeHtml(sd.status);
    const description = escapeHtml(sd.description);
    const download = escapeHtml(sd.download);
    const manifest = escapeHtml(sd.manifest);
    const source = escapeHtml(sd.source);

    sdSlot.innerHTML = `
      <article class="card sd-card">
        <span class="status">${status}</span>
        <h2>${name}</h2>
        <p>${description}</p>
        <div class="installer sd-download">
          <a class="download-button" href="${download}">DOWNLOAD SD LIBRARY</a>
        </div>
        <p class="sd-note"><strong>Install:</strong> extract the ZIP directly into the root of the FAT32 SD card. The archive is not firmware and must not be flashed.</p>
        <div class="links">
          <a href="${manifest}">SD manifest</a>
          <a href="${source}">SD source tree</a>
        </div>
      </article>`;
  }

  async function loadCatalog() {
    try {
      const response = await fetch("./firmware-list.json", { cache: "no-store" });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      const catalog = await response.json();
      const firmwares = Array.isArray(catalog.firmwares) ? catalog.firmwares : [];
      if (!firmwares.length) throw new Error("catalog contains no firmware entries");

      renderSdLibrary(catalog.sd_library);
      grid.innerHTML = firmwares.map(renderFirmware).join("");

      console.log(`${catalog.project}: loaded ${firmwares.length} firmware entries`);
      for (const firmware of firmwares) {
        console.log(`${firmware.id}: ${firmware.name} ${firmware.version} - ${firmware.status}`);
      }
    } catch (error) {
      console.error("Firmware catalog load failed", error);
      grid.innerHTML = `<div class="catalog-error"><strong>Firmware catalog could not be loaded.</strong><br>${escapeHtml(error.message)}</div>`;
      if (sdSlot) sdSlot.innerHTML = "";
    }
  }

  console.log("KONTAKTS ESP32-8048S043 Firmware Catalog");
  console.log(`Secure context: ${secure}`);
  console.log(`Web Serial supported: ${serialSupported}`);
  loadCatalog();
})();
