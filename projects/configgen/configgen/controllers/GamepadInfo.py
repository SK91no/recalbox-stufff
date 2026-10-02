from enum import StrEnum


class GamepadType(StrEnum):
    """
    Physical layout of the controller — what the pad IS, as configured by
    the user in EmulationStation. The string values match those emitted
    by ES (see ``projects/frontend/es-core/src/input/PadTypes.cpp``).
    """
    Standard    = "standard"
    HeightBitdo = "8bitdo"
    XBox        = "xbox"
    Playstation = "sony"
    Arcade4     = "arcade4"
    Arcade6     = "arcade6"
    Arcade8     = "arcade8"
    Gamecube    = "gamecube"
    Megadrive3  = "megadrive3"
    Megadrive6  = "megadrive6"
    N64         = "n64"


class Layout(StrEnum):
    """
    Per-game gameplay overlay. The frontend only emits ``--jammalayoutp1/p2``
    when ``ShouldConfigureArcadePanelConfiggen()`` is true (i.e. on RGBJamma
    or JVS hardware). On a regular non-Jamma board the CLI flag is absent
    altogether — that case is represented by ``NotProvided``, which is
    distinct from a frontend that explicitly forwarded ``"standard"``.
    Only meaningful on arcade 6/8-button sticks — for any other Type the
    layout is forced to ``NotProvided``.
    """
    NotProvided = ""            # No CLI override (frontend off-Jamma/JVS)
    Standard    = "standard"    # Jamma frontend forwarded "standard"
    SixBtn      = "6btns"
    NeoLine     = "line"
    NeoSquare   = "square"
    NeoDefault  = "neodefault"


# Arcade frontends where an Arcade6/Arcade8 stick is treated as a plain
# arcade pad (no 6-button reshuffle, RETROPAD_CLASSIC for fbneo). Other
# systems with the same stick get the legacy reshuffle so a 6-button
# physical panel matches what a 3/4-button game expects.
_ARCADE_SYSTEMS = frozenset({"mame", "fbneo", "naomi", "naomigd", "atomiswave"})

# Arcade systems whose libretro core maps a 6-button arcade panel natively
# correctly — no need to reshuffle the retropad bindings for them. Empirical
# list: MAME confirmed OK; fbneo explicitly excluded (its native ABXY layout
# does not match a typical 6-button arcade panel and needs the shuffle to
# get LP/MP/HP / LK/MK/HK in the expected positions). Add naomi/atomiswave
# here once verified.
_NO_SHUFFLE_ARCADE_SYSTEMS = frozenset({"mame"})


