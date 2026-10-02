#!/usr/bin/env python
import argparse
import logging
import sys
import json


class LessThanFilter(logging.Filter):
    def __init__(self, exclusive_maximum, name=""):
        super(LessThanFilter, self).__init__(name)
        self.max_level = exclusive_maximum

    def filter(self, record):
        #non-zero return means we log this message
        return 1 if record.levelno < self.max_level else 0


def parse_arguments(args):
    """
    simple command line arguments parser
    """
    parser = argparse.ArgumentParser(description="Install Nvidia drivers")
    parser.add_argument("--supported-gpus", help="path to supported-gpus.json")
    parser.add_argument(
        "-d", "--debug", help="enable debug logs", action=argparse.BooleanOptionalAction
    )
    return parser.parse_args(args)


def init_logger(debug):
    logger = logging.getLogger("Nvidia")
    logger.setLevel(logging.NOTSET)

    logging_handler_out = logging.StreamHandler(sys.stdout)
    logging_handler_out.setLevel(logging.DEBUG)
    logging_handler_out.addFilter(LessThanFilter(logging.WARNING))
    logger.addHandler(logging_handler_out)

    logging_handler_err = logging.StreamHandler(sys.stderr)
    logging_handler_err.setLevel(logging.WARNING)
    logger.addHandler(logging_handler_err)

    if debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def load_supported_gpus(file):
    with open(file, encoding="utf-8") as f:
        return json.load(f)


def generate_tables(chips):
    branches = {}
    branches["current"] = set([])
    for chip in chips:
        if "legacybranch" in chip:
            if not chip["legacybranch"] in branches:
                branches[chip["legacybranch"]] = set([])
            branches[chip["legacybranch"]].add(chip["devid"])
        else:
            branches["current"].add(chip["devid"])

    list_branches = {}
    for item,branch_set in branches.items():
        list_branches[item] = list(branch_set)

    return list_branches

if __name__ == "__main__":
    args = parse_arguments(sys.argv[1:])
    init_logger(args.debug)
    data = load_supported_gpus(args.supported_gpus)
    print(json.dumps(generate_tables(data["chips"])))
