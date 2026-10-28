package com.thumbremote.app

import android.Manifest
import android.annotation.TargetApi
import android.app.*
import android.bluetooth.BluetoothDevice
import android.content.*
import android.content.pm.PackageManager
import android.content.pm.ServiceInfo
import android.os.*
import com.thumbremote.app.ble.BleClient
import com.thumbremote.app.ble.BleFrame
import com.thumbremote.app.ble.BlePhase

/** Only this coordinator joins validated BLE events to the gesture executor. */
@TargetApi(31)
class RemoteControlService : Service() {
    companion object {
        const val ACTION_START = "com.thumbremote.app.START_REMOTE"
        const val ACTION_STOP = "com.thumbremote.app.STOP_REMOTE"
        const val EXTRA_DEVICE = "device"
        const val EXTRA_TICKET = "ticket"
        private const val CHANNEL = "remote_control"
        private const val NOTIFICATION = 30
        var current: RemoteControlService? = null; private set

        fun ensureChannel(context: Context) {
            context.getSystemService(NotificationManager::class.java).createNotificationChannel(
                NotificationChannel(CHANNEL, "遥控运行状态", NotificationManager.IMPORTANCE_LOW))
        }
        fun notificationsVisible(context: Context): Boolean {
            val manager = context.getSystemService(NotificationManager::class.java)
            return (Build.VERSION.SDK_INT < 33 || context.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED) &&
                manager.areNotificationsEnabled() && manager.getNotificationChannel(CHANNEL)?.importance?.let { it != NotificationManager.IMPORTANCE_NONE } == true
        }
        fun screenReady(context: Context): Boolean =
            context.getSystemService(PowerManager::class.java).isInteractive &&
                !context.getSystemService(KeyguardManager::class.java).isKeyguardLocked

        fun startFailure(context: Context, checkManual: Boolean = true): String? = RemoteStartConditions(
            notificationsVisible(context),
            context.checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED &&
                context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED,
            ThumbAccessibilityService.current != null,
            screenReady(context),
            checkManual && ThumbAccessibilityService.current?.isActive == true
        ).failure()
    }

