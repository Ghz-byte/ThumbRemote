package com.thumbremote.app.ble

import org.junit.Assert.*
import org.junit.Test
class BleEventProtocolTest {
    @Test fun literalFixtureAndControl() {
        val bytes = byteArrayOf(1,0x78,0x56,0x34,0x12,1,0,0,0,1)
        assertEquals(BleFrame(0x12345678,1,1), BleEventProtocol.decode(bytes))
        assertArrayEquals(bytes.copyOfRange(0,5), BleEventProtocol.startSession(0x12345678))
    }
    @Test fun rejectsMalformedFrames() {
        val good = byteArrayOf(1,1,0,0,0,1,0,0,0,4)
        assertNotNull(BleEventProtocol.decode(good))
        assertNull(BleEventProtocol.decode(good.copyOf(9)))
        assertNull(BleEventProtocol.decode(good.copyOf(11)))
        for ((index,value) in listOf(0 to 2, 1 to 0, 9 to 6)) {
            val bad=good.copyOf(); bad[index]=value.toByte(); assertNull(BleEventProtocol.decode(bad))
        }
        val ready=good.copyOf(); ready[9]=0
        assertNull(BleEventProtocol.decode(ready))
        ready[5]=0; assertEquals(0, BleEventProtocol.decode(ready)?.code)
    }
    @Test fun acceptsTapEvent() {
        assertEquals(5, BleEventProtocol.decode(byteArrayOf(1,1,0,0,0,1,0,0,0,5))?.code)
    }
}
