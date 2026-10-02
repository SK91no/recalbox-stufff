"""Unit tests for the autopair daemon, run against a faked dbus/gi world."""

import importlib.machinery
import importlib.util
import os
import sys
import tempfile
import types
import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock

AUTOPAIR_PATH = os.path.join(
    os.path.dirname(__file__), "..", "src", "scripts", "autopair"
)


class FakeDBusException(Exception):
    def get_dbus_name(self):
        return "org.bluez.Error.Fake"


class FakeProps:
    """org.freedesktop.DBus.Properties on a fake object."""

    def __init__(self, store):
        self.store = store

    def Get(self, iface, prop):
        try:
            return self.store[iface][prop]
        except KeyError as err:
            raise FakeDBusException(f"no {prop} on {iface}") from err

    def Set(self, iface, prop, value):
        self.store.setdefault(iface, {})[prop] = value


class FakeObject:
    """One object on the fake bus: properties plus one mock per interface."""

    def __init__(self, path):
        self.path = path
        self.props = {}
        self.ifaces = {}

    def iface(self, name):
        if name == "org.freedesktop.DBus.Properties":
            return FakeProps(self.props)
        return self.ifaces.setdefault(name, MagicMock(name=name))


class FakeBus:
    def __init__(self):
        self.objects = {}
        self.receivers = []
        self.bluez_present = True

    def get_object(self, service, path):
        return self.objects.setdefault(path, FakeObject(path))

    def add_signal_receiver(self, *args, **kwargs):
        self.receivers.append((args, kwargs))

    def name_has_owner(self, name):
        return self.bluez_present


FAKE_BUS = FakeBus()


def install_fake_modules():
    dbus = types.ModuleType("dbus")
    dbus.SystemBus = lambda: FAKE_BUS
    dbus.Interface = lambda obj, name: obj.iface(name)
    dbus.String = str
    dbus.Boolean = bool

    exceptions = types.ModuleType("dbus.exceptions")
    exceptions.DBusException = FakeDBusException
    dbus.exceptions = exceptions

    service = types.ModuleType("dbus.service")

    class Object:
        def __init__(self, bus, path):
            pass

    def method(_iface, in_signature="", out_signature=""):
        return lambda func: func

    service.Object = Object
    service.method = method
    dbus.service = service

    mainloop = types.ModuleType("dbus.mainloop")
    glib = types.ModuleType("dbus.mainloop.glib")
    glib.DBusGMainLoop = lambda set_as_default=False: None
    glib.threads_init = lambda: None
    mainloop.glib = glib
    dbus.mainloop = mainloop

    gi = types.ModuleType("gi")
    repository = types.ModuleType("gi.repository")
    GLib = MagicMock(name="GLib")
    repository.GLib = GLib
    gi.repository = repository

    for name, module in {
        "dbus": dbus,
        "dbus.exceptions": exceptions,
        "dbus.service": service,
        "dbus.mainloop": mainloop,
        "dbus.mainloop.glib": glib,
        "gi": gi,
        "gi.repository": repository,
    }.items():
        sys.modules[name] = module
    return GLib


GLIB = install_fake_modules()
loader = importlib.machinery.SourceFileLoader("autopair", AUTOPAIR_PATH)
spec = importlib.util.spec_from_loader("autopair", loader)
ap = importlib.util.module_from_spec(spec)
loader.exec_module(ap)


def make_device(path, **props):
    obj = FAKE_BUS.get_object("org.bluez", path)
    obj.props["org.bluez.Device1"] = dict(props)
    return obj


def make_adapter(path, **props):
    obj = FAKE_BUS.get_object("org.bluez", path)
    obj.props["org.bluez.Adapter1"] = dict(props)
    return obj


def make_manager(managed_objects=None):
    root = FAKE_BUS.get_object("org.bluez", "/")
    manager_iface = root.iface("org.freedesktop.DBus.ObjectManager")
    manager_iface.GetManagedObjects.return_value = managed_objects or {}
    return ap.BluetoothManager(FAKE_BUS)


