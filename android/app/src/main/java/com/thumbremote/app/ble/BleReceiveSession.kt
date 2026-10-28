package com.thumbremote.app.ble

enum class BlePhase { IDLE, CONNECTING, DISCOVERING, SUBSCRIBING, STARTING, READY, ERROR }
data class ReceivedEvent(val frame: BleFrame, val receivedAt: Long)
class BleReceiveSession {
    var generation = 0L; private set
    var sessionId = 0L; private set
    var phase = BlePhase.IDLE; private set
    var accepted = 0; private set
    var dropped = 0; private set
    var gaps = 0L; private set
    private val history = ArrayDeque<ReceivedEvent>()
    val records: List<ReceivedEvent> get() = history.toList()
    private var writeAck = false
    private var readyAck = false
    private var lastSequence: Long? = null
    fun clearRecords() { history.clear() }
    fun begin(generation: Long, sessionId: Long) {
        require(sessionId in 1..0xffffffffL)
        end(generation)
        this.sessionId=sessionId
        phase=BlePhase.CONNECTING
    }
    fun advance(generation: Long, expected: BlePhase, next: BlePhase): Boolean {
        val allowed = when (expected) {
            BlePhase.CONNECTING -> BlePhase.DISCOVERING
            BlePhase.DISCOVERING -> BlePhase.SUBSCRIBING
            BlePhase.SUBSCRIBING -> BlePhase.STARTING
            else -> null
        }
        if (generation != this.generation || phase != expected || next != allowed) return false
        phase=next; return true
    }
    private fun finishHandshake() {
        if (phase == BlePhase.STARTING && writeAck && readyAck) phase=BlePhase.READY
    }
    fun writeSucceeded(generation: Long) {
        if (generation != this.generation || phase != BlePhase.STARTING) return
        writeAck=true; finishHandshake()
    }
    fun receive(generation: Long, bytes: ByteArray, now: Long): BleFrame? {
        if (generation != this.generation) return null
        val frame=BleEventProtocol.decode(bytes)
        if (frame == null || frame.sessionId != sessionId) { dropped++; return null }
        if (frame.code == 0) {
            if (phase == BlePhase.STARTING) { readyAck=true; finishHandshake() }
            return null
        }
        if (phase != BlePhase.READY) { dropped++; return null }
        val previous=lastSequence
        if (previous != null) {
            val delta=(frame.sequence-previous) and 0xffffffffL
            if (delta == 0L || delta > 0x7fffffffL) { dropped++; return null }
            gaps += delta-1
        } else if (frame.sequence > 1) gaps += frame.sequence-1
        lastSequence=frame.sequence
        accepted++
        history.addLast(ReceivedEvent(frame,now))
        if (history.size > 30) history.removeFirst()
        return frame
    }
    fun end(generation: Long, failed: Boolean = false) {
        this.generation=generation; sessionId=0; phase=if (failed) BlePhase.ERROR else BlePhase.IDLE
        writeAck=false; readyAck=false; lastSequence=null
        accepted=0; dropped=0; gaps=0; history.clear()
    }
}
