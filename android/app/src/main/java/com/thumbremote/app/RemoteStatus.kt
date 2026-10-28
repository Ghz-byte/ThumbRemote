package com.thumbremote.app

/** Main-thread, process-local status. No screen content or persistent history. */
object RemoteStatus {
    var message: String? = null
        private set
    private val observers = mutableSetOf<() -> Unit>()

    fun publish(message: String) {
        this.message = message
        observers.toList().forEach { it() }
    }

    fun observe(observer: () -> Unit) { observers.add(observer) }
    fun stopObserving(observer: () -> Unit) { observers.remove(observer) }
}
