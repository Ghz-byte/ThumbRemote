package com.thumbremote.app

import org.junit.Assert.*
import org.junit.Test

class RemoteControlSessionTest {
    @Test fun executorRejectionCountsOnceAndDoesNotReplayOrAcceptLateCompletion() {
        val s = RemoteControlSession(); val g = s.begin(); s.ready(g)
        val request = s.accept(g, 1, 1, 0, 0)!!
        assertTrue(s.reject(g, request.id, 100, "执行器拒绝"))
        assertEquals(1, s.received)
        assertEquals(1, s.skipped)
        assertFalse(s.busy)
        assertEquals("执行器拒绝", s.records.single().result)
        assertFalse(s.reject(g, request.id, 101, "重复拒绝"))
        assertFalse(s.finish(g, request.id, 102, "迟到完成"))
        assertEquals(1, s.skipped)
        assertNull(s.accept(g, 1, 1, 600, 600))
        assertNotNull(s.accept(g, 2, 1, 101, 101))
    }

    @Test fun oldExecutorRejectionCannotCountOrCancelNewRequest() {
        val s = RemoteControlSession(); val old = s.begin(); s.ready(old)
        val previous = s.accept(old, 1, 1, 0, 0)!!
        s.stop()
        assertFalse(s.reject(old, previous.id, 100, "旧拒绝"))
        val g = s.begin(); s.ready(g)
        val current = s.accept(g, 1, 2, 1000, 1000)!!
        assertFalse(s.reject(old, previous.id, 1100, "旧拒绝"))
        assertEquals(0, s.skipped)
        assertTrue(s.busy)
        assertTrue(s.finish(g, current.id, 1300, "完成"))
        assertEquals(0, s.skipped)
    }

    @Test fun onlyFreshReadySessionCanExecuteVerticalInput() {
        val s = RemoteControlSession()
        val g = s.begin()
        assertNull(s.accept(g, 1, 1, 0, 0))
        assertTrue(s.ready(g))
        assertNull(s.accept(g - 1, 1, 1, 0, 0))
        assertNull(s.accept(g, 2, 3, 0, 0))
        assertNull(s.accept(g, 3, 4, 0, 0))
        val request = s.accept(g, 4, 1, 0, 0)!!
        assertEquals(GestureCommand.SWIPE_UP, request.command)
        assertTrue(s.finish(g, request.id, 300, "完成"))
        assertEquals(GestureCommand.SWIPE_DOWN, s.accept(g, 5, 2, 800, 800)!!.command)
    }

    @Test fun busyEventsQueueAndExecuteInOrderWithoutCooldown() {
        val s = RemoteControlSession(); val g = s.begin(); s.ready(g)
        val first = s.accept(g, 1, 5, 0, 0)!!
        assertEquals(GestureCommand.TAP, first.command)
        assertNull(s.accept(g, 2, 5, 40, 40))
        assertTrue(s.finish(g, first.id, 80, "完成"))
        val second = s.takeNext(g, 80)!!
        assertEquals(GestureCommand.TAP, second.command)
        assertEquals(2L, second.sequence)
        assertTrue(s.finish(g, second.id, 160, "完成"))
        assertNotNull(s.accept(g, 3, 2, 161, 161))
    }

    @Test fun queuedEventsExpireAndStopNeverReplays() {
        val s = RemoteControlSession(); val g = s.begin(); s.ready(g)
        val first = s.accept(g, 1, 1, 0, 0)!!
        assertNull(s.accept(g, 2, 5, 10, 10))
        assertTrue(s.finish(g, first.id, 511, "完成"))
        assertNull(s.takeNext(g, 511))
        val next = s.accept(g, 3, 5, 512, 512)!!
        assertNull(s.accept(g, 4, 5, 520, 520))
        s.stop()
        assertFalse(s.finish(g, next.id, 600, "完成"))
        assertNull(s.takeNext(g, 600))
    }

    @Test fun queueIsBoundedAndPreservesSequenceOrder() {
        val s = RemoteControlSession(); val g = s.begin(); s.ready(g)
        val first = s.accept(g, 1, 5, 0, 0)!!
        for (i in 2L..5L) assertNull(s.accept(g, i, 5, i, i))
        assertNull(s.accept(g, 6, 5, 6, 6))
        assertEquals(1, s.skipped)
        assertTrue(s.finish(g, first.id, 80, "完成"))
        for (i in 2L..5L) {
            val next = s.takeNext(g, 80 + i)!!
            assertEquals(i, next.sequence)
            assertTrue(s.finish(g, next.id, 80 + i, "完成"))
        }
        assertNull(s.takeNext(g, 90))
    }

    @Test fun staleArrivalsAreDroppedAtDispatchTime() {
        val s = RemoteControlSession(); val g = s.begin(); s.ready(g)
        assertNull(s.accept(g, 1, 1, 10, 511))
        assertNull(s.accept(g, 2, 1, 600, 599))
        assertNotNull(s.accept(g, 3, 1, 600, 1100))
    }

    @Test fun stopAndOldCompletionCannotReviveOrFinishNewSession() {
        val s = RemoteControlSession(); val old = s.begin(); s.ready(old)
        val request = s.accept(old, 1, 1, 0, 0)!!
        s.stop()
        assertFalse(s.ready(old))
        assertNull(s.accept(old, 2, 1, 0, 0))
        val g = s.begin(); s.ready(g)
        val current = s.accept(g, 1, 2, 1000, 1000)!!
        assertFalse(s.finish(old, request.id, 1200, "旧完成"))
        assertTrue(s.busy)
        assertTrue(s.finish(g, current.id, 1300, "完成"))
    }

    @Test fun recordsAreBoundedAndNewSessionDoesNotReplay() {
        val s = RemoteControlSession(); val g = s.begin(); s.ready(g)
        for (i in 1L..35L) assertNull(s.accept(g, i, 3, i, i))
        assertEquals(30, s.records.size)
        assertEquals(35, s.received)
        assertEquals(35, s.skipped)
        s.stop(); s.begin()
        assertEquals(0, s.received)
        assertFalse(s.busy)
    }
}
