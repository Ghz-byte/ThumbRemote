package com.thumbremote.app.ble

import android.Manifest
import android.annotation.TargetApi
import android.bluetooth.*
import android.bluetooth.le.*
import android.content.*
import android.content.pm.PackageManager
import android.os.*
import java.security.SecureRandom
import java.util.UUID

/** API31+ adapter, owned by a visible page or an explicitly started foreground service. */
@TargetApi(31)
class BleClient(
    private val context: Context,
    private val onAccepted: ((BleFrame, Long, Long) -> Unit)? = null,
    private val changed: () -> Unit
) {
    data class FoundDevice(val device: BluetoothDevice, val label: String)
    val session = BleReceiveSession()
    var selectedDevice: FoundDevice? = null; private set
    var message = "尚未连接，点击扫描开始"; private set
    var scanning = false; private set
    val devices: List<FoundDevice> get() = found.values.toList()
    private val found = linkedMapOf<String, FoundDevice>()
    private val main = Handler(Looper.getMainLooper())
    private val random = SecureRandom()
    @Volatile private var generation = 0L
    @Volatile private var gatt: BluetoothGatt? = null
    private var foreground = false
    private var receiverRegistered = false
    private var timeout: Runnable? = null
    private var scanCallback: ScanCallback? = null
    private var scanner: BluetoothLeScanner? = null
    private var event: BluetoothGattCharacteristic? = null
    private var start: BluetoothGattCharacteristic? = null
    private var previousSessionId = 0L
    private val bluetoothState = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action == BluetoothAdapter.ACTION_STATE_CHANGED &&
                intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, -1) != BluetoothAdapter.STATE_ON) {
                stop("蓝牙已关闭，本次连接结束")
            }
        }
    }

    fun onStart() {
        foreground = true
        if (!receiverRegistered) {
            val filter = IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED)
            if (Build.VERSION.SDK_INT >= 33) context.registerReceiver(bluetoothState, filter, Context.RECEIVER_EXPORTED)
            else context.registerReceiver(bluetoothState, filter)
            receiverRegistered = true
        }
    }

    fun onResume() {
        if ((scanning || gatt != null) && !hasPermissions()) stop("附近设备权限已撤销", true)
    }

    fun onStop() {
        foreground = false
        if (receiverRegistered) { context.unregisterReceiver(bluetoothState); receiverRegistered = false }
        stop("已离开页面，连接已结束；返回后请主动连接")
    }

    fun hasPermissions(): Boolean =
        context.checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED &&
        context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED

    private fun cancelTimeout() { timeout?.let(main::removeCallbacks); timeout = null }

    private fun armTimeout(milliseconds: Long, reason: String) {
        cancelTimeout()
        val captured = generation
        timeout = Runnable { if (captured == generation && session.phase != BlePhase.READY) stop(reason, true) }
            .also { main.postDelayed(it, milliseconds) }
    }

    fun stop(reason: String = "已断开，不会补发旧动作", failed: Boolean = false) {
        generation++  // Invalidate callbacks before any platform cleanup.
        cancelTimeout()
        session.end(generation, failed)
        stopScan()
        val old = gatt; gatt = null; event = null; start = null
        val cleaned = if (old != null) {
            BlePlatformBoundary.close({
                if (context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) old.disconnect()
            }, { old.close() })
        } else true
        found.clear()
        message = if (cleaned) reason else "$reason；系统连接释放未完成，本地会话已作废"
        changed()
    }

    fun stopScan() {
        val oldCallback = scanCallback; scanCallback = null
        val oldScanner = scanner; scanner = null; scanning = false
        if (oldCallback != null && context.checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
            try { oldScanner?.stopScan(oldCallback) } catch (_: SecurityException) { /* Permission changed during cleanup. */ }
        }
    }

    fun finishScan() {
        if (!scanning) return
        cancelTimeout(); stopScan()
        message = if (found.isEmpty()) "未找到设备，请确认开发板供电后重试" else "扫描结束，请选择开发板连接"
        changed()
    }

    fun startScan() {
        if (!foreground) return
        stop("准备扫描")
        if (context.checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED ||
            context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            stop("请允许附近设备权限后重试", true); return
        }
        if (!context.packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            stop("手机不支持蓝牙低功耗", true); return
        }
        try {
            val adapter = context.getSystemService(BluetoothManager::class.java)?.adapter
            if (adapter == null || !adapter.isEnabled) { stop("请先在系统设置中开启蓝牙", true); return }
            val activeScanner = adapter.bluetoothLeScanner
            if (activeScanner == null) { stop("蓝牙扫描器暂不可用，请重试", true); return }
            val captured = generation
            val callback = object : ScanCallback() {
                override fun onScanResult(callbackType: Int, result: ScanResult) {
                    if (captured != generation) return
                    main.post {
                        if (captured != generation || !scanning || !foreground) return@post
                        if (context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                            stop("附近设备权限已撤销", true); return@post
                        }
                        val read = BlePlatformBoundary.attempt {
                            val address = result.device.address
                            val name = result.scanRecord?.deviceName ?: "未命名设备"
                            val number = found.keys.indexOf(address).let { if (it < 0) found.size + 1 else it + 1 }
                            val label = "$name · 设备$number"
                            val previous = found[address]
                            found[address] = FoundDevice(result.device, label)
                            if (previous?.label != label) changed()
                        }
                        if (!read) stop("读取扫描结果时附近设备权限失效", true)
                    }
                }
                override fun onScanFailed(errorCode: Int) {
                    main.post { if (captured == generation && scanning) stop("扫描失败（$errorCode），请稍后重试", true) }
                }
            }
            scanner = activeScanner; scanCallback = callback; scanning = true
            message = "正在扫描，最多10秒；请点选开发板"
            activeScanner.startScan(listOf(ScanFilter.Builder().setServiceUuid(ParcelUuid(UUID.fromString(BleEventProtocol.SERVICE))).build()),
                ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build(), callback)
            timeout = Runnable { if (captured == generation) finishScan() }.also { main.postDelayed(it, 10_000) }
            changed()
        } catch (_: SecurityException) { stop("附近设备权限不可用", true) }
        catch (_: IllegalStateException) { stop("蓝牙状态已变化，请重试", true) }
    }

    fun connect(selected: FoundDevice) {
        if (!foreground) return
        stop("正在连接 ${selected.label}")
        selectedDevice = selected
        if (context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED ||
            context.checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            stop("请允许附近设备权限后重试", true); return
        }
        var id: Long
        do { id = random.nextInt().toLong() and 0xffffffffL } while (id == 0L || id == previousSessionId)
        previousSessionId = id
        session.begin(generation, id)
        val captured = generation
        try {
            gatt = selected.device.connectGatt(context, false, callbacks(captured), BluetoothDevice.TRANSPORT_LE)
            if (gatt == null) { stop("无法创建蓝牙连接", true); return }
            armTimeout(10_000, "连接超时，请主动重试"); changed()
        } catch (_: SecurityException) { stop("附近设备权限不可用", true) }
        catch (_: IllegalStateException) { stop("蓝牙状态已变化，请重试", true) }
    }

    private fun dispatch(captured: Long, source: BluetoothGatt, action: () -> Unit) {
        if (captured != generation) return
        main.post {
            if (captured != generation || source !== gatt || !foreground) return@post
            if (!hasPermissions()) { stop("附近设备权限已撤销", true); return@post }
            try { action() } catch (_: SecurityException) { stop("附近设备权限不可用", true) }
        }
    }

    private fun callbacks(captured: Long) = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(source: BluetoothGatt, status: Int, newState: Int) = dispatch(captured, source) {
            if (status != BluetoothGatt.GATT_SUCCESS || newState == BluetoothProfile.STATE_DISCONNECTED) {
                stop("连接已断开（$status），不会补发旧动作", status != BluetoothGatt.GATT_SUCCESS)
            } else if (newState == BluetoothProfile.STATE_CONNECTED &&
                session.advance(captured, BlePhase.CONNECTING, BlePhase.DISCOVERING)) discover(source)
        }
        override fun onServicesDiscovered(source: BluetoothGatt, status: Int) = dispatch(captured, source) {
            if (session.phase != BlePhase.DISCOVERING) return@dispatch
            if (status != BluetoothGatt.GATT_SUCCESS) stop("服务发现失败（$status）", true) else subscribe(source)
        }
        override fun onDescriptorWrite(source: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) = dispatch(captured, source) {
            if (session.phase != BlePhase.SUBSCRIBING || descriptor.uuid != UUID.fromString(BleEventProtocol.CCCD)) return@dispatch
            if (status != BluetoothGatt.GATT_SUCCESS) stop("订阅失败（$status）", true) else writeSession(source)
        }
        override fun onCharacteristicWrite(source: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) = dispatch(captured, source) {
            if (session.phase != BlePhase.STARTING || characteristic.uuid != UUID.fromString(BleEventProtocol.START)) return@dispatch
            if (status != BluetoothGatt.GATT_SUCCESS) stop("会话启动失败（$status）", true)
            else { session.writeSucceeded(captured); handshakeChanged() }
        }
        override fun onCharacteristicChanged(source: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
            if (Build.VERSION.SDK_INT >= 33 && characteristic.uuid == UUID.fromString(BleEventProtocol.EVENT)) notification(captured, source, value.copyOf())
        }
        @Deprecated("Platform callback for Android 12 only")
        override fun onCharacteristicChanged(source: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            if (Build.VERSION.SDK_INT < 33 && characteristic.uuid == UUID.fromString(BleEventProtocol.EVENT)) notification(captured, source, characteristic.value?.copyOf() ?: byteArrayOf())
        }
    }

    private fun discover(source: BluetoothGatt) {
        if (context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) { stop("附近设备权限已撤销", true); return }
        message = "正在发现服务"; armTimeout(10_000, "服务发现超时")
        if (!source.discoverServices()) stop("无法开始服务发现", true) else changed()
    }

    private fun subscribe(source: BluetoothGatt) {
        if (context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) { stop("附近设备权限已撤销", true); return }
        val service = source.getService(UUID.fromString(BleEventProtocol.SERVICE))
        val notification = service?.getCharacteristic(UUID.fromString(BleEventProtocol.EVENT))
        val control = service?.getCharacteristic(UUID.fromString(BleEventProtocol.START))
        val cccd = notification?.getDescriptor(UUID.fromString(BleEventProtocol.CCCD))
        if (notification == null || control == null || cccd == null ||
            notification.properties and BluetoothGattCharacteristic.PROPERTY_NOTIFY == 0 ||
            control.properties and BluetoothGattCharacteristic.PROPERTY_WRITE == 0) {
            stop("设备服务不符合接收协议，请检查固件版本", true); return
        }
        if (!session.advance(generation, BlePhase.DISCOVERING, BlePhase.SUBSCRIBING)) return
        event = notification; start = control
        message = "正在订阅方向通知"; armTimeout(5_000, "通知订阅超时")
        if (!source.setCharacteristicNotification(notification, true)) { stop("无法开启通知", true); return }
        val value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
        val started = if (Build.VERSION.SDK_INT >= 33) source.writeDescriptor(cccd, value) == BluetoothStatusCodes.SUCCESS
            else { cccd.value = value; source.writeDescriptor(cccd) }
        if (!started) stop("无法写入通知订阅", true) else changed()
    }

    private fun writeSession(source: BluetoothGatt) {
        if (context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) { stop("附近设备权限已撤销", true); return }
        val control = start ?: return
        if (!session.advance(generation, BlePhase.SUBSCRIBING, BlePhase.STARTING)) return
        message = "正在确认本次接收会话"; armTimeout(5_000, "会话确认超时，请重新连接")
        val value = BleEventProtocol.startSession(session.sessionId)
        val started = if (Build.VERSION.SDK_INT >= 33) source.writeCharacteristic(control, value, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) == BluetoothStatusCodes.SUCCESS
            else { control.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT; control.value = value; source.writeCharacteristic(control) }
        if (!started) stop("无法启动接收会话", true) else changed()
    }

    private fun notification(captured: Long, source: BluetoothGatt, value: ByteArray) {
        // Capture before queuing onto main: blocked UI must not make old inputs fresh.
        val receivedElapsed = SystemClock.elapsedRealtime()
        dispatch(captured, source) {
            val accepted = session.receive(captured, value, System.currentTimeMillis())
            handshakeChanged()
            if (accepted != null && captured == generation) onAccepted?.invoke(accepted, captured, receivedElapsed)
        }
    }

    private fun handshakeChanged() {
        if (session.phase == BlePhase.READY) { cancelTimeout(); message = "可以接收 · 仅显示方向，不控制手机" }
        changed()
    }
}
