#!/usr/bin/env python
import argparse
import glob
import os
import sys
import urllib.request
import zipfile

from simplify import Simplifier

from jinja2 import Template
import xml.dom.minidom

from controller import template

class Updater:

    mameVersion = "mame0278"
    sMameDir = "/tmp"
    sMameZip = os.path.join(sMameDir, "mame.zip")
    sMameUrl = "https://progettosnaps.net/dats/MAME/packs/MAME_Dats_{}.7z"
    sMameXmlTemplate = os.path.join(sMameDir, "mame{}.xml")
    sMameDatTemplate = os.path.join(sMameDir, "MAME {} (arcade).dat")
    sSystemDir = os.path.join(sMameDir, "recalbox-system-extraction")
    sSystemZip = os.path.join(sMameDir, "tmp.compressed")

    def __init__(self, args: argparse.Namespace):
        self.__us: str = os.path.dirname(os.path.realpath(__file__))
        self.__package: str = os.path.dirname(self.__us)
        self.__args = args

    class SystemDescriptor:

        def __init__(self, name: str, package: str, systems: list[str], urlTemplate: str, relativePath: list[str], mameXmlPathOverride: str):
            self.__name: str = name
            self.__package: str = package
            self.__systems: list[str] = systems
            self.__urlTemplate: str = urlTemplate
            self.__relativePath: list[str] = relativePath
            self.__mameXMLPath: str = mameXmlPathOverride

        @property
        def Name(self) -> str: return self.__name

        @property
        def Package(self) -> str: return self.__package

        @property
        def Systems(self) -> list[str]: return self.__systems

        @property
        def UrlTemplate(self) -> str: return self.__urlTemplate

        @property
        def RelativePath(self) -> list[str]: return self.__relativePath

        @property
        def MameXMLPathOverride(self) -> list[str]: return self.__mameXMLPath

    Descriptors: dict[str, SystemDescriptor] = {
        "advancemame": SystemDescriptor("advancemame", "advancemame"           , ["advancemame"]                             , "https://www.progettosnaps.net/dats/MAME/packs/MAME_Dats_106.7z"      , ["MAME 0.106u13.dat"], None),
        "mame2000"   : SystemDescriptor("mame2000"   , "libretro-mame2000"     , ["mame2000"]                                , "https://github.com/libretro/mame2000-libretro/archive/{}.tar.gz"     , ["metadata/MAME 0.37b5 XML.dat"], None),
        "mame2003"   : SystemDescriptor("mame2003"   , "libretro-mame2003"     , ["mame2003"]                                , "https://github.com/libretro/mame2003-libretro/archive/{}.tar.gz"     , ["metadata/mame2003.xml"], None),
        "mame2003+"  : SystemDescriptor("mame2003+"  , "libretro-mame2003-plus", ["mame2003-plus"]                           , "https://github.com/libretro/mame2003-plus-libretro/archive/{}.tar.gz", ["metadata/mame2003-plus.xml"], None),
        "mame2010"   : SystemDescriptor("mame2010"   , "libretro-mame2010"     , ["mame2010"]                                , "https://github.com/libretro/mame2010-libretro/archive/{}.tar.gz"     , ["metadata/mame2010.xml"], None),
        "mame2015"   : SystemDescriptor("mame2015"   , "libretro-mame2015"     , ["mame2015"]                                , "https://github.com/libretro/mame2015-libretro/archive/{}.tar.gz"     , ["metadata/mame2015-xml.zip"], None),
        "{}".format(mameVersion)   : SystemDescriptor("{}".format(mameVersion)       , "libretro-{}".format(mameVersion)     , ["{}".format(mameVersion)]                                , "https://www.progettosnaps.net/dats/MAME/packs/MAME_Dats_{}.7z"       , ["MAME 0.{} (arcade).dat", "MAME 0.{} (Arcade).dat"], "mame_717_0.278.xml"),
        "mame0258"   : SystemDescriptor("mame0258"       , "libretro-mame0258"     , ["mame0258"]                                , "https://www.progettosnaps.net/dats/MAME/packs/MAME_Dats_258.7z"       , ["MAME 0.258 (arcade).dat", "MAME 0.258 (Arcade).dat"], None),
        "pifba"      : SystemDescriptor("pifba"      , "pifba"                 , ["pifba"]                                   , "packages:pifba"                                                      , ["fba_rb.dat"], None),
        "fbneo"      : SystemDescriptor("fbneo"      , "libretro-fbneo"        , ["fbneo"]                                   , "https://github.com/libretro/FBNeo/archive/{}.tar.gz"                 , ["dats/FinalBurn Neo (ClrMame Pro XML, Arcade only).dat"], None),
        "dice"       : SystemDescriptor("dice"       , "libretro-dice"         , ["dice"]                                    , "https://github.com/mittonk/dice-libretro/archive/{}.tar.gz"          , ["dice_xml.dat"], None),
        "supermodel" : SystemDescriptor("supermodel" , "supermodel"            , ["supermodel"]                              , "", [], None),
        "flycast"    : SystemDescriptor("flycast"    , "libretro-flycast"      , ["naomi", "naomigd", "atomiswave"], "", [], None),
        "controller" : SystemDescriptor("controller" , "libretro-{}".format(mameVersion), ["mame"]                          , "https://www.progettosnaps.net/dats/MAME/packs/MAME_Dats_{}.7z"       , ["MAME 0.{} (arcade).dat", "MAME 0.{} (Arcade).dat"], "mame_717_0.278.xml"),
        "mednafen" : SystemDescriptor("mednafen"    , "mednafen"      , ["stv"], "", []),
        "flycast-next" : SystemDescriptor("flycast-next"    , "libretro-flycast-next"      , ["naomi", "naomigd", "atomiswave", "naomi2"], "", [], "mame_717_0.278.xml"),
    }

    def execute(self):
        # Execute
        systems: list[str] = [self.__args.target]
        if self.__args.target == "all":
            systems.clear()
            for k, v in self.Descriptors.items():
                systems.append(k)
        self.create(systems)

    @staticmethod
    def extractVersion(folder: str):
        files: list[str] = glob.glob(os.path.join(folder, "*.mk"))
        with open(files[0], "r") as f:
            for line in f.readlines():
                # generic case
                p: int = line.find('=')
                if p >= 0:
                    key = line[:p].strip(' \r\n\t')
                    value = line[p+1:].strip(' \r\n\t')
                    if key.endswith("_VERSION"):
                        return value
        print("Cannot extract version from package {}".format(folder))
        sys.exit(0)

    def downloadMameDat(self, descriptor: SystemDescriptor) -> (str, str):
        pureMameVersion: str = self.extractVersion(os.path.join(self.__package, "arcade-dats"))
        mameVersion = '0.'+pureMameVersion
        mameDat = self.sMameDatTemplate.format(mameVersion)
        mameXml = self.sMameXmlTemplate.format(pureMameVersion)
        if not os.path.exists(mameXml) or not os.path.exists(mameDat):
            print("  Downloading reference mame dat {} ({} to {})".format(mameVersion, self.sMameUrl.format(pureMameVersion), self.sMameZip))
            try:
                urllib.request.urlretrieve(self.sMameUrl.format(pureMameVersion), self.sMameZip)
            except:
                print("  WARNING: Reference mame {} doesn't exist".format(mameVersion))
                sys.exit(0)
            urllib.request.urlretrieve(self.sMameUrl.format(pureMameVersion), self.sMameZip)
            try:
                with urllib.request.urlopen(self.sMameUrl.format(str(int(pureMameVersion)))):
                    print("  WARNING: mame {} is not the latest version! Fix the version in arcade-dats.mk".format(mameVersion))
            except:
                print("  mame {} seems to be the latest version!".format(mameVersion))
            print("  Uncrunching {}".format(self.sMameZip))
            if os.system("7z e -y -o/tmp {} >/dev/null".format(self.sMameZip)) != 0:
                os.system("7zz e -y -o/tmp {} >/dev/null".format(self.sMameZip))
            if not os.path.exists(mameXml):
                if descriptor.MameXMLPathOverride != None:
                    os.system("mv /tmp/{} {}".format(descriptor.MameXMLPathOverride, mameXml))
            if not os.path.exists(mameXml) or not os.path.exists(mameDat):
                print("Cannot extract {} and/or {}".format(mameDat, mameXml))
                sys.exit(0)
        return mameDat, mameXml

    def generateOrDownloadDat(self, descriptor: SystemDescriptor, systemVersion: str, mameDat: str):
        if descriptor.UrlTemplate == "":
            for subsystem in descriptor.Systems:
                print("  Generating {} dat file".format(subsystem))
                os.makedirs("./precompiled", exist_ok=True)
                status = os.system("xsltproc ./{}.xslt '{}' > ./precompiled/{}-{}.dat".format(subsystem, mameDat, subsystem, systemVersion))
                if status != 0:
                    print("Cannot generate {} dat".format(subsystem))
                    sys.exit(0)
        elif descriptor.UrlTemplate.startswith("packages:"):
            for subsystem in descriptor.Systems:
                datPath: str = os.path.join(self.__package, descriptor.UrlTemplate[len("packages:"):], descriptor.RelativePath[0])
                destination: str = os.path.join(self.__us, "precompiled", subsystem + '-' + systemVersion + ".dat")
                if not os.path.exists(datPath):
                    print("Cannot find dat file {}".format(datPath))
                    sys.exit(0)
                if os.system("cp '{}' {}".format(datPath, destination)):
                    print("Error copying {} to {}".format(datPath, destination))
                    sys.exit(0)
        else:
            for subsystem in descriptor.Systems:
                destination: str = os.path.join(self.__us, "precompiled", subsystem + '-' + systemVersion + ".dat")
                if not os.path.exists(destination):
                    if subsystem == self.mameVersion:
                        #systemVersion = systemVersion[2:]
                        systemVersion: str = self.extractVersion(os.path.join(self.__package, "arcade-dats"))
                    url: str = descriptor.UrlTemplate.format(systemVersion)
                    print("  Downloading package of {} at {}".format(descriptor.Name, url))
                    urllib.request.urlretrieve(url, self.sSystemZip)
                    print("  Uncrunching {}".format(self.sSystemZip))
                    os.makedirs(self.sSystemDir, exist_ok=True)
                    if descriptor.UrlTemplate.endswith(".tar.gz"):
                        os.system("tar -zxf {} -C {} --strip-components=1".format(self.sSystemZip, self.sSystemDir))
                    elif descriptor.UrlTemplate.endswith(".tar.zx") or descriptor.UrlTemplate.endswith(".tar.bz2"):
                        os.system("tar -jxf {} -C {} --strip-components=1".format(self.sSystemZip, self.sSystemDir))
                    elif descriptor.UrlTemplate.endswith(".zip") or descriptor.UrlTemplate.endswith(".7z"):
                        if os.system("7z e -y -o{} {} >/dev/null".format(self.sSystemDir, self.sSystemZip)) != 0:
                            os.system("7zz e -y -o{} {} >/dev/null".format(self.sSystemDir, self.sSystemZip))
                    else:
                        print("No extractor defined for file {}".format(descriptor.UrlTemplate))
                        sys.exit(0)
                    found: bool = False
                    for relativePath in descriptor.RelativePath:
                        if found: break;
                        relativePath = relativePath.format(systemVersion)
                        datPath = os.path.join(self.sSystemDir, relativePath)
                        #print("  Trying to find {}".format(relativePath))
                        if relativePath.endswith(".zip"):
                            with zipfile.ZipFile(datPath) as z:
                                for name in z.namelist():
                                    if name.endswith(".xml"):
                                        z.extract(name, os.path.dirname(destination))
                                        os.rename(os.path.join(os.path.dirname(destination), name), destination)
                                        found = True
                        else:
                            if os.path.exists(datPath):
                                if os.system("mv '{}' {}".format(datPath, destination)):
                                    print("Error copying {} to {}".format(datPath, destination))
                                    sys.exit(0)
                                found = True
                    if not found:
                        print("Error extracting valid dat file.")
                        sys.exit(0)

    @staticmethod
    def generateFlatDat(descriptor: SystemDescriptor, version: str):
        for subsystem in descriptor.Systems:
            print("  Generating {} flat dat".format(subsystem))
            status = os.system("xsltproc ./arcade-flat.xslt ./precompiled/{}-{}.dat > ./precompiled/{}-{}.fdt".format(subsystem, version, subsystem, version))
            if status != 0:
                print("Cannot generate {} flat dat".format(subsystem))
                sys.exit(0)

    @staticmethod
    def generateControllerList():
        os.system("rm ./precompiled/controller-*.lst 2>/dev/null")
        print("  Generating controller list")
        xml_doc = xml.dom.minidom.parse(os.path.join(Updater.sMameDir, "mame258.xml"))
        machines = xml_doc.getElementsByTagName("machine")
        t = Template(template)

        # One line per machine that exposes at least one control. The template
        # handles player grouping (and player-less controls -> player 1), so we
        # only need the raw control attributes. Truncate ("w") to avoid appending
        # to a previous run, and write each machine exactly once (no stale 'd').
        with open("./precompiled/controller.lst", "w") as f:
            for machine in machines:
                controls = machine.getElementsByTagName("control")
                if controls.length < 1:
                    continue
                d = {
                    "name": machine.getAttribute("name"),
                    "controls": [
                        {a.name: a.value for a in control.attributes.values()}
                        for control in controls
                    ],
                }
                f.write(t.render(d=d) + "\n")

    @staticmethod
    def generateFlatList(descriptor: SystemDescriptor, version: str, mameXml: str):
        for subsystem in descriptor.Systems:
            print("  Generating {} flat list".format(subsystem))
            flatName: str = "./precompiled/{}-{}.lst".format(subsystem, version)
            status = os.system("xsltproc --stringparam lastmamexml {} ./arcade.xslt ./precompiled/{}-{}.dat > {}.original".format(mameXml, subsystem, version, flatName))
            if status != 0:
                print("Cannot generate {} flat list".format(subsystem))
                sys.exit(0)
            print("  Simplifying {} manufacturers".format(subsystem))
            simplifier = Simplifier("{}.original".format(flatName), flatName)
            if not simplifier.execute(False):
                print("Cannot simplify {} flat list".format(subsystem))
                sys.exit(0)

    def create(self, systems: list[str]):
        for system in systems:
            if system not in self.Descriptors:
                knownList: str = ""
                for s in self.Descriptors: knownList += s + ", "
                print("Unknown system {}. Known systems are: {}".format(system, knownList.strip(", ")))
                sys.exit(0)
            descriptor: Updater.SystemDescriptor = self.Descriptors[system]
            print("Processing system {} in package {}:".format(descriptor.Name, descriptor.Package))
            folder: str = os.path.join(self.__package, descriptor.Package)
            if not os.path.exists(folder):
                print("{} package not found!".format(folder))
                sys.exit(0)
            # Get package version
            version: str = self.extractVersion(folder)
            print("  Found version: {}".format(version))
            # Get generic mame dat
            mameDat, mameXml = self.downloadMameDat(descriptor)
            # generate controller lists
            if system == "controller":
                self.generateControllerList()
            else:
                # Generate dat when required
                self.generateOrDownloadDat(descriptor, version, mameDat)
                # generate flat dat
                self.generateFlatDat(descriptor, version)
                # generate flat lists
                self.generateFlatList(descriptor, version, mameXml)


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='arcade database generator')
    parser.add_argument("-target", help="package", type=str, required=True)
    arguments = parser.parse_args()

    fs = Updater(arguments)
    fs.execute()
