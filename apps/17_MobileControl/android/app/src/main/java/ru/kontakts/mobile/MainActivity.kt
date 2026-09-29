package ru.kontakts.mobile

import android.Manifest
import android.app.Activity
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.*
import android.view.Gravity
import android.view.View
import android.widget.*
import java.nio.charset.StandardCharsets
import java.util.ArrayDeque
import java.util.Locale
import java.util.UUID

class MainActivity : Activity() {
    companion object {
        const val TARGET = "KONTAKTS-8048"
        const val REQ = 1701
        val SERVICE: UUID = UUID.fromString("0000fff0-0000-1000-8000-00805f9b34fb")
        val COMMAND: UUID = UUID.fromString("0000fff1-0000-1000-8000-00805f9b34fb")
        val RESPONSE: UUID = UUID.fromString("0000fff2-0000-1000-8000-00805f9b34fb")
    }

    private val h = Handler(Looper.getMainLooper())
    private lateinit var adapter: BluetoothAdapter
    private var gatt: BluetoothGatt? = null
    private var tx: BluetoothGattCharacteristic? = null
    private var rx: BluetoothGattCharacteristic? = null
    private var busy = false
    private val queue = ArrayDeque<String>()

    private lateinit var status: TextView
    private lateinit var log: TextView
    private lateinit var raw: EditText
    private val state = mutableMapOf<Int, TextView>()

    private val scan = object : ScanCallback() {
        override fun onScanResult(type: Int, result: ScanResult) {
            val name = result.scanRecord?.deviceName ?: safeName(result.device)
            if (name == TARGET) {
                try { adapter.bluetoothLeScanner.stopScan(this) } catch (_: Exception) {}
                ui("FOUND " + TARGET + " RSSI=" + result.rssi)
                connect(result.device)
            }
        }

        override fun onScanFailed(code: Int) {
            ui("SCAN ERROR " + code)
            setStatus("Ошибка сканирования")
        }
    }

