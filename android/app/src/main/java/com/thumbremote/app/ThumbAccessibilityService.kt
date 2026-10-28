package com.thumbremote.app

import android.accessibilityservice.AccessibilityService
import android.accessibilityservice.GestureDescription
import android.app.KeyguardManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.graphics.Path
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.PowerManager
import android.os.SystemClock
import android.util.DisplayMetrics
import android.view.WindowManager
import android.view.accessibility.AccessibilityEvent
import kotlin.math.ceil

class ThumbAccessibilityService : AccessibilityService() {
    companion object {
        var current: ThumbAccessibilityService? = null
            private set
    }

    private val handler = Handler(Looper.getMainLooper())
    private val session = GestureSession()
    private var receiverRegistered = false
    private var remoteCompletion: Pair<Long, (String) -> Unit>? = null
    val isActive: Boolean get() = session.isActive
    val isWaiting: Boolean get() = session.waiting != null
    private val screenReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action == Intent.ACTION_SCREEN_OFF) {
                RemoteControlService.current?.stopRemote("屏幕已关闭，遥控结束")
                if (session.cancel(now())) {
                    handler.removeCallbacksAndMessages(null)
                    RemoteStatus.publish(getString(R.string.locked))
                }
            }
        }
    }

    override fun onServiceConnected() {
        super.onServiceConnected()
        current = this
        if (!receiverRegistered) {
            val filter = IntentFilter(Intent.ACTION_SCREEN_OFF)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                registerReceiver(screenReceiver, filter, Context.RECEIVER_NOT_EXPORTED)
            } else {
                registerReceiver(screenReceiver, filter)
            }
            receiverRegistered = true
        }
        RemoteStatus.publish(getString(R.string.connected))
    }

    fun arm(command: GestureCommand) {
        if (RemoteControlStatus.running) {
            RemoteStatus.publish("请先停止遥控，再使用手动测试")
            return
        }
        if (!screenReady()) {
            RemoteStatus.publish(getString(R.string.locked))
            return
        }
        if (!session.arm(command, now())) {
            RemoteStatus.publish(getString(if (isActive) R.string.busy else R.string.cooldown))
            return
        }
        tick()
    }

    private fun tick() {
        val pending = session.waiting ?: return
        if (!screenReady()) {
            session.cancel(now())
            RemoteStatus.publish(getString(R.string.locked))
            return
        }
        val remaining = pending.dueAtMs - now()
        if (remaining > 0) {
            RemoteStatus.publish(getString(R.string.countdown_format, pending.command.label,
                ceil(remaining / 1000.0).toInt()))
            handler.postDelayed({ tick() }, minOf(remaining, 250L))
            return
        }
        val request = session.takeDue(now()) ?: return
        execute(request)
    }

    fun executeRemote(command: GestureCommand, completed: (String) -> Unit): Boolean {
        if (!screenReady()) return false
        val request = session.armImmediate(command, now()) ?: return false
        remoteCompletion = request.id to completed
        execute(request)
        return true
    }

    fun cancelRemote() {
        if (remoteCompletion != null) {
            remoteCompletion = null
            session.cancel(now())
            handler.removeCallbacksAndMessages(null)
        }
    }

    private fun execute(request: PendingGesture) {
        RemoteStatus.publish(getString(R.string.executing_format, request.command.label))
        try {
            val (width, height) = screenSize()
            val plan = GestureGeometry.plan(request.command, width, height)
            val path = Path().apply {
                moveTo(plan.startX, plan.startY)
                lineTo(plan.endX, plan.endY)
            }
            val gesture = GestureDescription.Builder()
                .addStroke(GestureDescription.StrokeDescription(path, 0, plan.durationMs)).build()
            val accepted = dispatchGesture(gesture, object : GestureResultCallback() {
                override fun onCompleted(gestureDescription: GestureDescription?) {
                    complete(request, R.string.completed)
                }
                override fun onCancelled(gestureDescription: GestureDescription?) {
                    complete(request, R.string.gesture_cancelled)
                }
            }, handler)
            if (!accepted) {
                complete(request, R.string.gesture_rejected)
            } else {
                handler.postDelayed({ complete(request, R.string.gesture_timeout) }, 3000)
            }
        } catch (_: RuntimeException) {
            complete(request, R.string.gesture_error)
        }
    }

    private fun complete(request: PendingGesture, messageId: Int) {
        if (session.finish(request.id, now())) {
            handler.removeCallbacksAndMessages(null)
            val completion = remoteCompletion?.takeIf { it.first == request.id }?.second
            remoteCompletion = null
            RemoteStatus.publish(getString(R.string.result_format, request.command.label, getString(messageId)))
            completion?.invoke(getString(messageId))
        }
    }

    fun cancelByUser() {
        if (isWaiting && session.cancel(now())) {
            handler.removeCallbacksAndMessages(null)
            RemoteStatus.publish(getString(R.string.cancelled))
        }
    }

    private fun screenReady(): Boolean =
        getSystemService(PowerManager::class.java).isInteractive &&
            !getSystemService(KeyguardManager::class.java).isKeyguardLocked

    @Suppress("DEPRECATION")
    private fun screenSize(): Pair<Int, Int> {
        val manager = getSystemService(WindowManager::class.java)
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            val bounds = manager.maximumWindowMetrics.bounds
            bounds.width() to bounds.height()
        } else {
            val metrics = DisplayMetrics()
            manager.defaultDisplay.getRealMetrics(metrics)
            metrics.widthPixels to metrics.heightPixels
        }
    }

    override fun onAccessibilityEvent(event: AccessibilityEvent?) = Unit
    override fun onInterrupt() {
        RemoteControlService.current?.stopRemote("辅助功能中断，遥控结束")
        // An interruption stops feedback/work, but does not unbind the service.
        if (session.cancel(now())) {
            handler.removeCallbacksAndMessages(null)
            RemoteStatus.publish(getString(R.string.gesture_cancelled))
        }
    }
    override fun onUnbind(intent: Intent?): Boolean {
        disconnect()
        return super.onUnbind(intent)
    }
    override fun onDestroy() {
        disconnect()
        if (receiverRegistered) {
            unregisterReceiver(screenReceiver)
            receiverRegistered = false
        }
        super.onDestroy()
    }

    private fun disconnect() {
        RemoteControlService.current?.stopRemote("辅助功能已断开，遥控结束")
        remoteCompletion = null
        handler.removeCallbacksAndMessages(null)
        session.cancel(now())
        if (current === this) {
            current = null
            RemoteStatus.publish(getString(R.string.service_lost))
        }
    }

    private fun now(): Long = SystemClock.elapsedRealtime()
}
