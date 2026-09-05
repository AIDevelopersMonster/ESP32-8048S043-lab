(() => {
  const secure = window.isSecureContext;
  const serialSupported = "serial" in navigator;

  console.log("KONTAKTS ESP32-8048S043 Firmware Catalog");
  console.log("Programmer: Sol");
  console.log("Engineer: Alex Malachevsky");
  console.log(`Secure context: ${secure}`);
  console.log(`Web Serial supported: ${serialSupported}`);
  console.log("App 01: Six-Card Serial Deck v1.0.0 - WEB FLASHER PHYSICAL PASS");
  console.log("App 02: Mixed Widgets v0.1.1 - functional PHYSICAL PASS + cosmetic fix candidate");
  console.log("App 02 video: https://youtube.com/shorts/LwmW8UwDED0");
})();