    private val callback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, s: Int, n: Int) {
            if (s == BluetoothGatt.GATT_SUCCESS && n == BluetoothProfile.STATE_CONNECTED) {
                ui("CONNECTED")
                h.post {
                    setStatus("Подключено. Поиск сервиса…")
                    if (!g.requestMtu(256)) g.discoverServices()
                }
            } else if (n == BluetoothProfile.STATE_DISCONNECTED || s != BluetoothGatt.GATT_SUCCESS) {
                ui("DISCONNECTED status=" + s)
                h.post {
                    close(g)
                    setStatus("Отключено")
                }
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, s: Int) {
            ui("MTU=" + mtu)
            g.discoverServices()
        }

        override fun onServicesDiscovered(g: BluetoothGatt, s: Int) {
            val svc = g.getService(SERVICE)
            tx = svc?.getCharacteristic(COMMAND)
            rx = svc?.getCharacteristic(RESPONSE)
            h.post {
                if (tx == null || rx == null) {
                    setStatus("FFF0/FFF1/FFF2 не найдены")
                    ui("GATT CONTRACT MISSING")
                } else {
                    setStatus("READY — " + TARGET)
                    ui("READY FFF0 / FFF1 WRITE / FFF2 READ")
                    send("MA01 READ")
                }
            }
        }

        @Suppress("DEPRECATION")
        override fun onCharacteristicWrite(g: BluetoothGatt, c: BluetoothGattCharacteristic, s: Int) {
            if (c.uuid != COMMAND) return
            if (s != BluetoothGatt.GATT_SUCCESS) {
                finish("WRITE ERROR " + s)
            } else {
                h.postDelayed({ readResponse() }, 150)
            }
        }

        @Suppress("DEPRECATION")
        override fun onCharacteristicRead(g: BluetoothGatt, c: BluetoothGattCharacteristic, s: Int) {
            if (Build.VERSION.SDK_INT >= 33 || c.uuid != RESPONSE) return
            response(c.value ?: ByteArray(0), s)
        }

        override fun onCharacteristicRead(g: BluetoothGatt, c: BluetoothGattCharacteristic, value: ByteArray, s: Int) {
            if (c.uuid == RESPONSE) response(value, s)
        }
    }

    override fun onCreate(b: Bundle?) {
        super.onCreate(b)
        adapter = (getSystemService(BLUETOOTH_SERVICE) as BluetoothManager).adapter
        buildUi()
        requestPermissionsIfNeeded()
    }

    private fun buildUi() {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(12), dp(12), dp(12), dp(12))
        }

        root.addView(TextView(this).apply {
            text = "KONTAKTS BLE Control"
            textSize = 24f
        })

        status = TextView(this).apply {
            text = "BLE permissions…"
            textSize = 16f
            setPadding(0, dp(8), 0, dp(8))
        }
        root.addView(status)

        val top = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        top.addView(button("SCAN & CONNECT") { startScan() }, LinearLayout.LayoutParams(0, -2, 1f))
        top.addView(button("DISCONNECT") { disconnect() }, LinearLayout.LayoutParams(0, -2, 1f))
        root.addView(top)

        root.addView(button("MA01 READ / REFRESH") { send("MA01 READ") })

        for (ch in 1..8) root.addView(relayRow(ch))

        raw = EditText(this).apply {
            setSingleLine(true)
            setText("MA01 READ")
        }
        val rr = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        rr.addView(raw, LinearLayout.LayoutParams(0, -2, 1f))
        rr.addView(button("SEND") { send(raw.text.toString()) })
        root.addView(rr)

        log = TextView(this).apply {
            typeface = android.graphics.Typeface.MONOSPACE
            textSize = 12f
            setTextIsSelectable(true)
            setBackgroundColor(Color.rgb(238,238,238))
            setPadding(dp(8), dp(8), dp(8), dp(8))
        }
        root.addView(log, LinearLayout.LayoutParams(-1, dp(260)))
        setContentView(ScrollView(this).apply { addView(root) })
    }

    private fun relayRow(ch: Int): View {
        val row = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        row.addView(TextView(this).apply { text = "DO" + ch }, LinearLayout.LayoutParams(dp(48), dp(48)))
        val s = TextView(this).apply {
            text = "?"
            gravity = Gravity.CENTER
        }
        state[ch] = s
        row.addView(s, LinearLayout.LayoutParams(dp(52), dp(48)))
        row.addView(button("ACTION") { send("MA01 DO" + ch + " ACTION", true) }, LinearLayout.LayoutParams(0, dp(48), 1.3f))
        row.addView(button("ON") { send("MA01 DO" + ch + " ON", true) }, LinearLayout.LayoutParams(0, dp(48), 1f))
        row.addView(button("OFF") { send("MA01 DO" + ch + " OFF", true) }, LinearLayout.LayoutParams(0, dp(48), 1f))
        return row
    }

    private fun button(label: String, action: () -> Unit) =
        Button(this).apply {
            text = label
            setOnClickListener { action() }
        }

    private fun perms(): Array<String> =
        if (Build.VERSION.SDK_INT >= 31)
            arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        else
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)

    private fun requestPermissionsIfNeeded(): Boolean {
        val missing = perms().filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
        return if (missing.isEmpty()) {
            setStatus("Готово. Нажмите SCAN & CONNECT")
            true
        } else {
            requestPermissions(missing.toTypedArray(), REQ)
            false
        }
    }

    override fun onRequestPermissionsResult(r: Int, p: Array<out String>, g: IntArray) {
        super.onRequestPermissionsResult(r, p, g)
        if (r == REQ && g.isNotEmpty() && g.all { it == PackageManager.PERMISSION_GRANTED })
            setStatus("Готово. Нажмите SCAN & CONNECT")
    }

    private fun startScan() {
        if (!requestPermissionsIfNeeded()) return
        if (!adapter.isEnabled) {
            setStatus("Включите Bluetooth")
            return
        }
        disconnect()
        setStatus("Поиск " + TARGET + "…")
        ui("SCAN START")
        try {
            adapter.bluetoothLeScanner.startScan(scan)
            h.postDelayed({
                try { adapter.bluetoothLeScanner.stopScan(scan) } catch (_: Exception) {}
            }, 12000)
        } catch (e: SecurityException) {
            setStatus("Нет BLE permission")
        }
    }

    private fun connect(d: BluetoothDevice) {
        h.post { setStatus("Подключение к " + TARGET + "…") }
        try {
            gatt = d.connectGatt(this, false, callback, BluetoothDevice.TRANSPORT_LE)
        } catch (e: SecurityException) {
            ui("CONNECT SecurityException")
        }
    }

    private fun disconnect() {
        queue.clear()
        busy = false
        val g = gatt ?: return
        try { g.disconnect() } catch (_: Exception) { close(g) }
    }

    private fun close(g: BluetoothGatt) {
        if (gatt === g) gatt = null
        tx = null
        rx = null
        busy = false
        queue.clear()
        try { g.close() } catch (_: Exception) {}
    }

    private fun send(cmd: String, refresh: Boolean = false) {
        val c = cmd.trim()
        if (c.isEmpty()) return
        if (gatt == null || tx == null || rx == null) {
            setStatus("Сначала подключитесь")
            return
        }
        queue.add(c)
        if (refresh && !c.equals("MA01 READ", true)) queue.add("MA01 READ")
        pump()
    }

    @Suppress("DEPRECATION")
    private fun pump() {
        if (busy) return
        val g = gatt ?: return
        val c = tx ?: return
        val cmd = queue.poll() ?: return
        busy = true
        ui("TX > " + cmd)
        val value = cmd.toByteArray(StandardCharsets.UTF_8)
        val ok = try {
            if (Build.VERSION.SDK_INT >= 33)
                g.writeCharacteristic(c, value, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) == BluetoothStatusCodes.SUCCESS
            else {
                c.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                c.value = value
                g.writeCharacteristic(c)
            }
        } catch (_: SecurityException) { false }
        if (!ok) finish("WRITE NOT STARTED")
    }

    private fun readResponse() {
        val g = gatt ?: return finish("NO GATT")
        val c = rx ?: return finish("NO FFF2")
        val ok = try { g.readCharacteristic(c) } catch (_: SecurityException) { false }
        if (!ok) finish("READ NOT STARTED")
    }

    private fun response(bytes: ByteArray, s: Int) {
        h.post {
            if (s != BluetoothGatt.GATT_SUCCESS) return@post finish("READ ERROR " + s)
            val text = String(bytes, StandardCharsets.UTF_8).trim()
            ui("RX < " + text)
            parseState(text)
            busy = false
            h.postDelayed({ pump() }, 80)
        }
    }

    private fun parseState(text: String) {
        val re = Regex("""DO([1-8])=(ON|OFF|1|0)""", RegexOption.IGNORE_CASE)
        for (m in re.findAll(text)) {
            val ch = m.groupValues[1].toInt()
            val v = m.groupValues[2].uppercase(Locale.US)
            val on = v == "ON" || v == "1"
            state[ch]?.apply {
                this.text = if (on) "ON" else "OFF"
                setTextColor(if (on) Color.rgb(0,128,0) else Color.DKGRAY)
            }
        }
    }

    private fun finish(msg: String) {
        h.post {
            ui(msg)
            busy = false
            h.postDelayed({ pump() }, 100)
        }
    }

    private fun setStatus(s: String) { status.text = s }

    private fun ui(s: String) {
        h.post {
            val old = if (::log.isInitialized) log.text.toString() else ""
            if (::log.isInitialized) log.text = (if (old.isEmpty()) s else old + "\n" + s).takeLast(12000)
        }
    }

    private fun safeName(d: BluetoothDevice): String? =
        try { d.name } catch (_: SecurityException) { null }

    private fun dp(v: Int) = (v * resources.displayMetrics.density + 0.5f).toInt()
}