class GamepadInfo:
    """
    Combines the controller's stable physical Type with its volatile
    per-game Layout. One instance per Controller.

    Construction enforces the invariant "Layout only honored on
    Arcade6/Arcade8" so callers cannot accidentally apply a NeoGeo layout
    to e.g. a Megadrive3.
    """

    def __init__(self, type_str: str = "", layout_str: str = ""):
        try:
            self._type = GamepadType(type_str) if type_str else GamepadType.Standard
        except ValueError:
            self._type = GamepadType.Standard

        # A CLI layout only ever comes from Jamma/JVS hardware, where the panel is an
        # arcade stick by construction. An es_input.cfg entry predating the gamepadtype
        # attribute loads as Standard and gets re-normalized to the "standard" string on
        # the layout-carrying construction: both spellings of absence must infer the type.
        if layout_str and layout_str != "standard" and self._type == GamepadType.Standard:
            self._type = GamepadType.Arcade6

        if layout_str and self.isArcade6Buttons():
            try:
                self._layout = Layout(layout_str)
            except ValueError:
                self._layout = Layout.NotProvided
        else:
            self._layout = Layout.NotProvided

    @property
    def Type(self) -> GamepadType:
        return self._type

    @property
    def Layout(self) -> Layout:
        return self._layout

    def isArcade(self) -> bool:
        """True for any arcade-stick layout (4/6/8 buttons)."""
        return self._type in (GamepadType.Arcade4, GamepadType.Arcade6, GamepadType.Arcade8)

    def isArcade6Buttons(self) -> bool:
        """True for sticks with at least 6 buttons exposed (Arcade6 + Arcade8)."""
        return self._type in (GamepadType.Arcade6, GamepadType.Arcade8)

    def hasLayout(self) -> bool:
        """
        True when the frontend forwarded a CLI layout (``--jammalayoutp1/p2``,
        i.e. RGB Jamma or JVS hardware). On a non-Jamma board the layout is
        absent (``NotProvided``) and fbneo handles the buttons natively via
        ``RETROPAD_CLASSIC``.
        """
        return self._layout != Layout.NotProvided

    def isSixBtnLayout(self) -> bool:
        """
        True when the frontend forwarded the legacy ``6btns`` jamma layout
        (panel 5/6 boutons + jeu non-arcade). Triggers the B/A/Y→Y/X/L,
        X/L/R→B/A/R button reshuffle in ``Controller.LoadUserControllerConfigurations``.
        Neo* layouts keep the native button mapping (no reshuffle).
        """
        return self._layout == Layout.SixBtn

    def shouldReshuffle6Btn(self, system: str) -> bool:
        """
        True when the 6-button arcade reshuffle (B/A/Y → Y/X/L, X/L/R → B/A/R)
        should be applied to an Arcade6/Arcade8 stick.

        Triggered by:
        - ``Layout == SixBtn`` (Jamma/JVS forwarding ``6btns`` via
          ``--jammalayoutpN``): same trigger as the legacy
          ``jammalayoutpN == "6btns"`` flag.
        - ``Layout.NotProvided`` (HDMI/RGB-dual, no CLI layout) AND ``system``
          is NOT in ``_NO_SHUFFLE_ARCADE_SYSTEMS``: the panel is remapped to
          the pad-style layout the game expects. MAME is currently the only
          system that maps the panel correctly natively and is therefore
          exempted; fbneo is *not* exempted (see
          ``_NO_SHUFFLE_ARCADE_SYSTEMS`` comment).
        """
        if not self.isArcade6Buttons():
            return False
        if self.isSixBtnLayout():
            return True
        if self._layout == Layout.NotProvided:
            return system not in _NO_SHUFFLE_ARCADE_SYSTEMS
        return False

    def toRetroarchDevice(self, system: str) -> str:
        """
        RetroArch ``input_libretro_device_p<N>`` value. Only Arcade6/Arcade8
        sticks with a CLI layout (``--jammalayoutpN``) produce a non-default
        device ID — every other case returns "1" (RETRO_DEVICE_JOYPAD), so
        the arcade panel goes through the standard reshuffle in
        ``shouldReshuffle6Btn`` and uses the regular pad mapping.

        When a CLI layout is forwarded (``Layout != NotProvided``):
        - NeoLine    → 1029 (libretro-fbneo ``RETRO_RB_JAMMA_NEOLINE``)
        - NeoSquare  → 1285 (libretro-fbneo ``RETRO_RB_JAMMA_NEOSQUARE``)
        - NeoDefault → 1541 (libretro-fbneo ``RETRO_RB_JAMMA_STANDARD`` —
          still actively used by RGB Jamma neogeo)
        - dreamcast + SixBtn → 1025 (core flycast 6-button arcade subclass)
        - any other Layout (Standard / SixBtn off-dreamcast) → 1
        """
        if not self.isArcade6Buttons(): return "1"
        # CLI layout forwarded → preserve the legacy JammaLayout behaviour.
        if self._layout == Layout.NeoLine:    return "1029"
        if self._layout == Layout.NeoSquare:  return "1285"
        if self._layout == Layout.NeoDefault: return "1541"
        if system == "dreamcast" and self._layout == Layout.SixBtn: return "1025"
        return "1"
