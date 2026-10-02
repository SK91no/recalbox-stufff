Install `7z` and `xsltproc`:
```bash
apt-get install 7zip xsltproc python3-jinja2
```

Update files for a system:
```bash
python3 updater.py -target <corename>
```

Example:
```bash
python3 updater.py -target fbneo
```

Available targets:
- advancemame
- mame2000
- mame2003
- mame2003+
- mame2010
- mame2015
- mame0278
- mame0258
- pifba
- fbneo
- dice
- supermodel
- flycast
- flycast-next
- controller
