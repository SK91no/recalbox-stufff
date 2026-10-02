# Jinja2 template producing one line per arcade ROM for controller.lst.
#
# Deterministic format — one segment per control, the player index repeated:
#
#   rom:player|type|ways|buttons;player|type|ways|buttons;...
#
#   - segments separated by ';', fields by '|', fixed field order
#   - player  : 1-based player index (int)
#   - type    : control type token, 1:1 mappable to the frontend ControlType enum.
#               MAME encodes "vertical"/"strange" in the *ways* attribute; we fold
#               them into the type so the type stays a clean token and ways stays
#               numeric (e.g. type="joy" ways="vertical2" -> type "joyvertical", ways 2).
#   - ways    : directional ways as an int (0 if not applicable)
#   - buttons : number of buttons as an int (0 if not specified)
#
# Examples:
#   centiped:1|joy|8|1;1|trackball|0|0
#   720:1|stick|0|2;1|dial|0|0
#   a51site4:1|joy|5|9;1|keyboard|0|103;2|joy|5|3
#
# Rationale: the previous format glued type+ways together and used "control=" as a
# context-dependent separator (e.g. "joyvertical2", "mousecontrol=keyboard"), which
# could not be parsed deterministically. This format has one rigid grammar.

template = (
    "{{ d.name }}:"
    # Iterate players in numeric order, then their controls in document order,
    # so player 1's primary control is always the first emitted segment.
    "{% set ns = namespace(first=True) %}"
    # Some MAME <control> elements have no 'player' attribute (e.g. shared
    # keyboards); default them to player 1 rather than dropping them.
    "{% set players = d.controls | map(attribute='player') | map('default', '1', true) | map('int') | unique | sort %}"
    "{% for pnum in players %}"
        "{% for ctrl in d.controls if (ctrl.player | default('1', true) | int) == pnum %}"
            "{% if not ns.first %};{% endif %}"
            "{% set ns.first = False %}"
            # Fold MAME "vertical"/"strange" ways qualifiers into the type token;
            # otherwise take the leading integer of ways ("8", "5 (half8)" -> 5).
            "{% set w = ctrl.ways | default('', true) %}"
            "{% if w.startswith('vertical') %}"
                "{% set typ = ctrl.type ~ 'vertical' %}{% set ways = w[8:] | int %}"
            "{% elif w.startswith('strange') %}"
                "{% set typ = ctrl.type ~ 'strange' %}{% set ways = w[7:] | int %}"
            "{% else %}"
                "{% set typ = ctrl.type %}{% set ways = w.split(' ')[0] | int %}"
            "{% endif %}"
            "{{ pnum }}|{{ typ }}|{{ ways }}|{{ ctrl.buttons | default(0, true) | int }}"
        "{% endfor %}"
    "{% endfor %}"
)
