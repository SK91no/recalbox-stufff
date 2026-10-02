import os
import sys
import configparser
import shutil
import subprocess
from configgen.utils.architecture import Architecture
from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.generators.Generator import Generator
from configgen.generators.vpinball.vpinballConfig import VpinballConfig
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.utils.recallog import recallog
from configgen.utils.Rotation import Rotation
from configgen.generators.vpinball.vpinballOptions import (
    DEFAULT_INI,
    default_player,
    default_pinmame,
    default_editor,
    profile_perf,
)


class VpinballGenerator(Generator):

    def _read_ini(self, path):
        settings = configparser.ConfigParser(
            strict=False, allow_no_value=True, empty_lines_in_values=False
        )
        settings.optionxform = str
        settings.read(path)
        return settings

    def _dmd_size(self, xsize):
        # A DMD is 128x32 dots; xsize scales it. Kept integer: VPX parses these
        # as ints, and the menu offers a fractional scale (x1.5).
        return int(128 * xsize), int(32 * xsize)

    def _dmd_x(self, scr1_width, scr2_width, dmd_width):
        return int(scr1_width) + (int(scr2_width) // 2) - (dmd_width // 2)

    def _set(self, settings, section, key, value):
        if not settings.has_section(section):
            settings.add_section(section)
        settings.set(section, key, value)

    def _load(self, settings, section, pairs):
        if not settings.has_section(section):
            settings.add_section(section)
        for key, value in pairs.items():
            settings.set(section, key, value)

    def _get_monitors(self):
        lines = subprocess.check_output(["xrandr"]).decode("utf-8").splitlines()
        return [l.split()[0] for l in lines if " connected " in l]

    @staticmethod
    def _vpx_code(item):
        # VPX listens to *raw* SDL joystick events, so the mapping codes are raw
        # indices: buttons keep their index, axes are 0x0200|axis, hats are
        # 0x0100|(hat<<2)|direction (left/right/up/down).
        if item.IsAxis:
            return 0x0200 | item.Id
        if item.IsHat:
            direction = {8: 0, 2: 1, 1: 2, 4: 3}.get(item.Value, 0)  # L,R,U,D
            return 0x0100 | (item.Id << 2) | direction
        return item.Id

    # VPX action -> (keyboard scancode fallback, list of recalbox buttons that
    # each trigger it [OR]). Button names are resolved via Has<Name>/<Name> on the
    # Controller. Keyboard scancodes mirror VPX's stock Default_VPinballX.ini.
    _INPUT_MAP = [
        ("LeftFlipper",  225, ["L1"]),       # left flipper  -> L1
        ("RightFlipper", 229, ["R1"]),       # right flipper -> R1
        ("LaunchBall",    40, ["R2"]),       # plunger/launch-> R2
        ("Lockbar",      226, ["B"]),        # action (fire) -> B
        ("LeftMagna",    224, ["Y"]),        # left magnasave-> Y
        ("RightMagna",   228, ["A"]),        # right magnasave->A
        ("CenterNudge",   44, ["Up"]),       # center tilt   -> Up
        ("LeftNudge",     29, ["Left"]),     # left tilt     -> Left
        ("RightNudge",    56, ["Right"]),    # right tilt    -> Right
        ("Credit1",       34, ["Select"]),   # credits       -> Select
        ("Start",         30, ["Start"]),    # start / menu validate -> Start
        ("Pause",         19, ["X"]),        # pause         -> X
        ("CoinDoor",      77, ["L2"]),       # toggle service-> L2
        ("Service1",      36, ["L3"]),       # service menu  -> L3
        # LiveUI / table-options navigation. Stock VPX scheme: magnasave moves the
        # selection, the flippers enter/adjust (right flipper = validate/enter,
        # left flipper = decrease/into). We also accept the D-pad. These actions
        # are menu-only, so they coexist with the in-game flipper/magna/nudge
        # bindings above. (UILeft/UIRight only wake after a first UIUp/UIDown.)
        ("UIUp",         224, ["Up",    "Y"]),    # menu up    -> D-pad Up / L.magna
        ("UIDown",       228, ["Down",  "A"]),    # menu down  -> D-pad Down / R.magna
        ("UILeft",       225, ["Left",  "L1"]),   # menu left  -> D-pad Left / L.flip
        ("UIRight",      229, ["Right", "R1"]),   # menu enter -> D-pad Right / R.flip
    ]

    # VPX action -> (keyboard scancode fallback, button names held together).
    # The joystick binding fires only when ALL named buttons are pressed (&).
    _INPUT_COMBOS = [
        ("ExitGame", 41, ["Hotkey", "Start"]),  # quit / menu back -> hotkey + start
        ("InGameUI", 69, ["Hotkey", "B"]),      # table options / menu back -> hotkey + B
    ]

    def _configure_input(self, settings, controllers):
        # Build a deterministic [Input] mapping from the real button/axis indices
        # of the connected pad. VPX uses raw SDL joystick codes (not GameController
        # codes), so we must emit the actual indices; relying on VPX auto-layout
        # gives a shifted, broken mapping on non-Xbox pads.
        controller = next(iter(controllers.values()), None)
        if controller is None:
            return
        device = "SDLJoy_{}_1".format(controller.GUID)

        def code(name):
            if getattr(controller, "Has" + name, False):
                return self._vpx_code(getattr(controller, name))
            return None

        def binding(key, names, combo=False):
            # keyboard fallback, then the joystick binding(s)
            value = "Key;{}".format(key)
            codes = [code(n) for n in names]
            if combo:
                # AND: a single binding that fires when every button is held
                if all(c is not None for c in codes):
                    value += " | " + " & ".join(
                        "{};{}".format(device, c) for c in codes)
            else:
                # OR: one alternative binding per available button
                for c in codes:
                    if c is not None:
                        value += " | {};{}".format(device, c)
            return value

        for action, key, names in self._INPUT_MAP:
            self._set(settings, "Input", "Mapping." + action, binding(key, names))
        for action, key, names in self._INPUT_COMBOS:
            self._set(settings, "Input", "Mapping." + action, binding(key, names, combo=True))

        # We supply the mapping ourselves, so suppress VPX's "apply device
        # layout" prompt for this pad.
        self._set(settings, "Input", "Device.{}.NoAutoLayout".format(device), "1")

    def _disable_input(self, settings, controllers):
        # Keyboard-only bindings + suppress auto-layout, so the pad is inert inside
        # VPX and the user can route controls through a pad-to-keyboard (.p2k.cfg).
        for action, key, _name in self._INPUT_MAP:
            self._set(settings, "Input", "Mapping." + action, "Key;{}".format(key))
        for action, key, _names in self._INPUT_COMBOS:
            self._set(settings, "Input", "Mapping." + action, "Key;{}".format(key))
        controller = next(iter(controllers.values()), None)
        if controller is not None:
            device = "SDLJoy_{}_1".format(controller.GUID)
            self._set(settings, "Input", "Device.{}.NoAutoLayout".format(device), "1")

    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:
        recalconf = keyValueSettings(recalboxFiles.recalboxConf)
        recalconf.loadFile(True)

        os.makedirs(recalboxFiles.vpinballRootFolder, exist_ok=True)

        config_file = recalboxFiles.vpinballConfigFile
        custom_ini = os.path.join(recalboxFiles.vpinballRootFolder, "VPinballX-custom.ini")

        if not os.path.exists(config_file):
            shutil.copy(DEFAULT_INI, config_file)

        if os.path.exists(custom_ini):
            recallog("*** Using Custom VPinballX.ini file ***")
            config_file = custom_ini
        else:
            try:
                vpinballSettings = self._read_ini(config_file)
            except configparser.DuplicateOptionError:
                recallog("*** Resetting to default VPinballX.ini file ***")
                shutil.copy(DEFAULT_INI, config_file)
                vpinballSettings = self._read_ini(config_file)

            ballTrail  = recalconf.getBool("pinball.ball_trail", 0)
            # Scales the 128x32 DMD, so this is an absolute dot size, not a ratio
            # of the screen: x8 gives a 1024px wide panel, which eats 80% of a
            # 1280 wide screen and 95% of a 1080 portrait one. x2 (256px) keeps
            # the panel to roughly a quarter of the width on the usual screens.
            # float: the menu offers a fractional scale (x1.5)
            b2sSize    = float(recalconf.getString("pinball.score_size", "2"))
            screen1    = recalconf.getString("pinball.screen1", "None")
            screen2    = recalconf.getString("pinball.screen2", "None")
            # No explicit choice: default to the Pi profile on a Pi 5, else perf.
            profile    = recalconf.getString("pinball.profile", "")
            if not profile:
                profile = "4" if Architecture().isPi5 else "1"
            autoConf   = recalconf.getBool("pinball.autoconf", "0")
            # 0 = generator manages the pad, 1 = user manages it via VPX LiveUI,
            # 2 = pad disabled in VPX (use a .p2k.cfg instead)
            customInput = recalconf.getString("pinball.custominput", "0")

            if Architecture().isX64 and len(self._get_monitors()) >= 2:
                cabinetMode = recalconf.getBool("pinball.cabinet", 0)
            else:
                cabinetMode = 0

            self._load(vpinballSettings, "Player", default_player)
            self._load(vpinballSettings, "Plugin.PinMAME", default_pinmame)
            self._load(vpinballSettings, "Editor", default_editor)
            self._load(vpinballSettings, "Player", profile_perf[profile])

            # Mirror recalbox's global "show FPS" toggle (0=off, 1=FPS, 2=Full).
            self._set(vpinballSettings, "Player", "ShowFPS", "1" if system.ShowFPS else "0")

            # Mirror recalbox's "rumble" toggle (0=Off, 3=Table with generic fallback).
            self._set(vpinballSettings, "Player", "RumbleMode", "3" if system.Rumble else "0")

            # Texture preload (0=Disabled, 1=Preload Textures): decodes the table
            # images on the loader thread pool, which shortens startup on the Pi5.
            # Needs the 006 patch: without it VPX deadlocks on the first cached
            # texture, and once that is lifted the whole table renders untextured.
            self._set(vpinballSettings, "Player", "CacheMode", "1")

            # Frame pacing (the default) cannot create its sync device on the
            # Pi5 GLES driver: VPX pops an error dialog, then falls back to
            # adaptive sync on its own. Ask for adaptive (2) upfront instead.
            # Empty elsewhere = keep VPX's default.
            self._set(vpinballSettings, "Player", "SyncMode",
                      "2" if Architecture().isPi5 else "")

            if customInput == "1":
                pass  # user manages the pad himself via VPX LiveUI; leave [Input] alone
            elif customInput == "2":
                self._disable_input(vpinballSettings, playersControllers)  # pad off, use p2k
            else:
                self._configure_input(vpinballSettings, playersControllers)

            if ballTrail:
                self._set(vpinballSettings, "Player", "BallTrail", "1")
                self._set(vpinballSettings, "Player", "BallTrailStrength", "0.5")

            vpinball_mode = "desktop"
            dmd_w, dmd_h = self._dmd_size(b2sSize)

            # Desktop view keeps the table backdrop, where the table draws its own
            # backglass and DMD: an embedded ScoreView would be painted over them
            # and hide them. Leave it to the table. Written explicitly because
            # _set/_load overwrite values without clearing the other ones, so a
            # value left by a previous run in another mode would stick.
            # The other modes override this below.
            self._set(vpinballSettings, "ScoreView", "ScoreViewOutput", "0")

            # isTate(), not "> 0": only left/right stand the screen up. upsidedown
            # is still a landscape screen, just flipped, and must keep the desktop
            # view with its backdrop.
            if system.Rotation.isTate() and not cabinetMode:
                vpinball_mode = "FSS"
                # 1 is 'Cabinet': the only view that stands the table up on its own,
                # by turning the 3D scene 270 degrees inside a landscape framebuffer
                # (ViewSetup::GetRotation) to suit a landscape panel mounted upright.
                # Beware: it only turns the scene. The mouse, the in game UI and the
                # score panel below stay level, so the DMD reads sideways. A real cab
                # does not notice, its DMD has a screen of its own. Turning the whole
                # output instead (compositor transform) fixes that but breaks this
                # view: it projects through a window of the physical screen size
                # (Player.ScreenWidth, 95.89cm by default) that we do not know, and
                # the table comes out squashed. Tried, reverted, do not redo it.
                self._set(vpinballSettings, "Player", "BGSet", "1")
                # No backdrop in this view, so embed the DMD in the playfield, at the
                # bottom left. A separate DMD window (what the old PinMAMEWindow=1
                # used to give) needs the BGFX build, VPinballX_GL renders nothing
                # for it, so embedding is the only way left.
                # The size must be set: it defaults to 16384, which VPX clamps to the
                # whole playfield. The position is clamped with min(pos, playfield -
                # size), so 16384 means "far edge" and we never need to know the
                # playfield size here.
                # Width and height are swapped on purpose: the 008 patch turns the
                # panel content along with the table, so a 4:1 DMD needs a 1:4
                # region to land in. The region is a screen space rectangle, the
                # patch does not turn it.
                self._load(vpinballSettings, "ScoreView", {
                    "ScoreViewOutput": "2",
                    "ScoreViewWndX":   "0",         # left
                    "ScoreViewWndY":   "16384",     # bottom
                    "ScoreViewWidth":  str(dmd_h),
                    "ScoreViewHeight": str(dmd_w),
                })

            if cabinetMode and system.Rotation.isTate():
                if screen2 == "None":
                    recallog("*** Cabinet mode requires screen2 to be configured ***")
                    sys.exit(1)

                vpinball_mode = "cabinet"
                scr1_res = recalconf.getString("pinball.screen1.resolution", "1920x1080").split("x")
                scr2_res = recalconf.getString("pinball.screen2.resolution", "1920x1080").split("x")
                dmd_x = self._dmd_x(scr1_res[0], scr2_res[0], dmd_w)
                dmd_y = int(int(scr2_res[1]) * 0.8)

                self._set(vpinballSettings, "Player", "BGSet", "1")
                self._load(vpinballSettings, "Backglass", {
                    "BackglassOutput": "1",
                    "BackglassWndX":   scr1_res[0],
                    "BackglassWndY":   "0",
                    "BackglassWidth":  scr2_res[0],
                    "BackglassHeight": scr2_res[1],
                })
                self._load(vpinballSettings, "ScoreView", {
                    "ScoreViewOutput": "1",
                    "ScoreViewWndX":   str(dmd_x),
                    "ScoreViewWndY":   str(dmd_y),
                    "ScoreViewWidth":  str(dmd_w),
                    "ScoreViewHeight": str(dmd_h),
                })
                self._load(vpinballSettings, "Plugin.B2SLegacy", {
                    "B2SHideGrill": "0",
                })

            if autoConf:
                VpinballConfig(args.rom, vpinball_mode).get_ini()

            path_rom = os.path.dirname(args.rom)
            rom_stem = os.path.splitext(args.rom)[0]
            override_dir = os.path.join(path_rom, "override")
            if os.path.isdir(override_dir):
                override = os.path.join(override_dir, vpinball_mode + ".ini")
                fallback = os.path.join(override_dir, "default.ini")
                if os.path.exists(override):
                    recallog("*** Load override settings ***")
                    shutil.copy(override, rom_stem + ".ini")
                elif os.path.exists(fallback):
                    shutil.copy(fallback, rom_stem + ".ini")
                else:
                    rom_ini = rom_stem + ".ini"
                    if os.path.exists(rom_ini):
                        os.remove(rom_ini)

            with open(config_file, "w") as f:
                vpinballSettings.write(f)

        commandArray = [recalboxFiles.recalboxBins[system.Emulator],
                        "-PrefPath", recalboxFiles.vpinballRootFolder,
                        "-Ini",      config_file,
                        "-play",     args.rom]

        if system.HasArgs:
            commandArray.extend(system.Args)

        if Architecture().isPi5:
            # Leave the output in landscape: the cabinet view turns the table itself
            # (see BGSet above), and standing the output up would break its
            # projection. Only the half turns are ours to do, because VPX always
            # turns the same way: 'right' is the upright mount its 270 degrees miss
            # by half a turn, and 'upsidedown' is a landscape mount it has no rule
            # for at all. 'left' and 'none' need nothing.
            transform = "--transform 180" \
                if system.Rotation in (Rotation.upsidedown, Rotation.right) else ""
            # wlr-randr silently matches nothing when passed an empty output name,
            # and still exits 0, so neither the mode nor the transform was applied.
            # The name is unknown at config time: resolve it at launch. The Pi5 is
            # single screen here (cabinet mode is x86 only), so the first one is it.
            # Substitute it in place: runner.py wraps every item of the command in
            # double quotes for dwl -s, which runs the whole thing through /bin/sh,
            # so a "$var" would be expanded (to nothing) by that shell before bash
            # ever sees it, and its quotes would break the wrapping.
            shell = "wlr-randr --output $(wlr-randr | awk 'NR==1{{print $1}}') --mode {}@60Hz {} && {}".format(
                system.VideoMode,
                transform,
                " ".join("'{}'".format(a) for a in commandArray),
            )
            commandArray = ["bash", "-c", shell]

        # Feed SDL the recalbox controller mapping so VPX recognizes the pad with
        # the correct GameController layout and applies its default bindings
        # (otherwise non-Xbox pads get a shifted button numbering, leaving half the
        # inputs — e.g. the left flipper — dead). Same approach as batocera.
        controllerConfig = "\n".join(
            controller.generateSDLGameDBLine()
            for controller in playersControllers.values()
        )
        env = {"SDL_RENDER_VSYNC": "0"}
        if controllerConfig:
            env["SDL_GAMECONTROLLERCONFIG"] = controllerConfig

        return Command(videomode=system.VideoMode, array=commandArray, env=env)
