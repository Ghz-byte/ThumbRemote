package com.thumbremote.app

import org.junit.Assert.*
import org.junit.Test

class GestureSessionTest {
    @Test fun immediateRequestsHaveExclusionButNoCooldown() {
        val s = GestureSession()
        val immediate = s.armImmediate(GestureCommand.SWIPE_UP, 1000)!!
        assertEquals(1000L, immediate.dueAtMs)
        assertNull(s.armImmediate(GestureCommand.SWIPE_DOWN, 1001))
        assertFalse(s.arm(GestureCommand.TAP, 1001))
        assertTrue(s.finish(immediate.id, 1300))
        assertFalse(s.arm(GestureCommand.TAP, 1799))
        val second = s.armImmediate(GestureCommand.TAP, 1301)!!
        assertTrue(s.finish(second.id, 1400))
        assertTrue(s.arm(GestureCommand.TAP, 1900))
        assertNull(s.armImmediate(GestureCommand.SWIPE_UP, 7000))
    }
    @Test fun countdownDoesNotDispatchEarlyAndOnlyDispatchesOnce() {
        val session = GestureSession()
        assertTrue(session.arm(GestureCommand.SWIPE_UP, 1000))
        assertNull(session.takeDue(5999))
        val request = session.takeDue(6000)!!
        assertEquals(GestureCommand.SWIPE_UP, request.command)
        assertNull(session.takeDue(6001))
        assertTrue(session.isActive)
    }

    @Test fun repeatedClicksCannotReplaceWaitingOrRunningRequest() {
        val session = GestureSession()
        session.arm(GestureCommand.SWIPE_DOWN, 0)
        assertFalse(session.arm(GestureCommand.TAP, 1))
        assertEquals(GestureCommand.SWIPE_DOWN, session.takeDue(5000)!!.command)
        assertFalse(session.arm(GestureCommand.TAP, 5001))
    }

    @Test fun cancelledCountdownNeverDispatches() {
        val session = GestureSession()
        session.arm(GestureCommand.TAP, 0)
        assertTrue(session.cancel(200))
        assertNull(session.takeDue(6000))
        assertFalse(session.isActive)
    }

    @Test fun staleCallbackCannotFinishNewRequest() {
        val session = GestureSession()
        session.arm(GestureCommand.SWIPE_UP, 0)
        val old = session.takeDue(5000)!!
        session.cancel(5100)
        assertTrue(session.arm(GestureCommand.SWIPE_DOWN, 5600))
        val current = session.takeDue(10600)!!
        assertFalse(session.finish(old.id, 10700))
        assertTrue(session.isActive)
        assertTrue(session.finish(current.id, 10700))
        assertFalse(session.isActive)
    }

    @Test fun completionRequiresCurrentRunningRequestAndRespectsCooldown() {
        val session = GestureSession()
        session.arm(GestureCommand.TAP, 0)
        assertFalse(session.finish(1, 100))
        val request = session.takeDue(5000)!!
        assertTrue(session.finish(request.id, 5100))
        assertFalse(session.arm(GestureCommand.TAP, 5599))
        assertTrue(session.arm(GestureCommand.TAP, 5600))
    }

    @Test fun newSessionDoesNotReplayPreviousWork() {
        GestureSession().arm(GestureCommand.TAP, 0)
        assertNull(GestureSession().takeDue(99999))
    }

    @Test fun geometryUsesCorrectDirectionsAndCurrentScreenDimensions() {
        val up = GestureGeometry.plan(GestureCommand.SWIPE_UP, 1080, 2400)
        assertEquals(540f, up.startX)
        assertEquals(1800f, up.startY)
        assertEquals(600f, up.endY)
        assertEquals(300L, up.durationMs)
        val down = GestureGeometry.plan(GestureCommand.SWIPE_DOWN, 1080, 2400)
        assertEquals(600f, down.startY)
        assertEquals(1800f, down.endY)
        val tap = GestureGeometry.plan(GestureCommand.TAP, 2400, 1080)
        assertEquals(1200f, tap.startX)
        assertEquals(540f, tap.startY)
        assertEquals(tap.startY, tap.endY)
        assertEquals(80L, tap.durationMs)
    }
}
