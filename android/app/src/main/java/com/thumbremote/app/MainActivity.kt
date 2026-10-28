package com.thumbremote.app

import android.app.Activity
import android.content.ActivityNotFoundException
import android.content.ComponentName
import android.content.Intent
import android.content.Context
import android.content.ServiceConnection
import android.graphics.Rect
import android.os.Build
import android.os.Bundle
import android.os.IBinder
import android.provider.Settings
import android.view.WindowInsets
import android.widget.Button
import android.widget.TextView
import android.widget.LinearLayout
import android.Manifest
import android.content.pm.PackageManager
import com.thumbremote.app.ble.BleClient
import com.thumbremote.app.ble.BlePhase
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class MainActivity : Activity() {
    private val observer: () -> Unit = { render() }
    private var bleClient: BleClient? = null
    private var pageStarted = false
    private var permissionRequestedWhileStarted = false
    private val controlObserver: () -> Unit = { renderControl(); render(); renderBle(); bindControlIfRunning() }
    private var controlBound = false
    private var controlService: RemoteControlService? = null
    private val controlConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, binder: IBinder?) {
            controlService = (binder as? RemoteControlService.LocalBinder)?.service
            renderControl()
        }
        override fun onServiceDisconnected(name: ComponentName?) { controlService = null; renderControl() }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        val root = findViewById<android.view.View>(R.id.root)
        root.setOnApplyWindowInsetsListener { view, insets ->
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                val bars = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
            } else {
                @Suppress("DEPRECATION")
                view.setPadding(insets.systemWindowInsetLeft, insets.systemWindowInsetTop,
                    insets.systemWindowInsetRight, insets.systemWindowInsetBottom)
            }
            insets
        }
        root.requestApplyInsets()
        if (Build.VERSION.SDK_INT >= 31) {
            RemoteControlService.ensureChannel(this)
            bleClient = BleClient(this) { renderBle(); renderControl() }
        }
        findViewById<Button>(R.id.remote_start).setOnClickListener { startRemote() }
        findViewById<Button>(R.id.remote_stop).setOnClickListener {
            if (Build.VERSION.SDK_INT >= 31) {
                val activeService = controlService ?: RemoteControlService.current
                activeService?.stopRemote("已停止遥控，不会补发")
                if (RemoteControlStatus.running) RemoteControlStatus.end("已停止遥控，不会补发")
                stopService(Intent(this, RemoteControlService::class.java))
            }
        }
        findViewById<Button>(R.id.ble_scan).setOnClickListener {
            if (Build.VERSION.SDK_INT < 31) return@setOnClickListener
            val client = bleClient ?: return@setOnClickListener
            if (client.scanning) client.finishScan()
            else if (checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED ||
                checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                permissionRequestedWhileStarted = pageStarted
                requestPermissions(arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT), 101)
            } else client.startScan()
        }
        findViewById<Button>(R.id.ble_disconnect).setOnClickListener { bleClient?.stop() }
        findViewById<Button>(R.id.ble_clear).setOnClickListener { bleClient?.session?.clearRecords(); renderBle() }
        findViewById<Button>(R.id.open_settings).setOnClickListener {
            try {
                startActivity(Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS))
            } catch (_: ActivityNotFoundException) {
                RemoteStatus.publish(getString(R.string.settings_failed))
            }
        }
        findViewById<Button>(R.id.swipe_up).setOnClickListener { request(GestureCommand.SWIPE_UP) }
        findViewById<Button>(R.id.swipe_down).setOnClickListener { request(GestureCommand.SWIPE_DOWN) }
        findViewById<Button>(R.id.tap).setOnClickListener { request(GestureCommand.TAP) }
        findViewById<Button>(R.id.cancel).setOnClickListener {
            ThumbAccessibilityService.current?.cancelByUser()
        }
    }

    override fun onStart() {
        super.onStart()
        pageStarted = true
        if (Build.VERSION.SDK_INT >= 31) bleClient?.onStart()
        RemoteStatus.observe(observer)
        RemoteControlStatus.observe(controlObserver)
        bindControlIfRunning()
        renderControl()
        render()
    }

    override fun onResume() {
        super.onResume()
        if (Build.VERSION.SDK_INT >= 31) bleClient?.onResume()
        renderBle()
        render()
    }

    override fun onStop() {
        pageStarted = false
        permissionRequestedWhileStarted = false
        if (Build.VERSION.SDK_INT >= 31) bleClient?.onStop()
        RemoteStatus.stopObserving(observer)
        RemoteControlStatus.stopObserving(controlObserver)
        if (controlBound) { unbindService(controlConnection); controlBound = false; controlService = null }
        super.onStop()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 101 && Build.VERSION.SDK_INT >= 31) {
            val canContinue = pageStarted && permissionRequestedWhileStarted
            permissionRequestedWhileStarted = false
            if (canContinue && bleClient?.hasPermissions() == true) bleClient?.startScan()
            else bleClient?.stop("未获得附近设备权限或页面已离开，请主动重试")
        }
        if (requestCode == 102) {
            RemoteStatus.publish("通知授权流程已结束，请回到遥控区域主动点击开始")
            renderControl()
        }
    }

    private fun bindControlIfRunning() {
        if (Build.VERSION.SDK_INT < 31 || !pageStarted || controlBound || !RemoteControlStatus.running) return
        controlBound = bindService(Intent(this, RemoteControlService::class.java), controlConnection, Context.BIND_AUTO_CREATE)
    }

    private fun startRemote() {
        if (Build.VERSION.SDK_INT < 31 || !pageStarted || RemoteControlStatus.running) return
        if (Build.VERSION.SDK_INT >= 33 && checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            RemoteStatus.publish("请允许通知以显示遥控停止按钮，授权后再主动开始")
            requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 102)
            return
        }
        val selected = bleClient?.selectedDevice
        val failure = RemoteControlService.startFailure(this)
        if (failure != null || selected == null) {
            RemoteStatus.publish(failure ?: "请先在接收测试中扫描并选择开发板，再开始遥控")
            findViewById<TextView>(R.id.remote_state).text = failure ?: "请先扫描并选择开发板"
            return
        }
        val ticket = RemoteControlStatus.reserve() ?: return
        bleClient?.stop("已转交遥控服务，接收测试暂停")
        try {
            startForegroundService(Intent(this, RemoteControlService::class.java).setAction(RemoteControlService.ACTION_START)
                .putExtra(RemoteControlService.EXTRA_TICKET, ticket).putExtra(RemoteControlService.EXTRA_DEVICE, selected.device))
            bindControlIfRunning()
        } catch (_: RuntimeException) {
            if (RemoteControlStatus.isCurrent(ticket)) RemoteControlStatus.end("遥控服务启动失败，请保持App前台后重试")
        }
    }

    private fun renderControl() {
        findViewById<TextView>(R.id.remote_state).text = if (Build.VERSION.SDK_INT < 31) "遥控需要 Android 12 或更高版本"
            else RemoteControlStatus.message + "\n收到 ${RemoteControlStatus.received} · 未执行 ${RemoteControlStatus.skipped}"
        findViewById<Button>(R.id.remote_start).isEnabled = Build.VERSION.SDK_INT >= 31 && !RemoteControlStatus.running
        findViewById<Button>(R.id.remote_stop).isEnabled = RemoteControlStatus.running
        findViewById<TextView>(R.id.remote_records).text = RemoteControlStatus.records.asReversed().joinToString("\n") {
            val name = when (it.code) { 1 -> "上"; 2 -> "下"; 3 -> "左"; 4 -> "右"; 5 -> "单击"; else -> "未知" }
            "$name #${it.sequence} · ${it.result}"
        }
    }

    private fun renderBle() {
        if (Build.VERSION.SDK_INT < 31) {
            findViewById<TextView>(R.id.ble_status).text = getString(R.string.ble_old_android)
            findViewById<Button>(R.id.ble_scan).isEnabled = false
            findViewById<Button>(R.id.ble_disconnect).isEnabled = false
            findViewById<Button>(R.id.ble_clear).isEnabled = false
            return
        }
        val client = bleClient ?: return
        val state = client.session
        findViewById<TextView>(R.id.ble_status).text = client.message
        findViewById<Button>(R.id.ble_scan).setText(if (client.scanning) R.string.ble_stop_scan else R.string.ble_scan)
        findViewById<Button>(R.id.ble_scan).isEnabled = !RemoteControlStatus.running && state.phase in listOf(BlePhase.IDLE, BlePhase.ERROR)
        findViewById<Button>(R.id.ble_disconnect).isEnabled = client.scanning || state.phase !in listOf(BlePhase.IDLE, BlePhase.ERROR)
        val time = SimpleDateFormat("HH:mm:ss", Locale.getDefault())
        fun direction(code: Int): String = if (code == 5) "单击" else getString(when (code) {
            1 -> R.string.ble_up; 2 -> R.string.ble_down; 3 -> R.string.ble_left; else -> R.string.ble_right
        })
        val latest = state.records.lastOrNull()
        findViewById<TextView>(R.id.ble_latest).text = getString(R.string.ble_latest_format,
            latest?.let { direction(it.frame.code) + " · " + time.format(Date(it.receivedAt)) } ?: getString(R.string.ble_no_events),
            state.accepted, state.dropped, state.gaps)
        findViewById<TextView>(R.id.ble_records).text = state.records.asReversed().joinToString("\n") {
            time.format(Date(it.receivedAt)) + "  " + direction(it.frame.code) + "  #" + it.frame.sequence
        }
        val list = findViewById<LinearLayout>(R.id.ble_devices)
        val labels = client.devices.map { it.label }
        if (list.tag != labels) {
            list.removeAllViews()
            client.devices.forEach { selected ->
                list.addView(Button(this).apply {
                    text = selected.label; isAllCaps = false; minHeight = (48 * resources.displayMetrics.density).toInt()
                    setOnClickListener { if (pageStarted) client.connect(selected) }
                })
            }
            list.tag = labels
        }
    }

    private fun isAuthorized(): Boolean {
        val expected = ComponentName(this, ThumbAccessibilityService::class.java)
        val enabled = Settings.Secure.getString(contentResolver, Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES)
        return enabled.orEmpty().split(':').any { ComponentName.unflattenFromString(it) == expected }
    }

    private fun request(command: GestureCommand) {
        val service = ThumbAccessibilityService.current
        if (!isAuthorized()) {
            RemoteStatus.publish(getString(R.string.no_permission))
        } else if (service == null) {
            RemoteStatus.publish(getString(R.string.not_running))
        } else {
            service.arm(command)
        }
        // On small displays the result card starts below the fold. Make the
        // countdown/error visible immediately, before the user switches apps.
        val result = findViewById<TextView>(R.id.result)
        result.post {
            result.requestRectangleOnScreen(Rect(0, 0, result.width, result.height), false)
        }
    }

    private fun render() {
        val service = ThumbAccessibilityService.current
        findViewById<TextView>(R.id.service_status).text = getString(R.string.status_format,
            getString(if (isAuthorized()) R.string.authorized else R.string.unauthorized),
            getString(if (service != null) R.string.connected else R.string.disconnected))
        findViewById<TextView>(R.id.result).text = RemoteStatus.message ?: getString(R.string.initial_result)
        val active = service?.isActive == true
        intArrayOf(R.id.swipe_up, R.id.swipe_down, R.id.tap).forEach {
            findViewById<Button>(it).isEnabled = !active && !RemoteControlStatus.running
        }
        findViewById<Button>(R.id.cancel).isEnabled = service?.isWaiting == true
    }
}
