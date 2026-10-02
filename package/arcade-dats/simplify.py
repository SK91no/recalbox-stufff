#!/usr/bin/env python3
import argparse

class Simplifier:

    def __init__(self, source: str, destination: str):
        self.__source = source
        self.__destination = destination

    def Load(self) -> list[str]:
        with open(self.__source, 'r') as f:
            lines: list[str] = f.readlines()
        return lines

    def Save(self, lines: list[str]):
        with open(self.__destination, 'w') as f:
            for line in lines:
                f.write(line + '\n')

    def SaveManufacturers(self, onestat: dict[str, int]):
        with open(self.__destination + ".manufacturers", 'w') as f:
            count: int = 1
            for k, v in onestat.items():
                f.write("{} - {} ({})".format(count, k, v))
                count += 1

    SUBSYSTEM_MANUFACTURERS: dict[str, list[tuple[str, str]]] = \
    {
        "capcom": [("cps1", "CPS1"), ("cps2", "CPS2"), ("cps3", "CPS3")],
        "irem" : [("m72", "M72"), ("m92", "M92")],
        "konami": [("konamigx", "GX")],
        "namco": [("namcona*", "NA"), ("namconb*", "NB"), ("namcos1", "System1"), ("namcos2", "System2"), ("namcos10", "System10"), ("namcos11", "System11"), ("namcos12", "System12")],
        "sega": [("segas32", "System32"), ("segas16*", "System16"), ("segas18", "System18"), ("stv", "STV")],
        "taito": [("taito_f3", "F3"), ("taitogn", "GNET")],
    }

    COMPLETE_REPLACEMENTS: dict[str, str] = \
    {
        "8ing": "Eighting",
        "Data East": "Data\u00a0East",  # Unsecable space to make two word, one word, so that we can eliminate tailing words
        "Electronic Devices": "Electronic\u00a0Devices",
        "Esco": "Esco\u00a0Trading",
        "Esco Trading": "Esco\u00a0Trading",
        "Video Game": "Video\u00a0Game",
        "Armenip": "Armenia",
        "N M K": "NMK",
        "FAMARE": "Famare",
        "Omori": "Omori Electric",
        "Liang Hwa": "Liang HWA Electronics",
        "Playmark bootleg": "Playmark",
        "Status Game": "Status Games",
        "Broderbund Software": "Broderbund",
        "Coinmaster-Gaming": "Coinmaster",
        "Incredible Technologies": "ITech",
        "Williams": "Midway",
    }

    def simplifyWords(self, manufacturer: str):
        # Eliminate "license" word and all derivatives
        manufacturer = manufacturer.replace("Licenses", "").replace("licenses", "") \
                                   .replace("Licensed to", "").replace("licensed to", "") \
                                   .replace("Licensed from", "").replace("licensed from", "") \
                                   .replace("Licensed", "").replace("licensed", "") \
                                   .replace("License", "").replace("license", "") \
                                   .replace("Licence", "").replace("licence", "") \
                                   .strip('\t\r\n ()[]?')

        # Typos
        manufacturer = manufacturer.replace("BreezaSoft", "BrezzaSoft") \
                                   .replace("Cocomatic", "Cocamatic") \
                                   .replace("Zilex-Zenitone", "Zilec-Zenitone") \
                                   .replace("Falcom", "Falcon") \
                                   .replace("Video Spiel", "Video Games") \
                                   .replace("Karateko", "Karateco") \
                                   .replace("Corpotation", "Corporation") \
                                   .replace(" and ", " & ") \
                                   .replace(" And ", " & ") \
                                   .replace("Signaton", "Signatron")

        # Eliminations/Replacements
        manufacturer = manufacturer.replace(", distritued by ", "/") \
                                   .replace(" GP9001 based", "") \
                                   .replace("007325", "") \
                                   .replace("FULLSET", "") \
                                   .replace("www.", "") \

        # End of word eliminations
        while True:
            found: bool = False
            for suffix in (" S.A.", " SA", ", Inc.", " Inc.", " Inc", " Corp.", " USA", " of America", " America", " Corporation", " Corp", " Co., Ltd.", " GmbH",
                           " Co. Ltd.", " Co Ltd.", " Co. Ltd", " Ltd.", " Co.", " Co", " co-ltd", " Co-Ltd", " Ltd", " LTD.", ", LTD", " SRL", " S.R.L.", "S.L.",
                           " Japan", " Italy", " UK", " Spain", " du Canade", " of Canada"):
                if manufacturer.endswith(suffix):
                    manufacturer = manufacturer[0:-len(suffix)].strip(' ,')
                    found = True
            if not found:
                break

        # Start of word eliminations
        for suffix in ["G. "]:
            if manufacturer.startswith(suffix):
                manufacturer = manufacturer[len(suffix):]

        # Replacements
        if manufacturer in self.COMPLETE_REPLACEMENTS:
            manufacturer = self.COMPLETE_REPLACEMENTS[manufacturer]

        # Special hack by
        if manufacturer.lower().startswith("hack by"): manufacturer = ""

        # Keep only the first word
        words: list[str] = manufacturer.split(' ')
        if len(words) > 0:
            if words[0].lower() in ("able", "alta", "amenip", "atari", "atw", "barko", "calfesa", "centromatic", "comad", "electrogame", "electronic\u00a0devices", "esco\u00a0trading",
                                    "sigma",  "taito", "sammy", "seibu", "snk", "taiyo", "data\u00a0east", "video\u00a0game", "zilec"):
                manufacturer = words[0]

        return manufacturer.strip(' ,')

    @staticmethod
    def score(s: str) -> int:
        sc: int = 0
        for c in s:
            if c.isalpha():
                if c.isupper():
                    sc += 1
            else:
                if not c.isdigit():
                    sc += 100
        return sc

    @staticmethod
    def purge(s: str) -> str:
        result: str = ''
        for c in s:
            if c.isalpha() or c.isdigit():
                result += c.lower()
        return result

    def simplify(self, manufacturer: str, driver: str, onestats: dict[str, int]) -> list[str]:
        manufacturers: list[str] = [manufacturer]

        # Split by slash?
        outManufacturers: list[str] = []
        for manu in manufacturers:
            if '/' in manu: outManufacturers.extend(manu.split('/'))
            else:           outManufacturers.append(manu)
        manufacturers = outManufacturers.copy()

        # Split by ()?
        outManufacturers.clear()
        for manu in manufacturers:
            if '(' in manu: outManufacturers.extend(manu.split('('))
            else:           outManufacturers.append(manu)
        manufacturers = outManufacturers.copy()

        # Split by [] at start (special case)
        outManufacturers.clear()
        for manu in manufacturers:
            if manu.startswith('['): outManufacturers.extend(manu.split(']'))
            else:                    outManufacturers.append(manu)
        manufacturers = outManufacturers.copy()

        # Split by []?
        outManufacturers.clear()
        for manu in manufacturers:
            if '[' in manu: outManufacturers.extend(manu.split('['))
            else:           outManufacturers.append(manu)
        manufacturers = outManufacturers.copy()

        # Split by +?
        outManufacturers.clear()
        for manu in manufacturers:
            if '+' in manu: outManufacturers.extend(manu.split('+'))
            else:           outManufacturers.append(manu)
        manufacturers = outManufacturers.copy()

        # Strip
        for i in range(len(manufacturers)):
            manufacturers[i] = manufacturers[i].strip('\t\r\n ()[]')

        # Simplify words
        for i in range(len(manufacturers)):
            manufacturers[i] = self.simplifyWords(manufacturers[i])

        # Remove bootlegs and other useless words
        for word in ("bootleg", "bootleg?", "<unknown>", "unknown", "hack", "homebrew", "Homebrew", ""):
            if word in manufacturers: manufacturers.remove(word)

        # Remove empty manufacturers
        outManufacturers.clear()
        for manu in manufacturers:
            if len(manu) > 0: outManufacturers.append(manu)
        manufacturers = outManufacturers.copy()

        # Driver mix
        driverLower = driver.lower().replace(".cpp", "")
        if '/' in driver:
            parts: list[str] = driverLower.split('/')
            system: str = parts[len(parts) - 2]
            subSystem: str = parts[len(parts) - 1]
            # Sony Taito Gnet special case
            if system == "sony" and subSystem == "taitogn":
                system = "taito"
            # Capcom/Sega/... sub-systems
            if system in self.SUBSYSTEM_MANUFACTURERS.keys():
                driverTuples: list[tuple[str, str]] = self.SUBSYSTEM_MANUFACTURERS[system]
                for (name, suffix) in driverTuples:
                    if subSystem == name or (name.endswith('*') and subSystem.startswith(name[:-1])):
                        found = False
                        for i in range(len(manufacturers)):
                            if manufacturers[i].lower() == system:
                                manufacturers[i] = "{}\\{}".format(manufacturers[i], suffix)
                                found = True
                        if not found:
                            manufacturers.append("{}\\{}".format(system.capitalize(), suffix))
            # Neogeo special case
            if subSystem == "neogeo":
                manufacturers.append("neogeo")
            # Hyper Neogeo 64 special case
            if subSystem == "hng64":
                manufacturers.append("hng64")

        # Sort
        manufacturers.sort(key=str.lower)

        # stats
        for manu in manufacturers:
            if manu not in onestats.keys(): onestats[manu] = 0
            onestats[manu] += 1

        return manufacturers

    @staticmethod
    def caseRemap(manufacturers: list[str], onestats: dict[str, int]):
        # Full uppercase names must join Initial uppercase names
        for i in range(len(manufacturers)):
            if manufacturers[i].upper() == manufacturers[i]:
                initialUppercase = manufacturers[i][0] + manufacturers[i][1:].lower()
                if initialUppercase != manufacturers[i]:
                    if initialUppercase in onestats:
                        onestats[manufacturers[i]] -= 1
                        onestats[initialUppercase] += 1
                        manufacturers[i] = initialUppercase

    @staticmethod
    def purgeStats(onestats: dict[str, int]):
        # Remove zero'ed items
        toRemove: list[str] = []
        for k, v in onestats.items():
            if v <= 0: toRemove.append(k)
        for k in toRemove:
            onestats.pop(k)

    def buildRemapper(self, onestats: dict[str, int]) -> dict[str, str]:
        # Build onestat to purged map
        purgedMap: dict[str, str] = {}
        for k in onestats.keys():
            purgedMap[k] = self.purge(k)

        # Build purged to onestat (multiple) entries
        purgedToOne: dict[str, list[str]] = {}
        for k in onestats.keys():
            p = purgedMap[k]
            if p not in purgedToOne.keys(): purgedToOne[p] = []
            purgedToOne[p].append(k)

        # Remap entries that need to be
        remapper: dict[str, str] = {}
        for k, vlist in purgedToOne.items():
            if len(vlist) > 1:
                # Calculate scores
                scores: list[int] = []
                for v in vlist: scores.append(self.score(v))
                # Select highest
                scoreIndex = 0
                for i in range(len(scores)):
                    if scores[i] > scores[scoreIndex]: scoreIndex = i
                # Remap the others!
                for i in range(len(scores)):
                    if i != scoreIndex:
                        remapper[vlist[i]] = vlist[scoreIndex]
                        onestats[vlist[scoreIndex]] += onestats[vlist[i]]
                        onestats[vlist[i]] = -1

        return remapper

    def execute(self, debug: bool) -> bool:
        try:
            # Load
            lines: list[str] = self.Load()

            # Browse lines
            linesToManufacturers: dict[str, list[str]] = {}
            onestats: dict[str, int] = {}
            for line in lines:
                # Split
                chunks: list[str] = line.strip('\r\n').split('|')
                # Check
                if len(chunks) != 12:
                    print("Error reading chunk of {}".format(line))
                    linesToManufacturers[line] = []
                else:
                    manufacturers: list[str] = self.simplify(chunks[11], chunks[3], onestats)
                    linesToManufacturers[line] = manufacturers

            # Case remapping
            for line, manufacturers in linesToManufacturers.items():
                self.caseRemap(manufacturers, onestats)
            self.purgeStats(onestats)

            # Complex
            remapper: dict[str, str] = self.buildRemapper(onestats)
            for line, manufacturers in linesToManufacturers.items():
                for i in range(len(manufacturers)):
                    if manufacturers[i] in remapper:
                        manufacturers[i] = remapper[manufacturers[i]]
                self.purgeStats(onestats)

            # rebuild
            for i in range(len(lines)):
                # Retrieve & split
                line: str = lines[i]
                manufacturers: list[str] = linesToManufacturers[line]
                chunks: list[str] = line.strip('\r\n').split('|')
                # Replace manufacturers
                chunks[11] = '/'.join(manufacturers)
                # join again & save
                lines[i] = '|'.join(chunks)

            # save
            self.Save(lines)
            self.SaveManufacturers(onestats)

            if debug:
                # Display
                print("Final manufacturer list:")
                count: int = 0
                keys: list[str] = []
                for key in onestats.keys():
                    if onestats[key] > 0: keys.append(key)
                keys.sort(key=str.lower)
                for key in keys:
                    print("{} - {} ({})".format(count, key, onestats[key]))
                    count += 1

            # Success!
            return True

        except Exception as e:
            print("ERROR: Failed to process lst file because:\n" + str(e))
            return False


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='arcade database generator')
    parser.add_argument("-source", help="source lst file", type=str, required=True)
    parser.add_argument("-destination", help="destination lst file", type=str, required=True)
    arguments = parser.parse_args()

    fs = Simplifier(arguments.source, arguments.destination)
    fs.execute(True)
