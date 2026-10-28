package com.thumbremote.app

import org.junit.Assert.*
import org.junit.Test

class RemoteControlStartTest {
    @Test fun everyRequiredConditionPreventsStartingWhenMissing() {
        val ready = RemoteStartConditions(true, true, true, true, false)
        assertNull(ready.failure())
        assertNotNull(ready.copy(notifications = false).failure())
        assertNotNull(ready.copy(nearby = false).failure())
        assertNotNull(ready.copy(accessibility = false).failure())
        assertNotNull(ready.copy(unlocked = false).failure())
        assertNotNull(ready.copy(manualBusy = true).failure())
    }

    @Test fun doubleStartIsRejectedAndOldStartTicketCannotReviveStoppedSession() {
        RemoteControlStatus.end("结束")
        val old = RemoteControlStatus.reserve()!!
        assertNull(RemoteControlStatus.reserve())
        RemoteControlStatus.end("停止")
        assertFalse(RemoteControlStatus.isCurrent(old))
        val next = RemoteControlStatus.reserve()!!
        assertFalse(RemoteControlStatus.isCurrent(old))
        assertTrue(RemoteControlStatus.isCurrent(next))
        RemoteControlStatus.end("结束")
    }
}
