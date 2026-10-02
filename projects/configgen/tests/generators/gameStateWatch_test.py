#!/usr/bin/env python
from configgen.remapper.GameStateWatch import NetCmd, classify, watch, MENU, RACE


def test_classify():
    assert classify(0x00, 0) == MENU
    assert classify(0x1E, 0) == RACE
    assert classify(None, 0) is None


def test_show_osd_sends_native_command():
    sent = []

    class FakeSock:
        def sendto(self, data, addr): sent.append((data, addr))

    nc = NetCmd.__new__(NetCmd)
    nc._addr = ("127.0.0.1", 55355)
    nc._sock = FakeSock()
    nc.show_osd("Recalbox JVS: switched controls to Race Mode")
    assert sent == [(b"SHOW_MSG Recalbox JVS: switched controls to Race Mode", ("127.0.0.1", 55355))]


def test_show_osd_empty_is_noop():
    class BoomSock:
        def sendto(self, *a): raise AssertionError("must not send")

    nc = NetCmd.__new__(NetCmd)
    nc._addr = ("127.0.0.1", 55355)
    nc._sock = BoomSock()
    nc.show_osd("")  # no send, no raise


def test_show_osd_swallows_socket_error():
    class DeadSock:
        def sendto(self, *a): raise OSError("closed")

    nc = NetCmd.__new__(NetCmd)
    nc._addr = ("127.0.0.1", 55355)
    nc._sock = DeadSock()
    nc.show_osd("hello")  # must not raise


def test_watch_applies_initial_then_transition():
    seq = iter([0x00, 0x00, 0x1E, 0x1E, 0x1E])
    changes = []
    watch(lambda: next(seq, 0x1E),
          lambda st, changed: changes.append(st) if changed else None,
          menu_value=0, poll_hz=1000.0, stable_frames=2,
          should_stop=lambda: len(changes) >= 2)
    assert changes == [MENU, RACE]


def test_watch_self_heals_current_state_without_reflagging():
    # RAM stays in menu; after the initial change, watch must periodically
    # re-apply (changed=False) so overwritten combos recover, without a re-flag.
    calls = []
    steps = [0]

    def stop():
        steps[0] += 1
        return steps[0] > 200  # ~200 frames, enough for a resync tick

    watch(lambda: 0x00, lambda st, changed: calls.append((st, changed)),
          menu_value=0, poll_hz=1000.0, stable_frames=2,
          resync_seconds=0.05, should_stop=stop)  # resync every 50 frames

    assert calls[0] == (MENU, True)                       # initial apply, flagged
    resyncs = [c for c in calls if c == (MENU, False)]
    assert resyncs, calls                                 # self-heal happened
    assert all(st == MENU for st, _ in calls)             # never spuriously RACE
