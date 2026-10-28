package com.thumbremote.app.ble

/** Permission checks and platform calls can race; failures must not escape callbacks. */
internal object BlePlatformBoundary {
    fun attempt(action: () -> Unit): Boolean = try {
        action(); true
    } catch (_: SecurityException) {
        false
    } catch (_: IllegalStateException) {
        false
    }
    fun close(disconnect: () -> Unit, close: () -> Unit): Boolean {
        val disconnected = attempt(disconnect)
        val closed = attempt(close)
        return disconnected && closed
    }
}
