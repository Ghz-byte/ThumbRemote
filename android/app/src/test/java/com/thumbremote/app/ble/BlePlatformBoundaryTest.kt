package com.thumbremote.app.ble

import org.junit.Assert.*
import org.junit.Test
class BlePlatformBoundaryTest {
    @Test fun permissionRaceReturnsFailureInsteadOfEscapingCallback() {
        assertFalse(BlePlatformBoundary.attempt { throw SecurityException("revoked between check and read") })
    }
    @Test fun closeStillRunsWhenDisconnectPermissionIsRevoked() {
        var closed=false
        assertFalse(BlePlatformBoundary.close({ throw SecurityException("disconnect denied") }, { closed=true }))
        assertTrue(closed)
    }
    @Test fun closePermissionFailureDoesNotEscapeLocalCleanup() {
        assertFalse(BlePlatformBoundary.close({}, { throw SecurityException("close denied") }))
    }
}