class AutopairTest(unittest.TestCase):
    def setUp(self):
        FAKE_BUS.objects.clear()
        FAKE_BUS.receivers.clear()
        FAKE_BUS.bluez_present = True
        while not ap.message_queue.empty():
            ap.message_queue.get_nowait()

    def test_power_on_sets_the_powered_property_on_the_adapter_interface(self):
        obj = make_adapter("/org/bluez/hci0", Powered=False)
        ap.AdapterDevice("/org/bluez/hci0").power_on()
        self.assertIs(obj.props["org.bluez.Adapter1"]["Powered"], True)

    def test_a_speaker_that_refuses_the_connection_does_not_raise(self):
        obj = make_device("/dev_speaker", Name="JBL", Class=0x240414)
        obj.iface("org.bluez.Device1").Connect.side_effect = FakeDBusException("off")
        with self.assertLogs("autopair", level="WARNING"):
            ap.SpeakerDevice("/dev_speaker")

    def test_a_joystick_is_dropped_only_after_three_consecutive_failures(self):
        make_device("/dev_pad")
        joystick = ap.JoystickDevice("/dev_pad")
        self.assertFalse(joystick.should_remove())
        self.assertFalse(joystick.should_remove())
        joystick.connection_succeeded()
        self.assertFalse(joystick.should_remove())
        self.assertFalse(joystick.should_remove())
        self.assertTrue(joystick.should_remove())

    def test_a_busy_joystick_is_not_restarted_by_a_property_change(self):
        manager = make_manager()
        obj = make_device("/dev_pad", Name="Pad", Trusted=False, Paired=False)
        joystick = ap.JoystickDevice("/dev_pad")
        joystick.pending = "pairing"
        manager.connect_joystick_device_new(joystick)
        obj.iface("org.bluez.Device1").Pair.assert_not_called()

    def test_pairing_starts_for_an_untrusted_unpaired_joystick(self):
        manager = make_manager()
        obj = make_device("/dev_pad", Name="Pad", Trusted=False, Paired=False)
        joystick = ap.JoystickDevice("/dev_pad")
        manager.connect_joystick_device_new(joystick)
        self.assertIs(obj.props["org.bluez.Device1"]["Trusted"], True)
        self.assertEqual(joystick.pending, "pairing")
        _, kwargs = obj.iface("org.bluez.Device1").Pair.call_args
        self.assertIn("reply_handler", kwargs)
        self.assertIn("error_handler", kwargs)

    def test_repeated_failures_remove_the_joystick_even_if_bluez_refuses(self):
        manager = make_manager()
        make_device("/dev_pad", Name="Pad", Adapter="/org/bluez/hci0")
        adapter = make_adapter("/org/bluez/hci0")
        adapter.iface("org.bluez.Adapter1").RemoveDevice.side_effect = (
            FakeDBusException("busy")
        )
        joystick = ap.JoystickDevice("/dev_pad")
        manager.devices["/dev_pad"] = joystick
        manager.is_discovering = True
        for _ in range(ap.JoystickDevice.MAX_CONNECT_TRIES):
            manager.on_step_failed(joystick, "connecting", "timeout")
        self.assertNotIn("/dev_pad", manager.devices)

    def test_an_unpowered_adapter_is_powered_on_by_the_watchdog(self):
        manager = make_manager()
        obj = make_adapter("/org/bluez/hci0", Powered=False)
        manager.adapters["/org/bluez/hci0"] = ap.AdapterDevice("/org/bluez/hci0")
        self.assertTrue(manager.watchdog())
        self.assertIs(obj.props["org.bluez.Adapter1"]["Powered"], True)

    def _connected_pad_without_input_node(self, sysfs):
        manager = make_manager()
        obj = make_device(
            "/dev_pad", Name="Pad", Address="E4:17:D8:E7:1F:44",
            Connected=True, ServicesResolved=True,
        )
        joystick = ap.JoystickDevice("/dev_pad")
        manager.devices["/dev_pad"] = joystick
        self._original_sysfs = ap.INPUT_SYSFS_ROOT
        ap.INPUT_SYSFS_ROOT = sysfs
        self.addCleanup(lambda: setattr(ap, "INPUT_SYSFS_ROOT", self._original_sysfs))
        return manager, joystick, obj

    def test_a_connected_pad_without_input_device_gets_its_connection_cycled(self):
        with tempfile.TemporaryDirectory() as sysfs:
            manager, joystick, obj = self._connected_pad_without_input_node(sysfs)
            manager.hid_watchdog()
            obj.iface("org.bluez.Device1").Disconnect.assert_not_called()
            manager.hid_watchdog()
            obj.iface("org.bluez.Device1").Disconnect.assert_called_once()
            self.assertEqual(joystick.pending, "cycling")
            self.assertEqual(joystick.hid_cycles_left, ap.HID_CYCLE_BUDGET - 1)

    def test_a_pad_with_its_input_device_is_left_alone_by_the_hid_watchdog(self):
        with tempfile.TemporaryDirectory() as sysfs:
            manager, joystick, obj = self._connected_pad_without_input_node(sysfs)
            node = os.path.join(sysfs, "input7")
            os.mkdir(node)
            with open(os.path.join(node, "uniq"), "w") as uniq:
                uniq.write("e4:17:d8:e7:1f:44\n")
            joystick.hid_misses = 1
            joystick.hid_cycles_left = 0
            manager.hid_watchdog()
            obj.iface("org.bluez.Device1").Disconnect.assert_not_called()
            self.assertEqual(joystick.hid_misses, 0)
            self.assertEqual(joystick.hid_cycles_left, ap.HID_CYCLE_BUDGET)

    def test_the_cycle_budget_prevents_endless_reconnection_loops(self):
        with tempfile.TemporaryDirectory() as sysfs:
            manager, joystick, obj = self._connected_pad_without_input_node(sysfs)
            joystick.hid_cycles_left = 0
            manager.hid_watchdog()
            manager.hid_watchdog()
            manager.hid_watchdog()
            obj.iface("org.bluez.Device1").Disconnect.assert_not_called()

    def test_pairing_completion_connects_the_hid_profile_of_a_still_linked_pad(self):
        with tempfile.TemporaryDirectory() as sysfs:
            manager, joystick, obj = self._connected_pad_without_input_node(sysfs)
            manager.start_connection(joystick)
            obj.iface("org.bluez.Device1").Connect.assert_called_once()
            self.assertEqual(joystick.pending, "connecting")

    def test_a_paired_and_connected_pad_is_left_to_the_watchdog(self):
        manager = make_manager()
        obj = make_device(
            "/dev_pad", Name="Pad", Trusted=True, Paired=True, Connected=True,
        )
        joystick = ap.JoystickDevice("/dev_pad")
        manager.connect_joystick_device_new(joystick)
        obj.iface("org.bluez.Device1").Connect.assert_not_called()
        obj.iface("org.bluez.Device1").Pair.assert_not_called()

    def test_a_paired_but_disconnected_pad_gets_a_connection_attempt(self):
        manager = make_manager()
        obj = make_device(
            "/dev_pad", Name="Pad", Trusted=True, Paired=True, Connected=False,
        )
        joystick = ap.JoystickDevice("/dev_pad")
        manager.connect_joystick_device_new(joystick)
        obj.iface("org.bluez.Device1").Connect.assert_called_once()

    def test_an_in_progress_reply_burns_no_retry(self):
        make_device("/dev_pad", Name="Pad")
        manager = make_manager()
        joystick = ap.JoystickDevice("/dev_pad")

        class InProgress(FakeDBusException):
            def get_dbus_name(self):
                return "org.bluez.Error.InProgress"

        manager.on_connect_error(joystick, InProgress("busy"))
        self.assertEqual(joystick._connect_tries_left, ap.JoystickDevice.MAX_CONNECT_TRIES)
        self.assertIsNone(joystick.pending)

    def test_failures_outside_a_discovery_never_remove_a_paired_pad(self):
        manager = make_manager()
        make_device("/dev_pad", Name="Pad", Adapter="/org/bluez/hci0")
        make_adapter("/org/bluez/hci0")
        joystick = ap.JoystickDevice("/dev_pad")
        manager.devices["/dev_pad"] = joystick
        manager.is_discovering = False
        for _ in range(ap.JoystickDevice.MAX_CONNECT_TRIES * 2):
            manager.on_step_failed(joystick, "connecting", "asleep")
        self.assertIn("/dev_pad", manager.devices)

    def test_an_already_connected_reply_is_treated_as_a_success(self):
        make_device("/dev_pad", Name="Pad")
        manager = make_manager()
        joystick = ap.JoystickDevice("/dev_pad")
        joystick._connect_tries_left = 1

        class AlreadyConnected(FakeDBusException):
            def get_dbus_name(self):
                return "org.bluez.Error.AlreadyConnected"

        manager.on_connect_error(joystick, AlreadyConnected("already"))
        self.assertEqual(joystick._connect_tries_left, ap.JoystickDevice.MAX_CONNECT_TRIES)
        self.assertIsNone(joystick.pending)

    def test_a_failed_recovery_reconnection_is_not_counted_as_a_pairing_failure(self):
        make_device("/dev_pad", Name="Pad")
        manager = make_manager()
        joystick = ap.JoystickDevice("/dev_pad")
        tries_before = joystick._connect_tries_left
        with self.assertLogs("autopair", level="WARNING"):
            manager.on_cycle_reconnect_failed(joystick, "timeout")
        self.assertIsNone(joystick.pending)
        self.assertEqual(joystick._connect_tries_left, tries_before)

    def test_a_nameless_cached_device_does_not_kill_the_initial_scan(self):
        make_device("/dev_ghost", Class=0x240414)
        manager = make_manager(
            {"/dev_ghost": {"org.bluez.Device1": {}}}
        )
        self.assertIn("/dev_ghost", manager.devices)

    def test_a_device_advertising_a_joystick_appearance_is_recognized(self):
        make_device("/dev_le_joystick", Appearance=0x03C3)
        make_device("/dev_le_mouse", Appearance=0x03C2)
        self.assertTrue(ap.DeviceInterface("/dev_le_joystick").is_joystick())
        self.assertFalse(ap.DeviceInterface("/dev_le_mouse").is_joystick())

    def test_a_non_joystick_is_reported_with_its_class_and_appearance(self):
        manager = make_manager()
        make_device("/dev_headset", Name="Headset", Class=0x240404)
        with self.assertLogs("autopair", level="INFO") as logs:
            manager.on_interfaces_added_handler(
                "/dev_headset", {"org.bluez.Device1": {}}
            )
        self.assertNotIn("/dev_headset", manager.devices)
        self.assertTrue(any("class=0x240404" in line for line in logs.output))

    def test_a_crashing_signal_handler_is_contained_and_logged(self):
        def handler():
            raise ValueError("boom")

        with self.assertLogs("autopair", level="ERROR"):
            ap.logged_handler(handler)()

    def test_the_mqtt_manager_survives_without_paho(self):
        original = ap.MQTT_AVAILABLE
        ap.MQTT_AVAILABLE = False
        try:
            with self.assertLogs("autopair", level="ERROR"):
                mqtt_manager = ap.MqttManager()
            self.assertIsNone(mqtt_manager.mqtt_client)
            mqtt_manager.report_status({"devices": []})
        finally:
            ap.MQTT_AVAILABLE = original

    def test_a_malformed_mqtt_payload_is_logged_and_survived(self):
        original = ap.MQTT_AVAILABLE
        ap.MQTT_AVAILABLE = False
        try:
            mqtt_manager = ap.MqttManager()
            message = SimpleNamespace(payload=b"this is not json")
            with self.assertLogs("autopair", level="ERROR"):
                mqtt_manager.on_message(None, None, message)
        finally:
            ap.MQTT_AVAILABLE = original

    def test_the_discovery_countdown_runs_a_single_timer_chain(self):
        timers = []

        class FakeTimer:
            def __init__(self, interval, callback):
                self.cancelled = False
                timers.append(self)

            def start(self):
                pass

            def cancel(self):
                self.cancelled = True

        original_timer, original_available = ap.Timer, ap.MQTT_AVAILABLE
        ap.Timer, ap.MQTT_AVAILABLE = FakeTimer, False
        try:
            mqtt_manager = ap.MqttManager()
            mqtt_manager.restart_discovery_countdown()
            mqtt_manager.restart_discovery_countdown()
            self.assertEqual(len(timers), 2)
            self.assertTrue(timers[0].cancelled)
            self.assertFalse(timers[1].cancelled)
        finally:
            ap.Timer, ap.MQTT_AVAILABLE = original_timer, original_available

    def test_the_debug_flag_in_recalbox_conf_raises_the_log_level(self):
        original = ap.RECALBOX_CONF
        try:
            with tempfile.NamedTemporaryFile("w", suffix=".conf", delete=False) as conf:
                conf.write("some.key=value\ncontrollers.bluetooth.debug=1\n")
                ap.RECALBOX_CONF = conf.name
            self.assertTrue(ap.debug_logging_enabled())
            with open(ap.RECALBOX_CONF, "w") as conf:
                conf.write("controllers.bluetooth.debug=0\n")
            self.assertFalse(ap.debug_logging_enabled())
        finally:
            os.unlink(ap.RECALBOX_CONF)
            ap.RECALBOX_CONF = original

    def test_a_device_dump_serializes_every_flag_as_a_json_boolean(self):
        import json

        make_device(
            "/dev_pad", Name="Pad", Address="AA:BB", Paired=1, Trusted=0,
            Connected=1, Blocked=0, ServicesResolved=1,
        )
        dump = ap.JoystickDevice("/dev_pad").dump()
        serialized = json.loads(json.dumps(dump))
        for flag in ("paired", "trusted", "connected", "blocked", "services_resolved"):
            self.assertIsInstance(serialized[flag], bool, flag)

    def test_wait_for_bluez_gives_up_with_a_clear_error(self):
        FAKE_BUS.bluez_present = False
        with self.assertRaisesRegex(RuntimeError, "org.bluez"):
            ap.ThreadArbitror.wait_for_bluez(FAKE_BUS, timeout=0)


if __name__ == "__main__":
    unittest.main()
