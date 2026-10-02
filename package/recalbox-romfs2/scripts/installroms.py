#!/usr/bin/env python

import os
import shutil
from pathlib import PurePath, Path
from typing import Dict, List, Union

from inisettings import IniSettings
from configin import ConfigIn
from systemholder import SystemHolder


class InstallRoms:
    __TAGS_FILE: str = ".templates/tags.ini"

    __README_FILES: Dict[str, str] = {
        "_readme.txt": "en",
        "_lisezmoi.txt": "fr",
        "_leiame.txt": "pt",
        "_leggime.txt": "it",
        "_liesmich.txt": "de",
        "_leeme.txt": "es",
    }

    __NULL_CORE = SystemHolder.Core("", 0, "", "", "", False, False, "", "", False, "", "", "", "")

    def __init__(self, systemRoot: str, target: str, root: str, lite: bool = False):
        self.__systemRoot = systemRoot
        self.__targetInit = target + "share_init/roms"
        self.__targetUpgrade = target + "share_upgrade/roms"
        self.__targetUpgradeSaves = target + "share_upgrade/saves"
        self.__targetUpgradeConfigs = target + "share_upgrade/system/configs"
        self.__tags = IniSettings(os.path.join(systemRoot, self.__TAGS_FILE), True)
        self.__tags.loadFile(True)
        self.__config = ConfigIn(root)
        self.__arch = self.__config.Arch
        self.__lite = lite

    def execute(self):
        print("[ROMFS 2] Copying systems and metadata to {}".format(self.__targetInit))

        # Run through systems folders
        for system_folder in os.listdir(self.__systemRoot):
            absolute_system_folder = os.path.join(self.__systemRoot, system_folder)
            if os.path.isdir(absolute_system_folder) and not system_folder.startswith('.'):
                holder = SystemHolder(self.__arch, os.path.join(absolute_system_folder, "system.ini"), self.__config)
                if holder.CoreCount < 1:
                    continue

                print("[ROMFS 2] Creating readme files for system {}".format(system_folder))
                self.__installReadMe(system_folder, holder)

                print("[ROMFS 2] Copying init in share_init {}".format(system_folder))
                self.__copyFolderContent(os.path.join(absolute_system_folder, "init" if not self.__lite else "init_lite", "roms"),
                                         holder, self.__targetInit)

                print("[ROMFS 2] Copying upgrade in share_upgrade {}".format(system_folder))
                self.__copyFolderContent(os.path.join(absolute_system_folder, "upgrade", "roms"),
                                         holder, self.__targetUpgrade)

                print("[ROMFS 2] Create rom subfolders in share_init {}".format(system_folder))
                self.__createSubFolders(holder, self.__targetUpgrade)

                print("[ROMFS 2] Copying saves in share_upgrade/saves {}".format(system_folder))
                self.__copyFolderContent(os.path.join(absolute_system_folder, "upgrade", "saves"),
                                         holder, self.__targetUpgradeSaves)

                print("[ROMFS 2] Create config in share_upgrade/system/configs {}".format(system_folder))
                self.__copyFolderContent(os.path.join(absolute_system_folder, "upgrade", "system", "configs"),
                                         holder, self.__targetUpgradeConfigs)

    @staticmethod
    def __createSubFolders(holder: SystemHolder, target: str):
        target_directory = holder.RomFolder.replace("%ROOT%", target)
        for emulator in holder.Cores:
            for core in holder.Cores[emulator]:
                if len(core.subfolder) > 0:
                    subTarget: Path = Path(os.path.join(target_directory, core.subfolder))
                    subTarget.mkdir(parents=True, exist_ok=True)
                    keep = subTarget.joinpath(".keep")
                    keep.touch(exist_ok=True)

    def __copyFolderContent(self, roms_folder: str, holder: SystemHolder, target: str):
        if os.path.exists(roms_folder):
            if "%ROOT%" in holder.RomFolder:
                # Specific case for system/configs, they are following
                # emulator name as folder name instead of the system name
                # Copy the folder with file from upgrade/system/configs
                # into share_upgrade/system/configs directly
                if target == self.__targetUpgradeConfigs:
                    target_directory = os.path.join(target, ".")
                else:
                    target_directory = holder.RomFolder.replace("%ROOT%", target)
                shutil.copytree(roms_folder, target_directory, dirs_exist_ok=True, symlinks=False)
                return
            elif holder.RomFolder == "/recalbox/share/screenshots":
                target_directory = os.path.join(target, "../screenshots")
                shutil.copytree(roms_folder, target_directory, dirs_exist_ok=True)
                return
            raise Exception("You should use a %ROOT% in rom folder")

    @staticmethod
    def getAbsolutePathTarget(holder:SystemHolder, root_to_replace: str, file_name: str):
        if "%ROOT%" in holder.RomFolder:
            return os.path.join(holder.RomFolder.replace("%ROOT%", root_to_replace), file_name)
        elif holder.RomFolder == "/recalbox/share/screenshots":
            return os.path.join(root_to_replace, "../screenshots", file_name)
        raise Exception("You should use a %ROOT% in rom folder")

    def __installReadMe(self, system_folder: str, holder: SystemHolder):
        template_dir: str = os.path.join(self.__systemRoot, ".templates/readme")
        absolute_system_folder = os.path.join(self.__systemRoot, system_folder)

        # Check if placeholder
        placeholder_file: str = self.__find(".readme.placeholder", absolute_system_folder)
        if placeholder_file is not None:
            # Choose where to write the file
            upgrade_or_init: str = PurePath(os.path.dirname(placeholder_file)).parent.name
            for file_name, language in self.__README_FILES.items():
                if upgrade_or_init == "upgrade":
                    absolute_readme_path_target: str = self.getAbsolutePathTarget(holder, self.__targetUpgrade, file_name)
                elif upgrade_or_init == "init":
                    absolute_readme_path_target: str = self.getAbsolutePathTarget(holder, self.__targetInit, file_name)
                else:
                    raise Exception("What in hell is {}".format(upgrade_or_init))
                # Getting template
                template_path: str = os.path.join(template_dir, file_name)
                if not os.path.exists(template_path):
                    raise FileNotFoundError("{} not found in {} !".format(template_path, file_name))
                # Process readme directly to target
                Path(absolute_readme_path_target).parent.mkdir(parents=True, exist_ok=True)
                with open(template_path, 'r') as template_file:
                    with open(absolute_readme_path_target, 'w') as readme_file:
                        for line in template_file.readlines():
                            readme_file.writelines(self.__parse(line, holder, language))

    @staticmethod
    def __find(name: str, path: str) -> Union[str, None]:
        for root, directory, files in os.walk(path):
            if name in files:
                return os.path.join(root, name)
        return None

    def __parse(self, line: str, holder: SystemHolder, language: str) -> List[str]:
        # Core loop?
        if line.startswith("$(FOREACHCORE)"):
            # Sort cores
            cores: List[SystemHolder.Core] = []
            for emulator in holder.Cores:
                for core in holder.Cores[emulator]:
                    cores.append(core)
            cores.sort(key=lambda c: c.priority)
            # Prepare lines
            lines: List[str] = []
            line = line.replace("$(FOREACHCORE)", "")
            for core in cores:
                newline = line.replace("$(EMULATOR)", core.emulator)
                newline = newline.replace("$(CORE)", core.core)
                newline = newline.replace("$(COREEXTENTIONS)", ' '.join(core.extensions).replace('!',''))
                newline = self.__parseTags(newline, language, holder)
                newline = self.__parseCommon(newline, holder, language)
                newline = self.__parseParameterized(newline, holder, core)
                lines.append(newline)
            return lines
        else:
            notEmpty: bool = line != '\n'
            line = self.__parseTags(line, language, holder)
            line = self.__parseCommon(line, holder, language)
            line = self.__parseParameterized(line, holder, self.__NULL_CORE)
            if notEmpty and line == '\n': return []
            return [line]

    @staticmethod
    def __parseParameterized(line: str, holder: SystemHolder, core: SystemHolder.Core) -> str:
        while True:
            start: int = line.find('$(')
            if start < 0:
                return line
            stop: int = line.find(')', start)
            if stop < 0:
                return line
            dotdot: int = line.count(':')
            if dotdot < 2:
                return line
            inner = line[start + 2: stop]
            chunks = inner.split(':')
            replacement = "<UNKNOWN_PARAMETERIZED_VARIABLE>"
            if chunks[0] == "CORENETPLAY" and len(chunks) == 3:
                replacement = chunks[1 if core.netplay else 2]
            elif chunks[0] == "PAD" and len(chunks) == 5:
                replacement = chunks[1 + holder.Properties.PadEnum]
            elif chunks[0] == "KEYBOARD" and len(chunks) == 5:
                replacement = chunks[1 + holder.Properties.KeyboardEnum]
            elif chunks[0] == "MOUSE" and len(chunks) == 5:
                replacement = chunks[1 + holder.Properties.MouseEnum]
            elif chunks[0] == "TYPE" and len(chunks) == 9:
                replacement = chunks[1 + holder.Properties.TypeEnum]
            line = line.replace(line[start: stop + 1], replacement)

    @staticmethod
    def __parseCommon(line: str, holder: SystemHolder, language: str) -> str:
        line = line.replace("$(NAME)", holder.Name)
        line = line.replace("$(FULLNAME)", holder.FullName)
        line = line.replace("$(EXTENSIONS)", holder.Extensions.replace('!',''))
        line = line.replace("$(CORECOUNT)", str(holder.CoreCount))
        line = line.replace("$(DOCLINK)", holder.DocLinks(language))
        return line

    def __parseTags(self, line: str, language: str, holder: SystemHolder) -> str:
        # Special documentation tags
        docText: str = ""
        if len(holder.DocLinks(language)) != 0:
            docText = self.__tags.getOption("doc", "doc.text." + language, "").replace("\\n", "\n")
        line = line.replace("$(TAGS:DOC.TEXT)", docText)

        # Special zip tags
        zipText: str = ""
        if holder.Properties.Type != "arcade" and (".zip" in holder.Extensions or ".7z" in holder.Extensions):
            zipText = self.__tags.getOption("doc", "zip.onerom." + language, "").replace("\\n", "\n")
        line = line.replace("$(TAGS:ZIP.ONEROM)", zipText)

        # All other tags
        while True:
            start: int = line.find('$(TAGS:')
            if start < 0:
                return line
            stop: int = line.find(')', start)
            if stop < 0:
                return line
            tag = line[start + 7: stop].lower() + '.' + language
            line = line.replace(line[start: stop + 1], self.__tags.getOption("doc", tag, "").replace("\\n", "\n"))
