package com.thumbremote.app.ble

data class BleFrame(val sessionId: Long, val sequence: Long, val code: Int)
object BleEventProtocol {
    const val SERVICE = "50101af6-adf1-4884-b512-754d9f980ed0"
    const val EVENT = "d71a4ee9-9da8-4672-9e8f-e6478b2ca2b0"
    const val START = "b5fb20eb-73b5-4cfc-ba46-d787489c1b80"
    const val CCCD = "00002902-0000-1000-8000-00805f9b34fb"
    fun decode(bytes: ByteArray): BleFrame? {
        if (bytes.size != 10 || bytes[0].toInt() != 1) return null
        fun uint32(offset: Int): Long = (0..3).fold(0L) { value, i ->
            value or ((bytes[offset+i].toLong() and 255) shl (8*i))
        }
        val session = uint32(1)
        val sequence = uint32(5)
        val code = bytes[9].toInt() and 255
        if (session == 0L || code !in 0..5 || (code == 0 && sequence != 0L)) return null
        return BleFrame(session, sequence, code)
    }
    fun startSession(sessionId: Long): ByteArray {
        require(sessionId in 1..0xffffffffL)
        return ByteArray(5) { i -> if (i == 0) 1 else (sessionId shr (8*(i-1))).toByte() }
    }
}
