package com.thumbremote.app

data class RemoteRequest(val generation: Long, val id: Long, val command: GestureCommand, val sequence: Long)
data class RemoteRecord(val id: Long, val code: Int, val sequence: Long, val result: String)

data class RemoteStartConditions(val notifications: Boolean, val nearby: Boolean, val accessibility: Boolean,
    val unlocked: Boolean, val manualBusy: Boolean) {
    fun failure(): String? = when {
        !notifications -> "请允许通知并开启遥控通知通道，再主动开始遥控"
        !nearby -> "请允许附近设备权限，再主动开始遥控"
        !accessibility -> "请先开启并连接辅助功能服务"
        !unlocked -> "屏幕关闭或锁定，不能开启遥控"
        manualBusy -> "已有手动任务，请先取消或等待完成"
        else -> null
    }
}

class RemoteControlSession {
    var generation = 0L; private set
    var active = false; private set
    var connected = false; private set
    var received = 0; private set
    var skipped = 0; private set
    private var running: RemoteRequest? = null
    private val pending = ArrayDeque<Pair<RemoteRequest, Long>>()
    private var lastSequence: Long? = null
    private var nextId = 1L
    private val history = mutableListOf<RemoteRecord>()
    val busy: Boolean get() = running != null
    val records: List<RemoteRecord> get() = history.toList()
    fun begin(): Long {
        stop(); active = true; received = 0; skipped = 0; history.clear()
        lastSequence = null; pending.clear()
        return generation
    }
    fun ready(generation: Long): Boolean {
        if (!active || generation != this.generation) return false
        connected = true; return true
    }
    fun accept(generation: Long, sequence: Long, code: Int, receivedAt: Long, now: Long): RemoteRequest? {
        if (!active || !connected || generation != this.generation) return null
        val previous = lastSequence
        if (previous != null && ((sequence - previous) and 0xffffffffL).let { it == 0L || it > 0x7fffffffL }) return null
        lastSequence = sequence
        received++
        val reason = when {
            code !in listOf(1, 2, 5) -> "当前不执行此方向"
            now < receivedAt || now - receivedAt > 500 -> "消息已过期，不执行"
            busy && pending.size >= 4 -> "操作过快，队列已满"
            else -> null
        }
        val id = nextId++
        history.add(RemoteRecord(id, code, sequence, reason ?: "正在执行"))
        if (history.size > 30) history.removeAt(0)
        if (reason != null) { skipped++; return null }
        val command = when (code) { 1 -> GestureCommand.SWIPE_UP; 2 -> GestureCommand.SWIPE_DOWN; else -> GestureCommand.TAP }
        val request = RemoteRequest(generation, id, command, sequence)
        if (busy) {
            pending.addLast(request to receivedAt)
            val index = history.indexOfFirst { it.id == id }
            if (index >= 0) history[index] = history[index].copy(result = "等待前一个手势")
            return null
        }
        running = request
        return request
    }

    fun takeNext(generation: Long, now: Long): RemoteRequest? {
        if (!active || !connected || generation != this.generation || running != null) return null
        while (pending.isNotEmpty()) {
            val (request, receivedAt) = pending.removeFirst()
            val index = history.indexOfFirst { it.id == request.id }
            if (now < receivedAt || now - receivedAt > 500) {
                skipped++
                if (index >= 0) history[index] = history[index].copy(result = "消息已过期，不执行")
                continue
            }
            if (index >= 0) history[index] = history[index].copy(result = "正在执行")
            running = request
            return request
        }
        return null
    }
    fun finish(generation: Long, id: Long, now: Long, result: String): Boolean {
        if (!active || generation != this.generation || running?.id != id) return false
        running = null
        val index = history.indexOfFirst { it.id == id }
        if (index >= 0) history[index] = history[index].copy(result = result)
        return true
    }
    /** The executor declined before dispatch; unlike a system result, this is a skipped input. */
    fun reject(generation: Long, id: Long, now: Long, result: String): Boolean {
        if (!finish(generation, id, now, result)) return false
        skipped++
        return true
    }

    fun stop() {
        running?.let { request ->
            val index = history.indexOfFirst { it.id == request.id }
            if (index >= 0) history[index] = history[index].copy(result = "会话已停止，未等待系统结果")
        }
        pending.forEach { (request, _) ->
            val index = history.indexOfFirst { it.id == request.id }
            if (index >= 0) history[index] = history[index].copy(result = "会话已停止，未执行")
        }
        pending.clear()
        generation++; active = false; connected = false; running = null
    }
}
