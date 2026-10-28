package com.thumbremote.app.ble

import org.junit.Assert.*
import org.junit.Test
class BleReceiveSessionTest {
    private fun frame(sequence: Long=1, code: Int=1, session: Long=123): ByteArray =
        byteArrayOf(1,session.toByte(),(session shr 8).toByte(),(session shr 16).toByte(),(session shr 24).toByte(),
            sequence.toByte(),(sequence shr 8).toByte(),(sequence shr 16).toByte(),(sequence shr 24).toByte(),code.toByte())
    private fun starting(): BleReceiveSession = BleReceiveSession().apply {
        begin(1,123)
        advance(1,BlePhase.CONNECTING,BlePhase.DISCOVERING)
        advance(1,BlePhase.DISCOVERING,BlePhase.SUBSCRIBING)
        advance(1,BlePhase.SUBSCRIBING,BlePhase.STARTING)
    }
    private fun ready(): BleReceiveSession = starting().apply {
        writeSucceeded(1); receive(1,frame(0,0),0)
    }
    @Test fun bothHandshakeOrdersAndNoPrematureDirection() {
        for (readyFirst in listOf(true,false)) {
            val state=starting()
            assertEquals(BlePhase.STARTING,state.phase)
            assertNull(state.receive(1,frame(),1))
            if (readyFirst) state.receive(1,frame(0,0),2) else state.writeSucceeded(1)
            assertEquals(BlePhase.STARTING,state.phase)
            if (readyFirst) state.writeSucceeded(1) else state.receive(1,frame(0,0),2)
            assertEquals(BlePhase.READY,state.phase)
            assertNotNull(state.receive(1,frame(),3)); assertEquals(1,state.accepted)
        }
    }
    @Test fun staleCallbacksSessionAndStoppedGenerationAreIgnored() {
        val state=ready()
        assertNull(state.receive(0,frame(),0))
        assertNull(state.receive(1,frame(session=456),0))
        assertEquals(0,state.accepted)
        state.end(2)
        state.writeSucceeded(1); state.receive(1,frame(0,0),0)
        assertFalse(state.advance(1,BlePhase.IDLE,BlePhase.READY))
        assertEquals(BlePhase.IDLE,state.phase)
        assertNull(state.receive(1,frame(),0))
    }
    @Test fun duplicatesOldFramesGapsAndWrap() {
        val state=ready()
        assertNotNull(state.receive(1,frame(5),0)); assertEquals(4L,state.gaps)
        assertNull(state.receive(1,frame(5),0)); assertNull(state.receive(1,frame(4),0))
        assertNotNull(state.receive(1,frame(7),0)); assertEquals(5L,state.gaps)
        state.receive(1,frame(0,0),0)
        assertNull(state.receive(1,frame(7),0)); assertEquals(2,state.accepted)
        val wrap=ready()
        assertNotNull(wrap.receive(1,frame(0xffffffffL),0))
        assertNotNull(wrap.receive(1,frame(0),0)); assertEquals(2,wrap.accepted)
    }
    @Test fun logsBoundedAndNewSessionClearsHistory() {
        val state=ready()
        for (i in 1..31) state.receive(1,frame(i.toLong()),i.toLong())
        assertEquals(31,state.accepted); assertEquals(30,state.records.size)
        assertEquals(2L,state.records.first().frame.sequence)
        state.begin(2,456); assertTrue(state.records.isEmpty()); assertEquals(0,state.accepted)
        assertNull(state.receive(1,frame(32),32))
    }
    @Test fun invalidTransitionsAndTimeoutCannotReviveSession() {
        val state=BleReceiveSession(); state.begin(1,123)
        assertFalse(state.advance(1,BlePhase.CONNECTING,BlePhase.READY))
        state.end(2,true); assertEquals(BlePhase.ERROR,state.phase)
        assertFalse(state.advance(1,BlePhase.CONNECTING,BlePhase.DISCOVERING))
        assertNull(state.receive(1,frame(0,0),0))
    }
}