    inner class LocalBinder : Binder() { val service: RemoteControlService get() = this@RemoteControlService }
    private val binder = LocalBinder()
    private val main = Handler(Looper.getMainLooper())
    private val gate = RemoteControlSession()
    private var client: BleClient? = null
    private var ticket = -1L
    private var controlGeneration = -1L
    private var bleGeneration = -1L
    private var startingGatt = false
    private var stopping = false
    private var receiverRegistered = false
    private var notificationText = "正在连接开发板"
    private val screenReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action == Intent.ACTION_SCREEN_OFF) stopRemote("屏幕已关闭，遥控结束；不会补发")
        }
    }
    private val guardian = object : Runnable {
        override fun run() {
            if (!gate.active) return
            val failure = startFailure(this@RemoteControlService, false)
            if (failure != null) stopRemote("遥控结束：$failure")
            else main.postDelayed(this, 1000)
        }
    }

    override fun onCreate() { super.onCreate(); current = this }
    override fun onBind(intent: Intent?): IBinder = binder

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP) {
            if (intent.getLongExtra(EXTRA_TICKET, -1) == ticket) stopRemote("已停止遥控，不会补发")
            return START_NOT_STICKY
        }
        val requestedTicket = intent?.getLongExtra(EXTRA_TICKET, -1) ?: -1
        if (intent?.action != ACTION_START || !RemoteControlStatus.isCurrent(requestedTicket)) {
            if (!gate.active) stopSelf()
            return START_NOT_STICKY
        }
        if (gate.active) return START_NOT_STICKY
        stopping = false
        notificationText = "正在连接开发板"
        ticket = requestedTicket
        val device = if (Build.VERSION.SDK_INT >= 33) intent.getParcelableExtra(EXTRA_DEVICE, BluetoothDevice::class.java)
            else legacyDevice(intent)
        val failure = startFailure(this)
        if (device == null || failure != null) {
            stopRemote(failure ?: "未选择开发板，请回到App重新选择")
            return START_NOT_STICKY
        }
        try {
            ensureChannel(this)
            startForeground(NOTIFICATION, buildNotification("正在连接开发板"), ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE)
            controlGeneration = gate.begin()
            val filter = IntentFilter(Intent.ACTION_SCREEN_OFF)
            if (Build.VERSION.SDK_INT >= 33) registerReceiver(screenReceiver, filter, Context.RECEIVER_NOT_EXPORTED)
            else registerReceiver(screenReceiver, filter)
            receiverRegistered = true
            client = BleClient(this, onAccepted = ::onDirection, changed = ::onBleChanged)
            client?.onStart()
            startingGatt = true
            try { client?.connect(BleClient.FoundDevice(device, "ThumbRemote")) }
            finally { startingGatt = false }
            onBleChanged()
            if (gate.active) main.post(guardian)
        } catch (_: RuntimeException) {
            stopRemote("无法启动遥控服务，请检查蓝牙、通知与权限后主动重试")
        }
        return START_NOT_STICKY
    }

    @Deprecated("Only used on Android 12")
    private fun legacyDevice(intent: Intent): BluetoothDevice? = intent.getParcelableExtra(EXTRA_DEVICE)

    private fun buildNotification(text: String): Notification {
        val immutable = PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        val open = PendingIntent.getActivity(this, 0, Intent(this, MainActivity::class.java), immutable)
        val stop = PendingIntent.getService(this, 1, Intent(this, RemoteControlService::class.java)
            .setAction(ACTION_STOP).putExtra(EXTRA_TICKET, ticket), immutable)
        return Notification.Builder(this, CHANNEL).setSmallIcon(R.drawable.ic_thumb_remote)
            .setContentTitle("ThumbRemote 遥控").setContentText(text).setContentIntent(open)
            .setOngoing(true).setOnlyAlertOnce(true)
            .addAction(Notification.Action.Builder(null, "停止遥控", stop).build()).build()
    }

    private fun publish(text: String) {
        if (!gate.active || stopping) return
        RemoteControlStatus.update(text, gate)
        if (text != notificationText) {
            notificationText = text
            try { getSystemService(NotificationManager::class.java).notify(NOTIFICATION, buildNotification(text)) }
            catch (_: RuntimeException) { stopRemote("通知不可用，遥控已停止") }
        }
    }

    private fun onBleChanged() {
        if (!gate.active || stopping) return
        val ble = client ?: return
        when (ble.session.phase) {
            BlePhase.READY -> {
                bleGeneration = ble.session.generation
                gate.ready(controlGeneration)
                publish("遥控已开启，可切换到目标应用")
            }
            BlePhase.IDLE, BlePhase.ERROR -> if (!startingGatt) stopRemote("遥控结束：${ble.message}")
            else -> publish("遥控连接中：${ble.message}")
        }
    }

    private fun onDirection(frame: BleFrame, generation: Long, receivedElapsed: Long) {
        if (stopping || generation != bleGeneration || !RemoteControlStatus.isCurrent(ticket)) return
        val ble = client ?: return
        if (ble.session.phase != BlePhase.READY || frame.sessionId != ble.session.sessionId) return
        val failure = startFailure(this, false)
        if (failure != null) { stopRemote("遥控结束：$failure"); return }
        val request = gate.accept(controlGeneration, frame.sequence, frame.code, receivedElapsed, SystemClock.elapsedRealtime())
        if (request != null) dispatchRemote(request)
        publish("遥控已开启，可切换到目标应用")
    }

    private fun dispatchRemote(request: RemoteRequest) {
        val failure = startFailure(this, false)
        if (failure != null) { stopRemote("遥控结束：$failure"); return }
        val accepted = ThumbAccessibilityService.current?.executeRemote(request.command) { result ->
            if (gate.finish(request.generation, request.id, SystemClock.elapsedRealtime(), result)) {
                drainRemote()
                publish("遥控已开启，可切换到目标应用")
            }
        } == true
        if (!accepted && gate.reject(request.generation, request.id, SystemClock.elapsedRealtime(), "执行器忙碌或屏幕状态变化，未执行")) drainRemote()
    }

    private fun drainRemote() {
        if (stopping || !RemoteControlStatus.isCurrent(ticket) || client?.session?.phase != BlePhase.READY) return
        val next = gate.takeNext(controlGeneration, SystemClock.elapsedRealtime()) ?: return
        dispatchRemote(next)
    }

    fun stopRemote(reason: String) {
        if (stopping) return
        stopping = true
        val owned = RemoteControlStatus.isCurrent(ticket)
        gate.stop() // Invalidate execution first, including callbacks from an already dispatched gesture.
        main.removeCallbacksAndMessages(null)
        ThumbAccessibilityService.current?.cancelRemote()
        client?.onStop(); client = null
        if (receiverRegistered) { unregisterReceiver(screenReceiver); receiverRegistered = false }
        if (owned) { RemoteControlStatus.update(reason, gate); RemoteControlStatus.end(reason) }
        stopForeground(STOP_FOREGROUND_REMOVE)
        stopSelf()
    }

    override fun onTaskRemoved(rootIntent: Intent?) { stopRemote("任务已关闭，遥控结束"); super.onTaskRemoved(rootIntent) }
    override fun onDestroy() {
        stopRemote("遥控服务已结束，不会自动恢复")
        if (current === this) current = null
        super.onDestroy()
    }
}
