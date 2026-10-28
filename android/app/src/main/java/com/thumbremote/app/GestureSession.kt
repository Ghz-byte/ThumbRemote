package com.thumbremote.app

enum class GestureCommand(val label: String) {
    SWIPE_UP("上滑"), SWIPE_DOWN("下滑"), TAP("中心单击")
}

data class PendingGesture(val id: Long, val command: GestureCommand, val dueAtMs: Long)
data class GesturePlan(val startX: Float, val startY: Float, val endX: Float, val endY: Float, val durationMs: Long)

class GestureSession {
    var waiting: PendingGesture? = null
        private set
    private var running: PendingGesture? = null
    private var nextId = 1L
    private var lastFinishedAt: Long? = null
    val isActive: Boolean get() = waiting != null || running != null

    fun armImmediate(command: GestureCommand, now: Long): PendingGesture? {
        if (isActive) return null
        return PendingGesture(nextId++, command, now).also { running = it }
    }

    fun arm(command: GestureCommand, now: Long): Boolean {
        if (isActive || lastFinishedAt?.let { now - it < 500 } == true) return false
        waiting = PendingGesture(nextId++, command, now + 5000)
        return true
    }

    fun takeDue(now: Long): PendingGesture? {
        val request = waiting ?: return null
        if (now < request.dueAtMs) return null
        waiting = null
        running = request
        return request
    }

    fun finish(id: Long, now: Long): Boolean {
        if (running?.id != id) return false
        running = null
        lastFinishedAt = now
        return true
    }

    fun cancel(now: Long): Boolean {
        if (!isActive) return false
        waiting = null
        running = null
        lastFinishedAt = now
        return true
    }
}

object GestureGeometry {
    fun plan(command: GestureCommand, width: Int, height: Int): GesturePlan {
        require(width > 0 && height > 0) { "Display dimensions must be positive" }
        val x = width * 0.5f
        return when (command) {
            GestureCommand.SWIPE_UP -> GesturePlan(x, height * 0.75f, x, height * 0.25f, 300)
            GestureCommand.SWIPE_DOWN -> GesturePlan(x, height * 0.25f, x, height * 0.75f, 300)
            GestureCommand.TAP -> GesturePlan(x, height * 0.5f, x, height * 0.5f, 80)
        }
    }
}
