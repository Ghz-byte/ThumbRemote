package com.thumbremote.app

/** Main-thread process state only. No retained Activity or executable history. */
object RemoteControlStatus {
    var running = false; private set
    var message = "遥控未开启"; private set
    var received = 0; private set
    var skipped = 0; private set
    var records: List<RemoteRecord> = emptyList(); private set
    private var ticket = 0L
    private val observers = mutableSetOf<() -> Unit>()
    fun reserve(): Long? {
        if (running) return null
        ticket++; running = true; message = "正在启动遥控，请稍候"
        received = 0; skipped = 0; records = emptyList(); notifyChanged()
        return ticket
    }
    fun isCurrent(ticket: Long): Boolean = running && ticket == this.ticket
    fun end(message: String) {
        running = false; ticket++; this.message = message; notifyChanged()
    }
    fun update(message: String, session: RemoteControlSession) {
        this.message = message; received = session.received; skipped = session.skipped
        records = session.records; notifyChanged()
    }
    private fun notifyChanged() { observers.toList().forEach { it() } }
    fun observe(observer: () -> Unit) { observers.add(observer) }
    fun stopObserving(observer: () -> Unit) { observers.remove(observer) }
}
