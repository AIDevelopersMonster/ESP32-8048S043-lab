(() => {
  const secure = window.isSecureContext;
  const serialSupported = "serial" in navigator;

  console.log("ESP32-8048S043 App 01 Web Flasher");
  console.log(`Secure context: ${secure}`);
  console.log(`Web Serial supported: ${serialSupported}`);
  console.log("Firmware: Six-Card Serial Deck v1.0.0");
  console.log("Physical demo: https://youtube.com/shorts/0I5JL6jt8e0");
})();
